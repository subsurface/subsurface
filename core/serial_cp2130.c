/*
 * This is code from and inspired by https://www.silabs.com/Support%20Documents/TechnicalDocs/AN792.pdf
 */

#include <string.h>     // memset
#include <stdlib.h>		// malloc, free
#include <stdbool.h>	// bool
/*
#include <errno.h>      // errno
#include <sys/time.h>   // gettimeofday
#include <time.h>       // nanosleep
#include <stdio.h>
*/

#include <libusb.h>
#include <ftdi.h>

#include <libdivecomputer/custom_serial.h>

typedef struct cp2130_serial_t {
	/** libusb's context */
	struct libusb_context *context;
	/** libusb's usb_dev_handle */
	struct libusb_device_handle *cp2130Handle;
	/** Should we re-attach native driver? */
	bool kernelAttached;

	long timeout;
} cp2130_serial_t;

static dc_status_t cp2130_serial_close (void **userdata);
/*
8.1.3. Initialization and Device Discovery
The sample application shows the calls necessary to initialize and discover a device.
The steps that need to be taken to get a handle to the CP2130 device are:
1. Initialize LibUSB using libusb_init().
2. Get the device list using libusb_get_device_list() and find a device to connect to.
3. Open the device with LibUSB using libusb_open().
4. Detach any existing kernel connection by checking libusb_kernel_driver_active() and using
libusb_detach_kernel_driver() if it is connected to the kernel.
5. Claim the interface using libusb_claim_interface().
Here is the program listing from the sample application with comments for reference:
*/
static dc_status_t cp2130_serial_open (void **userdata, const char* name) {
	// Allocate memory.
	cp2130_serial_t *device = (cp2130_serial_t*) malloc (sizeof (cp2130_serial_t));
	libusb_device **deviceList = NULL;
	struct libusb_device_descriptor deviceDescriptor;
	libusb_device *usb_device = NULL;
	dc_status_t rc = DC_STATUS_SUCCESS;

	if (device == NULL)
		return DC_STATUS_NOMEMORY;

	memset(device, 0, sizeof (cp2130_serial_t));

	// Default to blocking io
	device->timeout = -1;

	// Initialize libusb
	if (libusb_init(&device->context) != 0)
		goto exit;

	// Search the connected devices to find and open a handle to the CP2130
	size_t deviceCount = libusb_get_device_list(device->context, &deviceList);
	if (deviceCount <= 0)
		goto exit;

	for (int i = 0; i < deviceCount; i++) {
		if (libusb_get_device_descriptor(deviceList[i], &deviceDescriptor) == 0) {
			if ((deviceDescriptor.idVendor == 0x10C4) && (deviceDescriptor.idProduct == 0x87A0)) {
				usb_device = deviceList[i];
				break;
			}
		}
	}
	if (usb_device == NULL) {
		rc = DC_STATUS_NODEVICE;
		goto exit;
	}

	// If a device is found, then open it
	if (libusb_open(usb_device, &device->cp2130Handle) != 0) {
		rc = DC_STATUS_IO;
		goto exit;
	}

	// See if a kernel driver is active already, if so detach it and store a
	// flag so we can reattach when we are done
	if (libusb_kernel_driver_active(device->cp2130Handle, 0) != 0) {
		libusb_detach_kernel_driver(device->cp2130Handle, 0);
		device->kernelAttached = true;
	}
	// Finally, claim the interface
	if (libusb_claim_interface(device->cp2130Handle, 0) != 0) {
		rc = DC_STATUS_IO;
		goto exit;
	}

	*userdata = device;

	if (deviceList)
		libusb_free_device_list(deviceList, 1);

	return rc;

exit:
	if (deviceList)
		libusb_free_device_list(deviceList, 1);

	(void)cp2130_serial_close((void**)&device);

	return rc;
}

/*
8.1.4. Uninitialization
The sample code also shows the calls necessary to uninitialize a device.
The steps need to be taken to disconnect from the CP2130 device are:
1. Release the interface using libusb_release_interface().
2. Reattach from the kernel using libusb_attach_kernel_driver() (only if the device was connected to the
kernel previously).
3. Close the LibUSB handle using libusb_close().
4. Free the device list we obtained originaly using libusb_free_device_list().
5. Uninitialize LibUSB using libusb_exit().
Here is the program listing from the sample application for reference:
*/
static dc_status_t cp2130_serial_close (void **userdata) {
	cp2130_serial_t *device = (cp2130_serial_t*) *userdata;

	if (device == NULL)
		return DC_STATUS_SUCCESS;

	if (device->cp2130Handle)
		libusb_release_interface(device->cp2130Handle, 0);
	if (device->kernelAttached)
		libusb_attach_kernel_driver(device->cp2130Handle, 0);
	if (device->cp2130Handle)
		libusb_close(device->cp2130Handle);
	if (device->context)
		libusb_exit(device->context);

	free(device);

	return DC_STATUS_SUCCESS;
}

/*
8.1.5.1. Control Requests
The example GPIO function will get/set the GPIO values with a control request. Each of the commands defined in
section "6. Configuration and Control Commands (Control Transfers)" can be used with the LibUSB control
request function. Each of the paramaters will map to the LibUSB function. In this example we will refer to section
"6.6. Get_GPIO_Values (Command ID 0x20)" .
In this command there is a bmRequestType, bRequest and wLength. In this case the other paramaters, wValue
and wIndex, are set to 0. These parameters are used directly with the libusb_control_transfer_function and it will
return the number of bytes transferred. Here is the definition:

int libusb_control_transfer(libusb_device_handle* dev_handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char* data, uint16_t wLength, unsigned int timeout)

After putting the defined values from section "6.6.2. Setup Stage (OUT Transfer)" in the function, this is the resulting call to get the GPIO

unsigned char control_buf_out[2];
libusb_control_transfer(cp2130Handle, 0xC0, 0x20, 0x0000, 0x0000, control_buf_out, sizeof(control_buf_out), usbTimeout);

*/

/*
8.1.5.2. Bulk OUT Requests
The example write function will send data to the SPI MOSI line. To perform writes, use the description in section
"5.2. Write (Command ID 0x01)" to transmit data with the LibUSB bulk transfer function. Here is the definition:
int libusb_bulk_transfer(struct libusb_device_handle* dev_handle, unsigned char endpoint,
unsigned char* data, int length, int * transferred, unsigned int timeout)
To perform a write to the MOSI line, pack a buffer with the specified data and payload then send the entire packet.
Here is an example from the sample application that will write 6 bytes to endpoint 1:
*/
static dc_status_t cp2130_serial_write(void **userdata, const void *data, size_t size, size_t *actual) {
	cp2130_serial_t *device = (cp2130_serial_t*) *userdata;
	int libusb_status;
	int bytesWritten;

	if (device == NULL)
		return DC_STATUS_SUCCESS;

	unsigned char write_command_buf[14] = {
		0x00, 0x00, // Reserved
		0x01, // Write command
		0x00, // Reserved
		// Number of bytes, little-endian
		size & 0xFF,
		(size >> 8) & 0xFF,
		(size >> 16) & 0xFF,
		(size >> 24) & 0xFF,
	};

	libusb_status = libusb_bulk_transfer(device->cp2130Handle, 0x01, write_command_buf, sizeof(write_command_buf), &bytesWritten, device->timeout);

	if (libusb_status != 0 || bytesWritten != sizeof(write_command_buf))
		return DC_STATUS_IO; // Simplified for now.

	libusb_status = libusb_bulk_transfer(device->cp2130Handle, 0x01, (unsigned char*) data, size, &bytesWritten, device->timeout);

	if (actual)
		*actual = bytesWritten;

	if (libusb_status == 0)
		return DC_STATUS_SUCCESS;
	else
		return DC_STATUS_IO; // Simplified for now.
}
/*
The function will return 0 upon success, otherwise it will return an error code to check for the failure reason.
*/

/*
8.1.5.3. Bulk IN Requests
Note: Because there is no input to the SPI the read is commented out. The code itself demonstrates reading 6 bytes, but will
not succeed since there is nothing to send this data to the host. This code is meant to serve as an example of how to
perform a read in a developed system.
The example read function will send a request to read data from the SPI MISO line. To perform reads, use the
description in section "5.1. Read (Command ID 0x00)" to request data with the LibUSB bulk transfer function (see
definition in "8.1.5.2. Bulk OUT Requests" , or the LibUSB documentation).
To perform a read from the MISO line, pack a buffer with the specified read command then send the entire packet.
Immediately after that, perform another bulk request to get the response. Here is an example from the sample
application that will try to read 6 bytes from endpoint 1:
*/

static dc_status_t cp2130_serial_read (void **userdata, void *data, size_t size, size_t *actual) {
	cp2130_serial_t *device = (cp2130_serial_t*) *userdata;
	int libusb_status;
	int bytesWritten, bytesRead;

	if (device == NULL)
		return DC_STATUS_SUCCESS;

	// This example shows how to issue a bulk read request to the SPI MISO line
	unsigned char read_command_buf[14] = {
		0x00, 0x00, // Reserved
		0x00, // Read command
		0x00, // Reserved
		// Read number of bytes, little-endian
		size & 0xFF,
		(size >> 8) & 0xFF,
		(size >> 16) & 0xFF,
		(size >> 24) & 0xFF,
	};

	libusb_status = libusb_bulk_transfer(device->cp2130Handle, 0x01, read_command_buf, sizeof(read_command_buf), &bytesWritten, device->timeout);

	if (libusb_status != 0  || bytesWritten != sizeof(read_command_buf))
		return DC_STATUS_IO; // Simplified for now.

	libusb_status = libusb_bulk_transfer(device->cp2130Handle, 0x01, (unsigned char*) data, size, &bytesRead, device->timeout);

	if (actual)
		*actual = bytesRead;

	if (libusb_status == 0)
		return DC_STATUS_SUCCESS;
	else
		return DC_STATUS_IO; // Simplified for now.
}
/*
The bulk transfer function will return 0 upon success, otherwise it will return an error code to check for the failure
reason. In this case make sure to check that the bytesWritten is the same as the command buffer size as well as a
successful transfer.
*/

dc_custom_serial_t cp2130_serial_ops = {
	.userdata = NULL,
	.open = cp2130_serial_open,
	.close = cp2130_serial_close,
	.read = cp2130_serial_read,
	.write = cp2130_serial_write,
// NULL means NOP
	.purge = NULL,
	.get_available = NULL,
	.set_timeout = NULL,
	.configure = NULL,
	.set_dtr = NULL,
	.set_rts = NULL,
	.set_halfduplex = NULL,
	.set_break = NULL
};
