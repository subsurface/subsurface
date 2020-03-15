#ifndef SERIAL_USB_ANDROID_H
#define SERIAL_USB_ANDROID_H

#include <string>
#include <vector>

/* USB Device Information */
struct android_usb_serial_device_descriptor {
	QAndroidJniObject usbDevice; /* the UsbDevice */
	std::string className; /* the driver class name. If empty, then "autodetect" */
	std::string uiRepresentation; /* The string that can be used for the user interface. */

	// Device information
	std::string usbManufacturer;
	std::string usbProduct;
	std::string manufacturer;
	std::string product;
	uint16_t pid;
	uint16_t vid;
};

std::vector<android_usb_serial_device_descriptor> serial_usb_android_get_devices();
android_usb_serial_device_descriptor getDescriptor(QAndroidJniObject usbDevice);

#endif
