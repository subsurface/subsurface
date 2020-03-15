#include "libdivecomputer.h"
#include <libdivecomputer/context.h>
#include <libdivecomputer/custom.h>

#include <cstring>

#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#include <QtAndroid>
#include <QRegularExpression>

#include <thread>

#include <android/log.h>

#include "serial_usb_android.h"

#define INFO(context, fmt, ...)	 __android_log_print(ANDROID_LOG_DEBUG, __FILE__, "INFO: " fmt "\n", ##__VA_ARGS__)
#define ERROR(context, fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, __FILE__, "ERROR: " fmt "\n", ##__VA_ARGS__)
#define TRACE INFO

static dc_status_t serial_usb_android_sleep(void *io, unsigned int timeout)
{
	TRACE (device->context, "%s: %i", __FUNCTION__, timeout);

	QAndroidJniObject *device = static_cast<QAndroidJniObject *>(io);
	if (device == nullptr)
		return DC_STATUS_INVALIDARGS;

	std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
	return DC_STATUS_SUCCESS;
}

static dc_status_t serial_usb_android_set_timeout(void *io, int timeout)
{
	TRACE (device->context, "%s: %i", __FUNCTION__, timeout);

	QAndroidJniObject *device = static_cast<QAndroidJniObject *>(io);
	if (device == nullptr)
		return DC_STATUS_INVALIDARGS;

	return static_cast<dc_status_t>(device->callMethod<jint>("set_timeout", "(I)I", timeout));
}

static dc_status_t serial_usb_android_set_dtr(void *io, unsigned int value)
{
	TRACE (device->context, "%s: %i", __FUNCTION__, value);

	QAndroidJniObject *device = static_cast<QAndroidJniObject *>(io);
	if (device == nullptr)
		return DC_STATUS_INVALIDARGS;

	return static_cast<dc_status_t>(device->callMethod<jint>("set_dtr", "(Z)I", value));
}

static dc_status_t serial_usb_android_set_rts(void *io, unsigned int value)
{
	TRACE (device->context, "%s: %i", __FUNCTION__, value);

	QAndroidJniObject *device = static_cast<QAndroidJniObject *>(io);
	if (device == nullptr)
		return DC_STATUS_INVALIDARGS;

	return static_cast<dc_status_t>(device->callMethod<jint>("set_rts", "(Z)I", value));
}

static dc_status_t serial_usb_android_close(void *io)
{
	TRACE (device->context, "%s", __FUNCTION__);

	QAndroidJniObject *device = static_cast<QAndroidJniObject *>(io);
	if (device == nullptr)
		return DC_STATUS_SUCCESS;

	auto retval = static_cast<dc_status_t>(device->callMethod<jint>("close", "()I"));
	delete device;
	return retval;
}

static dc_status_t serial_usb_android_purge(void *io, dc_direction_t direction)
{
	TRACE (device->context, "%s: %i", __FUNCTION__, direction);

	QAndroidJniObject *device = static_cast<QAndroidJniObject *>(io);
	if (device == nullptr)
		return DC_STATUS_INVALIDARGS;

	return static_cast<dc_status_t>(device->callMethod<jint>("purge", "(I)I", direction));
}

static dc_status_t serial_usb_android_configure(void *io, unsigned int baudrate, unsigned int databits, dc_parity_t parity,
			     dc_stopbits_t stopbits, dc_flowcontrol_t flowcontrol)
{
	TRACE (device->context, "%s: baudrate=%i, databits=%i, parity=%i, stopbits=%i, flowcontrol=%i", __FUNCTION__,
	       baudrate, databits, parity, stopbits, flowcontrol);

	QAndroidJniObject *device = static_cast<QAndroidJniObject *>(io);
	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	return static_cast<dc_status_t>(device->callMethod<jint>("configure", "(IIII)I", baudrate, databits, parity, stopbits));
}

/*
static dc_status_t serial_usb_android_get_available (void *io, size_t *value)
{
	INFO (device->context, "%s", __FUNCTION__);

	QAndroidJniObject *device = static_cast<QAndroidJniObject*>(io);
	if (device == NULL)
		return DC_STATUS_INVALIDARGS;


	auto retval = device->callMethod<jint>("get_available", "()I");
	if(retval < 0){
		INFO (device->context, "Error in %s, retval %i", __FUNCTION__, retval);
		return static_cast<dc_status_t>(retval);
	}

	*value = retval;
	return DC_STATUS_SUCCESS;
}
*/

static dc_status_t serial_usb_android_read(void *io, void *data, size_t size, size_t *actual)
{
	TRACE (device->context, "%s: size: %zu", __FUNCTION__, size);

	QAndroidJniObject *device = static_cast<QAndroidJniObject *>(io);
	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	QAndroidJniEnvironment env;
	jbyteArray array = env->NewByteArray(size);
	env->SetByteArrayRegion(array, 0, size, (const jbyte *) data);

	auto retval = device->callMethod<jint>("read", "([B)I", array);
	if (retval < 0) {
		env->DeleteLocalRef(array);
		INFO (device->context, "Error in %s, retval %i", __FUNCTION__, retval);
		return static_cast<dc_status_t>(retval);
	}
	*actual = retval;
	env->GetByteArrayRegion(array, 0, retval, (jbyte *) data);
	env->DeleteLocalRef(array);
	TRACE (device->context, "%s: actual read size: %i", __FUNCTION__, retval);

	if (retval < size)
		return DC_STATUS_TIMEOUT;
	else
		return DC_STATUS_SUCCESS;
}

static dc_status_t serial_usb_android_write(void *io, const void *data, size_t size, size_t *actual)
{
	TRACE (device->context, "%s: size: %zu", __FUNCTION__, size);

	QAndroidJniObject *device = static_cast<QAndroidJniObject *>(io);
	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	QAndroidJniEnvironment env;
	jbyteArray array = env->NewByteArray(size);
	env->SetByteArrayRegion(array, 0, size, (const jbyte *) data);

	auto retval = device->callMethod<jint>("write", "([B)I", array);
	env->DeleteLocalRef(array);
	if (retval < 0) {
		INFO (device->context, "Error in %s, retval %i", __FUNCTION__, retval);
		return static_cast<dc_status_t>(retval);
	}
	*actual = retval;
	TRACE (device->context, "%s: actual write size: %i", __FUNCTION__, retval);
	return DC_STATUS_SUCCESS;
}

dc_status_t serial_usb_android_open(dc_iostream_t **iostream, dc_context_t *context, QAndroidJniObject usbDevice, std::string driverClassName)
{
	TRACE(device->contxt, "%s", __FUNCTION__);

	static const dc_custom_cbs_t callbacks = {
		.set_timeout = serial_usb_android_set_timeout, /* set_timeout */
		.set_dtr = serial_usb_android_set_dtr, /* set_dtr */
		.set_rts = serial_usb_android_set_rts, /* set_rts */
		//.get_available = serial_usb_android_get_available,
		.configure = serial_usb_android_configure,
		.read = serial_usb_android_read,
		.write = serial_usb_android_write,
		.purge = serial_usb_android_purge,
		.sleep = serial_usb_android_sleep, /* sleep */
		.close = serial_usb_android_close, /* close */
	};

	QAndroidJniObject localdevice = QAndroidJniObject::callStaticObjectMethod("org/subsurfacedivelog/mobile/AndroidSerial",
										  "open_android_serial",
										  "(Landroid/hardware/usb/UsbDevice;Ljava/lang/String;)Lorg/subsurfacedivelog/mobile/AndroidSerial;",
										  usbDevice.object<jobject>(),
										  QAndroidJniObject::fromString(driverClassName.c_str()).object());
	if (localdevice == nullptr)
		return DC_STATUS_IO;

	QAndroidJniObject *device = new QAndroidJniObject(localdevice);
	TRACE(device->contxt, "%s", "calling dc_custom_open())");
	return dc_custom_open(iostream, context, DC_TRANSPORT_SERIAL, &callbacks, device);
}

static void guessVendorProduct(android_usb_serial_device_descriptor &descriptor)
{
	// for a couple of devices we get enough information that we can guess which dive computer this is
	QString product = QString::fromStdString(descriptor.usbProduct);
	int vid = descriptor.vid;
	int pid = descriptor.pid;

	if (product.contains("HeinrichsWeikamp OSTC3")) {
		descriptor.manufacturer = "Heinrichs Weikamp";
		descriptor.product = "OSTC 3";
	} else if (product.contains("HeinrichsWeikamp OSTC 2N")) {
		descriptor.manufacturer = "Heinrichs Weikamp";
		descriptor.product = "OSTC 2N";
	} else if (vid == 0x0403 && pid == 0xf460) {
		// some form of Oceanic
		descriptor.manufacturer = "Oceanic";
	} else if (vid == 0x0403 && pid == 0xf680) {
		// some form of Suunto
		descriptor.manufacturer = "Suunto";
	} else if (vid == 0x0403 && pid == 0x87d0) {
		// some form of Cressi
		descriptor.manufacturer = "Cressi";
	} else if (vid == 0xffff && pid == 0x0005) {
		// Mares Icon HD
		descriptor.manufacturer = "Mares";
		descriptor.product = "Icon HD";
	}
}

android_usb_serial_device_descriptor getDescriptor(QAndroidJniObject usbDevice)
{
	QAndroidJniEnvironment env;

	android_usb_serial_device_descriptor descriptor;

	descriptor.usbDevice = usbDevice;
	descriptor.pid = usbDevice.callMethod<jint>("getProductId");
	descriptor.vid = usbDevice.callMethod<jint>("getVendorId");

	// descriptor.manufacturer = UsbDevice.getManufacturerName();
	QAndroidJniObject usbManufacturerName = usbDevice.callObjectMethod<jstring>("getManufacturerName");
	if (usbManufacturerName.isValid()) {
		const char *charArray = env->GetStringUTFChars(usbManufacturerName.object<jstring>(), nullptr);
		descriptor.usbManufacturer = std::string(charArray);
		env->ReleaseStringUTFChars(usbManufacturerName.object<jstring>(), charArray);
	}

	// descriptor.product = UsbDevice.getProductName();
	QAndroidJniObject usbProductName = usbDevice.callObjectMethod<jstring>("getProductName");
	if (usbManufacturerName.isValid()) {
		const char *charArray = env->GetStringUTFChars(usbProductName.object<jstring>(), nullptr);
		descriptor.usbProduct = std::string(charArray);
		env->ReleaseStringUTFChars(usbProductName.object<jstring>(), charArray);
	}

	// guess the actual manufacturer / product if we happen to be able to guess
	guessVendorProduct(descriptor);

	// Get busnum and portnum
	QAndroidJniObject usbDeviceNameString = usbDevice.callObjectMethod<jstring>("getDeviceName");
	const char *charArray = env->GetStringUTFChars(usbDeviceNameString.object<jstring>(), nullptr);
	QRegularExpression reg("/dev/bus/usb/(\\d*)/(\\d*)");
	QRegularExpressionMatch match = reg.match(charArray);
	int busnum = match.captured(1).toInt();
	int portnum = match.captured(2).toInt();
	env->ReleaseStringUTFChars(usbDeviceNameString.object<jstring>(), charArray);

	// The ui representation
	char buffer[128];
	if (!descriptor.manufacturer.empty()) {
		// Heinrichs Weikamp is the longest, so let's just take the name
		sprintf(buffer, "%s [%i:%i]", descriptor.manufacturer.c_str(), busnum, portnum);
	} else if (descriptor.usbManufacturer.empty()) {
		sprintf(buffer, "USB Device [%i:%i]", busnum, portnum);
	} else if (descriptor.usbManufacturer.size() <= 16) {
		sprintf(buffer, "%s [%i:%i]", descriptor.usbManufacturer.c_str(), busnum, portnum);
	} else {
		sprintf(buffer, "%.16s… [%i:%i]", descriptor.usbManufacturer.c_str(), busnum, portnum);
	}
	descriptor.uiRepresentation = buffer;

	return descriptor;
}

std::vector<android_usb_serial_device_descriptor> serial_usb_android_get_devices(book driverSelection)
{
	std::vector<std::string> driverNames;
	if (driverSelection)
		driverNames = { "", "CdcAcmSerialDriver", "Ch34xSerialDriver", "Cp21xxSerialDriver", "FtdiSerialDriver", "ProlificSerialDriver" };
	else
		driverNames = {""};

	// Get the current main activity of the application.
	QAndroidJniObject activity = QtAndroid::androidActivity();
	QAndroidJniObject usb_service = QAndroidJniObject::fromString("usb");
	QAndroidJniEnvironment env;

	// Get UsbManager from activity
	QAndroidJniObject usbManager = activity.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", QAndroidJniObject::fromString("usb").object());

	//UsbDevice[] arrayOfDevices = usbManager.getDeviceList().values().toArray();
	QAndroidJniObject deviceListHashMap = usbManager.callObjectMethod("getDeviceList","()Ljava/util/HashMap;");
	QAndroidJniObject deviceListCollection = deviceListHashMap.callObjectMethod("values", "()Ljava/util/Collection;");
	jint numDevices = deviceListCollection.callMethod<jint>("size");
	QAndroidJniObject arrayOfDevices = deviceListCollection.callObjectMethod("toArray", "()[Ljava/lang/Object;");

	// Special case to keep a generic user-facing name if only one device is present.
	if (numDevices == 1 && !driverSelection) {
		// UsbDevice usbDevice = arrayOfDevices[0]
		jobject value = env->GetObjectArrayElement(arrayOfDevices.object<jobjectArray>(), 0);
		QAndroidJniObject usbDevice(value);
		android_usb_serial_device_descriptor descriptor = getDescriptor(usbDevice);
		descriptor.uiRepresentation = "USB Connection";

		return {descriptor};
	} else {
		std::vector<android_usb_serial_device_descriptor> retval;
		for (int i = 0; i < numDevices ; i++) {
			// UsbDevice usbDevice = arrayOfDevices[i]
			jobject value = env->GetObjectArrayElement(arrayOfDevices.object<jobjectArray>(), i);
			QAndroidJniObject usbDevice(value);

			for (std::string driverName : driverNames) {
				android_usb_serial_device_descriptor descriptor = getDescriptor(usbDevice);

				descriptor.className = driverName;
				if (driverName != "")
					descriptor.uiRepresentation += " (" + driverName + ")";
				else
					descriptor.uiRepresentation += " (autoselect driver)";
				retval.push_back(descriptor);
			}
		}
		return retval;
	}
}

/*
 * For testing and compatibility only, can be removed after the UI changes. Behaves exactly like the "old"
 * implementation if only one device is attached.
 */
dc_status_t serial_usb_android_open(dc_iostream_t **iostream, dc_context_t *context)
{
	// Testing of the method only!
	{
		TRACE(device->contxt, "List of devices with specific drivers:");
		      std::vector<android_usb_serial_device_descriptor> devices = serial_usb_android_get_devices(true);
		for (auto device : devices) {
			TRACE(device->contxt,
			      "USB Device: uiRepresentation=%s, className=%s, manufacturer=%s, product=%s, pid=%i, vid=%i",
			      device.uiRepresentation.c_str(), device.className.c_str(), device.manufacturer.c_str(),
			      device.product.c_str(), device.pid, device.vid);
		}

		TRACE(device->contxt, "List of devices simple:");
		devices = serial_usb_android_get_devices(false);
		for (auto device : devices) {
			TRACE(device->contxt,
			      "USB Device: uiRepresentation=%s, className=%s, manufacturer=%s, product=%s, pid=%i, vid=%i",
			      device.uiRepresentation.c_str(), device.className.c_str(), device.manufacturer.c_str(),
			      device.product.c_str(), device.pid, device.vid);
		}
	}

	std::vector<android_usb_serial_device_descriptor> devices = serial_usb_android_get_devices(false);

	if(devices.empty())
		return DC_STATUS_NODEVICE;

	return serial_usb_android_open(iostream, context, devices[0].usbDevice, devices[0].className);

}
