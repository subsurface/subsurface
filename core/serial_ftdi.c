// SPDX-License-Identifier: LGPL-2.1+
/*
 * libdivecomputer
 *
 * Copyright (C) 2008 Jef Driesen
 * Copyright (C) 2014 Venkatesh Shukla
 * Copyright (C) 2015-2016 Anton Lundin
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

#include "libdivecomputer.h"
#include <libdivecomputer/custom_io.h>
#include <libdivecomputer/context.h>

#define VID 0x0403 // Vendor ID of FTDI

typedef struct ftdi_serial_t {
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
	unsigned int databits;
	unsigned int stopbits;
	unsigned int parity;
} ftdi_serial_t;

static dc_status_t serial_ftdi_get_received (dc_custom_io_t *io, size_t *value)
{
	ftdi_serial_t *device = (ftdi_serial_t*) io->userdata;

	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	// Direct access is not encouraged. But function implementation
	// is not available. The return quantity might be anything.
	// Find out further about its possible values and correct way of
	// access.
	*value = device->ftdi_ctx->readbuffer_remaining;

	return DC_STATUS_SUCCESS;
}

static dc_status_t serial_ftdi_get_transmitted (ftdi_serial_t *device)
{
	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	// This is not possible using libftdi. Look further into it.
	return DC_STATUS_UNSUPPORTED;
}

static dc_status_t serial_ftdi_sleep (ftdi_serial_t *device, unsigned long timeout)
{
	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	INFO (device->context, "Sleep: value=%lu", timeout);

	struct timespec ts;
	ts.tv_sec  = (timeout / 1000);
	ts.tv_nsec = (timeout % 1000) * 1000000;

	while (nanosleep (&ts, &ts) != 0) {
		if (errno != EINTR ) {
			SYSERROR (device->context, errno);
			return DC_STATUS_IO;
		}
	}

	return DC_STATUS_SUCCESS;
}


// Used internally for opening ftdi devices
static int serial_ftdi_open_device (struct ftdi_context *ftdi_ctx)
{
	INFO(0, "serial_ftdi_open_device called");
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
		INFO(0, "FTDI tried VID %04x pid %04x ret %d\n", VID, pid, ret);
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
static dc_status_t serial_ftdi_open (dc_custom_io_t *io, dc_context_t *context, const char* name)
{
	INFO(0, "serial_ftdi_open called");
	// Allocate memory.
	ftdi_serial_t *device = (ftdi_serial_t *) malloc (sizeof (ftdi_serial_t));
	if (device == NULL) {
		SYSERROR (context, errno);
		return DC_STATUS_NOMEMORY;
	}
	INFO(0, "setting up ftdi_ctx");
	struct ftdi_context *ftdi_ctx = ftdi_new();
	if (ftdi_ctx == NULL) {
		free(device);
		SYSERROR (context, errno);
		return DC_STATUS_NOMEMORY;
	}

	// Library context.
	//device->context = context;

	// Default to blocking reads.
	device->timeout = -1;

	// Default to full-duplex.
	device->halfduplex = 0;
	device->baudrate = 0;
	device->nbits = 0;
	device->databits = 0;
	device->stopbits = 0;
	device->parity = 0;

	// Initialize device ftdi context
	INFO(0, "initialize ftdi_ctx");
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
		free(device);
		ERROR (context, "%s", ftdi_get_error_string(ftdi_ctx));
		return DC_STATUS_IO;
	}

	if (ftdi_usb_purge_buffers(ftdi_ctx)) {
		free(device);
		ERROR (context, "%s", ftdi_get_error_string(ftdi_ctx));
		return DC_STATUS_IO;
	}

	device->ftdi_ctx = ftdi_ctx;

	io->userdata = device;

	return DC_STATUS_SUCCESS;
}

//
// Close the serial port.
//
static dc_status_t serial_ftdi_close (dc_custom_io_t *io)
{
	ftdi_serial_t *device = (ftdi_serial_t*) io->userdata;

	if (device == NULL)
		return DC_STATUS_SUCCESS;

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

	io->userdata = NULL;

	return DC_STATUS_SUCCESS;
}

//
// Configure the serial port (baudrate, databits, parity, stopbits and flowcontrol).
//
static dc_status_t serial_ftdi_configure (dc_custom_io_t *io, unsigned int baudrate, unsigned int databits, dc_parity_t parity, dc_stopbits_t stopbits, dc_flowcontrol_t flowcontrol)
{
	ftdi_serial_t *device = (ftdi_serial_t*) io->userdata;

	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	INFO (device->context, "Configure: baudrate=%i, databits=%i, parity=%i, stopbits=%i, flowcontrol=%i",
	      baudrate, databits, parity, stopbits, flowcontrol);

	enum ftdi_bits_type ft_bits;
	enum ftdi_stopbits_type ft_stopbits;
	enum ftdi_parity_type ft_parity;

	if (ftdi_set_baudrate(device->ftdi_ctx, baudrate) < 0) {
		ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
		return DC_STATUS_IO;
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
	case DC_PARITY_NONE: /**< No parity */
		ft_parity = NONE;
		break;
	case DC_PARITY_EVEN: /**< Even parity */
		ft_parity = EVEN;
		break;
	case DC_PARITY_ODD:  /**< Odd parity */
		ft_parity = ODD;
		break;
	case DC_PARITY_MARK: /**< Mark parity (always 1) */
	case DC_PARITY_SPACE: /**< Space parity (alwasy 0) */
	default:
		return DC_STATUS_INVALIDARGS;
	}

	// Set the number of stop bits.
	switch (stopbits) {
	case DC_STOPBITS_ONE:          /**< 1 stop bit */
		ft_stopbits = STOP_BIT_1;
		break;
	case DC_STOPBITS_TWO:           /**< 2 stop bits */
		ft_stopbits = STOP_BIT_2;
		break;
	case DC_STOPBITS_ONEPOINTFIVE: /**< 1.5 stop bits*/
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
	case DC_FLOWCONTROL_NONE:     /**< No flow control */
		if (ftdi_setflowctrl(device->ftdi_ctx, SIO_DISABLE_FLOW_CTRL) < 0) {
			ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
			return DC_STATUS_IO;
		}
		break;
	case DC_FLOWCONTROL_HARDWARE: /**< Hardware (RTS/CTS) flow control */
		if (ftdi_setflowctrl(device->ftdi_ctx, SIO_RTS_CTS_HS) < 0) {
			ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
			return DC_STATUS_IO;
		}
		break;
	case DC_FLOWCONTROL_SOFTWARE:  /**< Software (XON/XOFF) flow control */
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
	device->databits = databits;
	device->stopbits = stopbits;
	device->parity = parity;

	return DC_STATUS_SUCCESS;
}

//
// Configure the serial port (timeouts).
//
static dc_status_t serial_ftdi_set_timeout (dc_custom_io_t *io, long timeout)
{
	ftdi_serial_t *device = (ftdi_serial_t*) io->userdata;

	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	INFO (device->context, "Timeout: value=%li", timeout);

	device->timeout = timeout;

	return DC_STATUS_SUCCESS;
}

static dc_status_t serial_ftdi_set_halfduplex (dc_custom_io_t *io, unsigned int value)
{
	ftdi_serial_t *device = (ftdi_serial_t*) io->userdata;

	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	// Most ftdi chips support full duplex operation. ft232rl does.
	// Crosscheck other chips.

	device->halfduplex = value;

	return DC_STATUS_SUCCESS;
}

static dc_status_t serial_ftdi_read (dc_custom_io_t *io, void *data, size_t size, size_t *actual)
{
	ftdi_serial_t *device = (ftdi_serial_t*) io->userdata;

	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	// The total timeout.
	long timeout = device->timeout;

	// Simulate blocking read as 10s timeout
	if (timeout == -1)
		timeout = 10000;

	int backoff = 1;
	int slept = 0;
	unsigned int nbytes = 0;
	while (nbytes < size) {
		int n = ftdi_read_data (device->ftdi_ctx, (char *) data + nbytes, size - nbytes);
		if (n < 0) {
			if (n == LIBUSB_ERROR_INTERRUPTED)
				continue; //Retry.
			ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
			return DC_STATUS_IO; //Error during read call.
		} else if (n == 0) {
			if (slept >= timeout) {
				ERROR(device->context, "%s", "FTDI read timed out.");
				return DC_STATUS_TIMEOUT;
			}
			serial_ftdi_sleep (device, backoff);
			slept += backoff;
		}

		nbytes += n;
	}

	INFO (device->context, "Read %d bytes", nbytes);

	if (actual)
		*actual = nbytes;

	return DC_STATUS_SUCCESS;
}

static dc_status_t serial_ftdi_write (dc_custom_io_t *io, const void *data, size_t size, size_t *actual)
{
	ftdi_serial_t *device = (ftdi_serial_t*) io->userdata;

	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	struct timeval tve, tvb;
	if (device->halfduplex) {
		// Get the current time.
		if (gettimeofday (&tvb, NULL) != 0) {
			SYSERROR (device->context, errno);
			return DC_STATUS_IO;
		}
	}

	unsigned int nbytes = 0;
	while (nbytes < size) {

		int n = ftdi_write_data (device->ftdi_ctx, (char *) data + nbytes, size - nbytes);
		if (n < 0) {
			if (n == LIBUSB_ERROR_INTERRUPTED)
				continue; // Retry.
			ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
			return DC_STATUS_IO; // Error during write call.
		} else if (n == 0) {
			break; // EOF.
		}

		nbytes += n;
	}

	if (device->halfduplex) {
		// Get the current time.
		if (gettimeofday (&tve, NULL) != 0) {
			SYSERROR (device->context, errno);
			return DC_STATUS_IO;
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

	if (actual)
		*actual = nbytes;

	return DC_STATUS_SUCCESS;
}

static dc_status_t serial_ftdi_flush (dc_custom_io_t *io, dc_direction_t queue)
{
	ftdi_serial_t *device = (ftdi_serial_t*) io->userdata;

	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	size_t input;
	serial_ftdi_get_received (io, &input);
	INFO (device->context, "Flush: queue=%u, input=%lu, output=%i", queue, input,
	      serial_ftdi_get_transmitted (device));

	switch (queue) {
	case DC_DIRECTION_INPUT:  /**< Input direction */
		if (ftdi_usb_purge_tx_buffer(device->ftdi_ctx)) {
			ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
			return DC_STATUS_IO;
		}
		break;
	case DC_DIRECTION_OUTPUT: /**< Output direction */
		if (ftdi_usb_purge_rx_buffer(device->ftdi_ctx)) {
			ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
			return DC_STATUS_IO;
		}
		break;
	case DC_DIRECTION_ALL: /**< All directions */
	default:
		if (ftdi_usb_reset(device->ftdi_ctx)) {
			ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
			return DC_STATUS_IO;
		}
		break;
	}

	return DC_STATUS_SUCCESS;
}

static dc_status_t serial_ftdi_send_break (dc_custom_io_t *io)
{
	ftdi_serial_t *device = (ftdi_serial_t*) io->userdata;

	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	INFO (device->context, "Break : One time period.");

	// no direct functions for sending break signals in libftdi.
	// there is a suggestion to lower the baudrate and sending NUL
	// and resetting the baudrate up again. But it has flaws.
	// Not implementing it before researching more.

	return DC_STATUS_UNSUPPORTED;
}

static dc_status_t serial_ftdi_set_break (dc_custom_io_t *io, unsigned int level)
{
	ftdi_serial_t *device = (ftdi_serial_t*) io->userdata;

	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	INFO (device->context, "Break: value=%i", level);

	if (ftdi_set_line_property2(device->ftdi_ctx, device->databits, device->stopbits, device->parity, level)) {
		ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
		return DC_STATUS_IO;
	}

	return DC_STATUS_UNSUPPORTED;
}

static dc_status_t serial_ftdi_set_dtr (dc_custom_io_t *io, int level)
{
	ftdi_serial_t *device = (ftdi_serial_t*) io->userdata;

	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	INFO (device->context, "DTR: value=%i", level);

	if (ftdi_setdtr(device->ftdi_ctx, level)) {
		ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
		return DC_STATUS_IO;
	}

	return DC_STATUS_SUCCESS;
}

static dc_status_t serial_ftdi_set_rts (dc_custom_io_t *io, int level)
{
	ftdi_serial_t *device = (ftdi_serial_t*) io->userdata;

	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	INFO (device->context, "RTS: value=%i", level);

	if (ftdi_setrts(device->ftdi_ctx, level)) {
		ERROR (device->context, "%s", ftdi_get_error_string(device->ftdi_ctx));
		return DC_STATUS_IO;
	}

	return DC_STATUS_SUCCESS;
}

dc_custom_io_t serial_ftdi_ops = {
	.userdata = NULL,
	.serial_open = serial_ftdi_open,
	.serial_close = serial_ftdi_close,
	.serial_read = serial_ftdi_read,
	.serial_write = serial_ftdi_write,
	.serial_purge = serial_ftdi_flush,
	.serial_get_available = serial_ftdi_get_received,
	.serial_set_timeout = serial_ftdi_set_timeout,
	.serial_configure = serial_ftdi_configure,
	.serial_set_dtr = serial_ftdi_set_dtr,
	.serial_set_rts = serial_ftdi_set_rts,
	.serial_set_halfduplex = serial_ftdi_set_halfduplex,
// Can't be done in ftdi?
// only used in vyper2
	.serial_set_break = serial_ftdi_set_break,
};
