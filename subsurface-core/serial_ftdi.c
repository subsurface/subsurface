/*
 * libdivecomputer
 *
 * Copyright (C) 2008 Jef Driesen
 * Copyright (C) 2014 Venkatesh Shukla
 * Copyright (C) 2015 Anton Lundin
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#include <stdlib.h> // malloc, free
#include <string.h>     // strerror
#include <errno.h>      // errno
#include <sys/time.h>   // gettimeofday
#include <time.h>       // nanosleep
#include <stdio.h>

#include <libusb.h>
#include <ftdi.h>

#ifndef __ANDROID__
#define INFO(context, fmt, ...)	fprintf(stderr, "INFO: " fmt "\n", ##__VA_ARGS__)
#define ERROR(context, fmt, ...)	fprintf(stderr, "ERROR: " fmt "\n", ##__VA_ARGS__)
#else
#include <android/log.h>
#define INFO(context, fmt, ...)	__android_log_print(ANDROID_LOG_DEBUG, __FILE__, "INFO: " fmt "\n", ##__VA_ARGS__)
#define ERROR(context, fmt, ...)	__android_log_print(ANDROID_LOG_DEBUG, __FILE__, "ERROR: " fmt "\n", ##__VA_ARGS__)
#endif
//#define SYSERROR(context, errcode)	ERROR(__FILE__ ":" __LINE__ ": %s", strerror(errcode))
#define SYSERROR(context, errcode)	;

#include <libdivecomputer/custom_serial.h>

/* Verbatim copied libdivecomputer enums to support configure */
typedef enum serial_parity_t {
	SERIAL_PARITY_NONE,
	SERIAL_PARITY_EVEN,
	SERIAL_PARITY_ODD
} serial_parity_t;

typedef enum serial_flowcontrol_t {
	SERIAL_FLOWCONTROL_NONE,
	SERIAL_FLOWCONTROL_HARDWARE,
	SERIAL_FLOWCONTROL_SOFTWARE
} serial_flowcontrol_t;

typedef enum serial_queue_t {
	SERIAL_QUEUE_INPUT = 0x01,
	SERIAL_QUEUE_OUTPUT = 0x02,
	SERIAL_QUEUE_BOTH = SERIAL_QUEUE_INPUT | SERIAL_QUEUE_OUTPUT
} serial_queue_t;

typedef enum serial_line_t {
	SERIAL_LINE_DCD, // Data carrier detect
	SERIAL_LINE_CTS, // Clear to send
	SERIAL_LINE_DSR, // Data set ready
	SERIAL_LINE_RNG, // Ring indicator
} serial_line_t;

#define VID 0x0403 // Vendor ID of FTDI

#define MAX_BACKOFF 500 // Max milliseconds to wait before timing out.

typedef struct serial_t {
	/* Library context. */
	dc_context_t *context;
	/*
	 * The file descriptor corresponding to the serial port.
	 * Also a libftdi_ftdi_ctx could be used?
	 */
	struct ftdi_context *ftdi_ctx;
	long timeout;
	/*
	 * Serial port settings are saved into this variable immediately
	 * after the port is opened. These settings are restored when the
	 * serial port is closed.
	 * Saving this using libftdi context or libusb. Search further.
	 * Custom implementation using libftdi functions could be done.
	 */

	/* Half-duplex settings */
	int halfduplex;
	unsigned int baudrate;
	unsigned int nbits;
} serial_t;

static int serial_ftdi_get_received (serial_t *device)
{
	if (device == NULL)
		return -1; // EINVAL (Invalid argument)

	// Direct access is not encouraged. But function implementation
	// is not available. The return quantity might be anything.
	// Find out further about its possible values and correct way of
	// access.
	int bytes = device->ftdi_ctx->readbuffer_remaining;

	return bytes;
}

static int serial_ftdi_get_transmitted (serial_t *device)
{
	if (device == NULL)
		return -1; // EINVAL (Invalid argument)

	// This is not possible using libftdi. Look further into it.
	return -1;
}

static int serial_ftdi_sleep (serial_t *device, unsigned long timeout)
{
	if (device == NULL)
		return -1;

	INFO (device->context, "Sleep: value=%lu", timeout);

	struct timespec ts;
	ts.tv_sec  = (timeout / 1000);
	ts.tv_nsec = (timeout % 1000) * 1000000;

	while (nanosleep (&ts, &ts) != 0) {
		if (errno != EINTR ) {
			SYSERROR (device->context, errno);
			return -1;
		}
	}

	return 0;
}


// Used internally for opening ftdi devices
static int serial_ftdi_open_device (struct ftdi_context *ftdi_ctx)
{
	int accepted_pids[] = { 0x6001, 0x6010, 0x6011, // Suunto (Smart Interface), Heinrichs Weikamp
		0xF460, // Oceanic
		0xF680, // Suunto
		0x87D0, // Cressi (Leonardo)
	};
	int num_accepted_pids = 6;
	int i, pid, ret;
	for (i = 0; i < num_accepted_pids; i++) {
		pid = accepted_pids[i];
		ret = ftdi_usb_open (ftdi_ctx, VID, pid);
		if (ret == -3) // Device not found
			continue;
		else
			return ret;
	}
	// No supported devices are attached.
	return ret;
}

//
// Open the serial port.
// Initialise ftdi_context and use it to open the device
//
//FIXME: ugly forward declaration of serial_ftdi_configure, util we support configure for real...
static dc_status_t serial_ftdi_configure (serial_t *device, int baudrate, int databits, int parity, int stopbits, int flowcontrol);
static dc_status_t serial_ftdi_open (serial_t **out, dc_context_t *context, const char* name)
{
	if (out == NULL)
		return -1; // EINVAL (Invalid argument)

	INFO (context, "Open: name=%s", name ? name : "");

	// Allocate memory.
	serial_t *device = (serial_t *) malloc (sizeof (serial_t));
	if (device == NULL) {
		SYSERROR (context, errno);
		return DC_STATUS_NOMEMORY;
	}

	struct ftdi_context *ftdi_ctx = ftdi_new();
	if (ftdi_ctx == NULL) {
		free(device);
		SYSERROR (context, errno);
		return DC_STATUS_NOMEMORY;
	}

	// Library context.
	device->context = context;

	// Default to blocking reads.
	device->timeout = -1;

	// Default to full-duplex.
	device->halfduplex = 0;
	device->baudrate = 0;
	device->nbits = 0;

	// Initialize device ftdi context
	ftdi_init(ftdi_ctx);

	if (ftdi_set_interface(ftdi_ctx,INTERFACE_ANY)) {
		free(device);
		ERROR (context, "%s", ftdi_get_error_string(ftdi_ctx));
		return DC_STATUS_IO;
	}

	if (serial_ftdi_open_device(ftdi_ctx) < 0) {
		free(device);
		ERROR (context, "%s", ftdi_get_error_string(ftdi_ctx));
		return DC_STATUS_IO;
	}

	if (ftdi_usb_reset(ftdi_ctx)) {
		ERROR (context, "%s", ftdi_get_error_string(ftdi_ctx));
		return DC_STATUS_IO;
	}

	if (ftdi_usb_purge_buffers(ftdi_ctx)) {
		free(device);
		ERROR (context, "%s", ftdi_get_error_string(ftdi_ctx));
		return DC_STATUS_IO;
	}

	device->ftdi_ctx = ftdi_ctx;

	//FIXME: remove this when custom-serial have support for configure calls
	serial_ftdi_configure (device, 115200, 8, 0, 1, 0);

	*out = device;

	return DC_STATUS_SUCCESS;
}

//
// Close the serial port.
//
static int serial_ftdi_close (serial_t *device)
{
	if (device == NULL)
		return 0;

	// Restore the initial terminal attributes.
	// See if it is possible using libusb or libftdi

	int ret = ftdi_usb_close(device->ftdi_ctx);
	if (ret < 0) {
		ERROR (device->context, "Unable to close the ftdi device : %d (%s)\n",
		       ret, ftdi_get_error_string(device->ftdi_ctx));
		return ret;
	}

	ftdi_free(device->ftdi_ctx);

	// Free memory.
	free (device);

	return 0;
}

//
// Configure the serial port (baudrate, databits, parity, stopbits and flowcontrol).
//
static dc_status_t serial_ftdi_configure (serial_t *device, int baudrate, int databits, int parity, int stopbits, int flowcontrol)
{
	if (device == NULL)
		return -1; // EINVAL (Invalid argument)

	INFO (device->context, "Configure: baudrate=%i, databits=%i, parity=%i, stopbits=%i, flowcontrol=%i",
	      baudrate, databits, parity, stopbits, flowcontrol);

	enum ftdi_bits_type ft_bits;
	enum ftdi_stopbits_type ft_stopbits;
	enum ftdi_parity_type ft_parity;

	if (ftdi_set_baudrate(device->ftdi_ctx, baudrate) < 0) {
		ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
		return -1;
	}

	// Set the character size.
	switch (databits) {
	case 7:
		ft_bits = BITS_7;
		break;
	case 8:
		ft_bits = BITS_8;
		break;
	default:
		return DC_STATUS_INVALIDARGS;
	}

	// Set the parity type.
	switch (parity) {
	case SERIAL_PARITY_NONE: // No parity
		ft_parity = NONE;
		break;
	case SERIAL_PARITY_EVEN: // Even parity
		ft_parity = EVEN;
		break;
	case SERIAL_PARITY_ODD: // Odd parity
		ft_parity = ODD;
		break;
	default:
		return DC_STATUS_INVALIDARGS;
	}

	// Set the number of stop bits.
	switch (stopbits) {
	case 1: // One stopbit
		ft_stopbits = STOP_BIT_1;
		break;
	case 2: // Two stopbits
		ft_stopbits = STOP_BIT_2;
		break;
	default:
		return DC_STATUS_INVALIDARGS;
	}

	// Set the attributes
	if (ftdi_set_line_property(device->ftdi_ctx, ft_bits, ft_stopbits, ft_parity)) {
		ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
		return DC_STATUS_IO;
	}

	// Set the flow control.
	switch (flowcontrol) {
	case SERIAL_FLOWCONTROL_NONE: // No flow control.
		if (ftdi_setflowctrl(device->ftdi_ctx, SIO_DISABLE_FLOW_CTRL) < 0) {
			ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
			return DC_STATUS_IO;
		}
		break;
	case SERIAL_FLOWCONTROL_HARDWARE: // Hardware (RTS/CTS) flow control.
		if (ftdi_setflowctrl(device->ftdi_ctx, SIO_RTS_CTS_HS) < 0) {
			ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
			return DC_STATUS_IO;
		}
		break;
	case SERIAL_FLOWCONTROL_SOFTWARE: // Software (XON/XOFF) flow control.
		if (ftdi_setflowctrl(device->ftdi_ctx, SIO_XON_XOFF_HS) < 0) {
			ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
			return DC_STATUS_IO;
		}
		break;
	default:
		return DC_STATUS_INVALIDARGS;
	}

	device->baudrate = baudrate;
	device->nbits = 1 + databits + stopbits + (parity ? 1 : 0);

	return DC_STATUS_SUCCESS;
}

//
// Configure the serial port (timeouts).
//
static int serial_ftdi_set_timeout (serial_t *device, long timeout)
{
	if (device == NULL)
		return -1; // EINVAL (Invalid argument)

	INFO (device->context, "Timeout: value=%li", timeout);

	device->timeout = timeout;

	return 0;
}

static int serial_ftdi_set_halfduplex (serial_t *device, int value)
{
	if (device == NULL)
		return -1; // EINVAL (Invalid argument)

	// Most ftdi chips support full duplex operation. ft232rl does.
	// Crosscheck other chips.

	device->halfduplex = value;

	return 0;
}

static int serial_ftdi_read (serial_t *device, void *data, unsigned int size)
{
	if (device == NULL)
		return -1; // EINVAL (Invalid argument)

	// The total timeout.
	long timeout = device->timeout;

	// The absolute target time.
	struct timeval tve;

	static int backoff = 1;
	int init = 1;
	unsigned int nbytes = 0;
	while (nbytes < size) {
		struct timeval tvt;
		if (timeout > 0) {
			struct timeval now;
			if (gettimeofday (&now, NULL) != 0) {
				SYSERROR (device->context, errno);
				return -1;
			}

			if (init) {
				// Calculate the initial timeout.
				tvt.tv_sec  = (timeout / 1000);
				tvt.tv_usec = (timeout % 1000) * 1000;
				// Calculate the target time.
				timeradd (&now, &tvt, &tve);
			} else {
				// Calculate the remaining timeout.
				if (timercmp (&now, &tve, <))
					timersub (&tve, &now, &tvt);
				else
					timerclear (&tvt);
			}
			init = 0;
		} else if (timeout == 0) {
			timerclear (&tvt);
		}

		int n = ftdi_read_data (device->ftdi_ctx, (char *) data + nbytes, size - nbytes);
		if (n < 0) {
			if (n == LIBUSB_ERROR_INTERRUPTED)
				continue; //Retry.
			ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
			return -1; //Error during read call.
		} else if (n == 0) {
			// Exponential backoff.
			if (backoff > MAX_BACKOFF) {
				ERROR(device->context, "%s", "FTDI read timed out.");
				return -1;
			}
			serial_ftdi_sleep (device, backoff);
			backoff *= 2;
		} else {
			// Reset backoff to 1 on success.
			backoff = 1;
		}

		nbytes += n;
	}

	INFO (device->context, "Read %d bytes", nbytes);

	return nbytes;
}

static int serial_ftdi_write (serial_t *device, const void *data, unsigned int size)
{
	if (device == NULL)
		return -1; // EINVAL (Invalid argument)

	struct timeval tve, tvb;
	if (device->halfduplex) {
		// Get the current time.
		if (gettimeofday (&tvb, NULL) != 0) {
			SYSERROR (device->context, errno);
			return -1;
		}
	}

	unsigned int nbytes = 0;
	while (nbytes < size) {

		int n = ftdi_write_data (device->ftdi_ctx, (char *) data + nbytes, size - nbytes);
		if (n < 0) {
			if (n == LIBUSB_ERROR_INTERRUPTED)
				continue; // Retry.
			ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
			return -1; // Error during write call.
		} else if (n == 0) {
			break; // EOF.
		}

		nbytes += n;
	}

	if (device->halfduplex) {
		// Get the current time.
		if (gettimeofday (&tve, NULL) != 0) {
			SYSERROR (device->context, errno);
			return -1;
		}

		// Calculate the elapsed time (microseconds).
		struct timeval tvt;
		timersub (&tve, &tvb, &tvt);
		unsigned long elapsed = tvt.tv_sec * 1000000 + tvt.tv_usec;

		// Calculate the expected duration (microseconds). A 2 millisecond fudge
		// factor is added because it improves the success rate significantly.
		unsigned long expected = 1000000.0 * device->nbits / device->baudrate * size + 0.5 + 2000;

		// Wait for the remaining time.
		if (elapsed < expected) {
			unsigned long remaining = expected - elapsed;

			// The remaining time is rounded up to the nearest millisecond to
			// match the Windows implementation. The higher resolution is
			// pointless anyway, since we already added a fudge factor above.
			serial_ftdi_sleep (device, (remaining + 999) / 1000);
		}
	}

	INFO (device->context, "Wrote %d bytes", nbytes);

	return nbytes;
}

static int serial_ftdi_flush (serial_t *device, int queue)
{
	if (device == NULL)
		return -1; // EINVAL (Invalid argument)

	INFO (device->context, "Flush: queue=%u, input=%i, output=%i", queue,
	      serial_ftdi_get_received (device),
	      serial_ftdi_get_transmitted (device));

	switch (queue) {
	case SERIAL_QUEUE_INPUT:
		if (ftdi_usb_purge_tx_buffer(device->ftdi_ctx)) {
			ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
			return -1;
		}
		break;
	case SERIAL_QUEUE_OUTPUT:
		if (ftdi_usb_purge_rx_buffer(device->ftdi_ctx)) {
			ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
			return -1;
		}
		break;
	default:
		if (ftdi_usb_purge_buffers(device->ftdi_ctx)) {
			ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
			return -1;
		}
		break;
	}

	return 0;
}

static int serial_ftdi_send_break (serial_t *device)
{
	if (device == NULL)
		return -1; // EINVAL (Invalid argument)a

	INFO (device->context, "Break : One time period.");

	// no direct functions for sending break signals in libftdi.
	// there is a suggestion to lower the baudrate and sending NUL
	// and resetting the baudrate up again. But it has flaws.
	// Not implementing it before researching more.

	return -1;
}

static int serial_ftdi_set_break (serial_t *device, int level)
{
	if (device == NULL)
		return -1; // EINVAL (Invalid argument)

	INFO (device->context, "Break: value=%i", level);

	// Not implemented in libftdi yet. Research it further.

	return -1;
}

static int serial_ftdi_set_dtr (serial_t *device, int level)
{
	if (device == NULL)
		return -1; // EINVAL (Invalid argument)

	INFO (device->context, "DTR: value=%i", level);

	if (ftdi_setdtr(device->ftdi_ctx, level)) {
		ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
		return -1;
	}

	return 0;
}

static int serial_ftdi_set_rts (serial_t *device, int level)
{
	if (device == NULL)
		return -1; // EINVAL (Invalid argument)

	INFO (device->context, "RTS: value=%i", level);

	if (ftdi_setrts(device->ftdi_ctx, level)) {
		ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
		return -1;
	}

	return 0;
}

const dc_serial_operations_t serial_ftdi_ops = {
	.open = serial_ftdi_open,
	.close = serial_ftdi_close,
	.read = serial_ftdi_read,
	.write = serial_ftdi_write,
	.flush = serial_ftdi_flush,
	.get_received = serial_ftdi_get_received,
	.get_transmitted = NULL, /*NOT USED ANYWHERE! serial_ftdi_get_transmitted  */
	.set_timeout = serial_ftdi_set_timeout
#ifdef FIXED_SSRF_CUSTOM_SERIAL
	,
	.configure = serial_ftdi_configure,
//static int serial_ftdi_configure (serial_t *device, int baudrate, int databits, int parity, int stopbits, int flowcontrol)
	.set_halfduplex = serial_ftdi_set_halfduplex,
//static int serial_ftdi_set_halfduplex (serial_t *device, int value)
	.send_break = serial_ftdi_send_break,
//static int serial_ftdi_send_break (serial_t *device)
	.set_break = serial_ftdi_set_break,
//static int serial_ftdi_set_break (serial_t *device, int level)
	.set_dtr = serial_ftdi_set_dtr,
//static int serial_ftdi_set_dtr (serial_t *device, int level)
	.set_rts = serial_ftdi_set_rts
//static int serial_ftdi_set_rts (serial_t *device, int level)
#endif
};

dc_status_t dc_serial_ftdi_open(dc_serial_t **out, dc_context_t *context)
{
	if (out == NULL)
		return DC_STATUS_INVALIDARGS;

	// Allocate memory.
	dc_serial_t *serial_device = (dc_serial_t *) malloc (sizeof (dc_serial_t));

	if (serial_device == NULL) {
		return DC_STATUS_NOMEMORY;
	}

	// Initialize data and function pointers
	dc_serial_init(serial_device, NULL, &serial_ftdi_ops);

	// Open the serial device.
	dc_status_t rc = (dc_status_t) serial_ftdi_open (&serial_device->port, context, NULL);
	if (rc != DC_STATUS_SUCCESS) {
		free (serial_device);
		return rc;
	}

	// Set the type of the device
	serial_device->type = DC_TRANSPORT_USB;;

	*out = serial_device;

	return DC_STATUS_SUCCESS;
}
