#include "libdivecomputer.h"
#include <libdivecomputer/context.h>
#include <libdivecomputer/custom.h>

#include <cstring>

#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>

#include <thread>

#include <android/log.h>

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

static dc_status_t
serial_usb_android_configure(void *io, unsigned int baudrate, unsigned int databits, dc_parity_t parity,
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
	TRACE (device->context, "%s: size: %i", __FUNCTION__, size);

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
	TRACE (device->context, "%s: size: %i", __FUNCTION__, size);

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

dc_status_t serial_usb_android_open(dc_iostream_t **iostream, dc_context_t *context)
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
										  "()Lorg/subsurfacedivelog/mobile/AndroidSerial;");
	if (localdevice == nullptr) {
		return DC_STATUS_IO;
	}
	QAndroidJniObject *device = new QAndroidJniObject(localdevice);
	TRACE(device->contxt, "%s", "calling dc_custom_open())");
	return dc_custom_open(iostream, context, DC_TRANSPORT_SERIAL, &callbacks, device);
}
