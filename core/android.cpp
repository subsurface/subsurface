// SPDX-License-Identifier: GPL-2.0
/* implements Android specific functions */
#include "device.h"
#include "libdivecomputer.h"
#include "file.h"
#include "qthelper.h"
#include "subsurfacestartup.h"
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <zip.h>
#include <string>

#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroid>
#include <QDebug>
#include <core/serial_usb_android.h>

#if defined(SUBSURFACE_MOBILE)
#include "mobile-widgets/qmlmanager.h"
#define LOG(x) QMLManager::instance()->appendTextToLog(x);
#else
#define LOG(x) qDebug() << x;
#endif

#define USB_SERVICE "usb"

static std::string system_default_path()
{
	// Qt appears to find a working path for us - let's just go with that
	// AppDataLocation allows potential sharing of the files we put there
	return QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first().toStdString();
}

static std::string make_default_filename()
{
	return system_default_path() + "/subsurface.xml";
}

using namespace std::string_literals;
std::string system_divelist_default_font = "Roboto"s;
double system_divelist_default_font_size = -1.0;

int get_usb_fd(uint16_t idVendor, uint16_t idProduct);
void subsurface_OS_pref_setup()
{
}

bool subsurface_ignore_font(const std::string &font)
{
	// there are no old default fonts that we would want to ignore
	return false;
}

std::string system_default_directory()
{
	static const std::string path = system_default_path();
	return path;
}

std::string system_default_filename()
{
	static const std::string fn = make_default_filename();
	return fn;
}


int enumerate_devices(device_callback_t callback, void *userdata, unsigned int transport)
{
	/* FIXME: we need to enumerate in some other way on android */
	/* qtserialport maybee? */
	return -1;
}

/**
 * Get the file descriptor of first available matching device attached to usb in android.
 *
 * returns a fd to the device, or -1 and errno is set.
 */
int get_usb_fd(uint16_t idVendor, uint16_t idProduct)
{
	int i;
	jint fd, vendorid, productid;
	QAndroidJniObject usbName, usbDevice;

	// Get the current main activity of the application.
	QAndroidJniObject activity = QtAndroid::androidActivity();

	QAndroidJniObject usb_service = QAndroidJniObject::fromString(USB_SERVICE);

	// Get UsbManager from activity
	QAndroidJniObject usbManager = activity.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", usb_service.object());

	// Get a HashMap<Name, UsbDevice> of all USB devices attached to Android
	QAndroidJniObject deviceMap = usbManager.callObjectMethod("getDeviceList", "()Ljava/util/HashMap;");
	jint num_devices = deviceMap.callMethod<jint>("size", "()I");
	if (num_devices == 0) {
		// No USB device is attached.
		LOG("usbManager says no devices attached");
		return -1;
	}

	// Iterate over all the devices and find the first available FTDI device.
	QAndroidJniObject keySet = deviceMap.callObjectMethod("keySet", "()Ljava/util/Set;");
	QAndroidJniObject iterator = keySet.callObjectMethod("iterator", "()Ljava/util/Iterator;");

	for (i = 0; i < num_devices; i++) {
		usbName = iterator.callObjectMethod("next", "()Ljava/lang/Object;");
		usbDevice = deviceMap.callObjectMethod ("get", "(Ljava/lang/Object;)Ljava/lang/Object;", usbName.object());
		vendorid = usbDevice.callMethod<jint>("getVendorId", "()I");
		productid = usbDevice.callMethod<jint>("getProductId", "()I");
		LOG(QString("Looking at device with VID/PID %1/%2").arg(vendorid).arg(productid));
		if(vendorid == idVendor && productid == idProduct) // Found the requested device
			break;
	}
	if (i == num_devices) {
		// No device found.
		LOG(QString("Didn't find device matching %1/%2").arg(idVendor).arg(idProduct))
		errno = ENOENT;
		return -1;
	}

	jboolean hasPermission = usbManager.callMethod<jboolean>("hasPermission", "(Landroid/hardware/usb/UsbDevice;)Z", usbDevice.object());
	if (!hasPermission) {
		// You do not have permission to use the usbDevice.
		// Please remove and reinsert the USB device.
		// Could also give an dialogbox asking for permission.
		LOG("usbManager tells us we don't have permission to access this device");
		errno = EPERM;
		return -1;
	}

	// An device is present and we also have permission to use the device.
	// Open the device and get its file descriptor.
	QAndroidJniObject usbDeviceConnection = usbManager.callObjectMethod("openDevice", "(Landroid/hardware/usb/UsbDevice;)Landroid/hardware/usb/UsbDeviceConnection;", usbDevice.object());
	if (usbDeviceConnection.object() == NULL) {
		// Some error occurred while opening the device. Exit.
		LOG("usbManager said we had permission to access, but then opening the device failed");
		errno = EINVAL;
		return -1;
	}

	// Finally get the required file descriptor.
	fd = usbDeviceConnection.callMethod<jint>("getFileDescriptor", "()I");
	if (fd == -1) {
		// The device is not opened. Some error.
		LOG("usbManager said we successfully opened the device, but the fd was -1");
		errno = ENODEV;
		return -1;
	}
	return fd;
}

JNIEXPORT void JNICALL
Java_org_subsurfacedivelog_mobile_SubsurfaceMobileActivity_setUsbDevice(JNIEnv *,
	jobject,
	jobject javaUsbDevice)
{
	QAndroidJniObject usbDevice(javaUsbDevice);
	if (usbDevice.isValid()) {
		android_usb_serial_device_descriptor descriptor = getDescriptor(usbDevice);

		LOG(QString("called by connect intent for device %1").arg(QString::fromStdString(descriptor.uiRepresentation)));
	}
#if defined(SUBSURFACE_MOBILE)
	QMLManager::instance()->showDownloadPage(usbDevice);
#endif
	return;
}

JNIEXPORT void JNICALL
Java_org_subsurfacedivelog_mobile_SubsurfaceMobileActivity_restartDownload(JNIEnv *,
	jobject,
	jobject javaUsbDevice)
{
	QAndroidJniObject usbDevice(javaUsbDevice);
	if (usbDevice.isValid()) {
		android_usb_serial_device_descriptor descriptor = getDescriptor(usbDevice);

		LOG(QString("called by permission granted intent for device %1").arg(QString::fromStdString(descriptor.uiRepresentation)));
	}
#if defined(SUBSURFACE_MOBILE)
	QMLManager::instance()->restartDownload(usbDevice);
#endif
	return;
}

/* NOP wrappers to comform with windows.c */
int subsurface_rename(const char *path, const char *newpath)
{
	return rename(path, newpath);
}

int subsurface_open(const char *path, int oflags, mode_t mode)
{
	return open(path, oflags, mode);
}

FILE *subsurface_fopen(const char *path, const char *mode)
{
	return fopen(path, mode);
}

void *subsurface_opendir(const char *path)
{
	return (void *)opendir(path);
}

int subsurface_access(const char *path, int mode)
{
	return access(path, mode);
}

int subsurface_stat(const char* path, struct stat* buf)
{
	return stat(path, buf);
}

struct zip *subsurface_zip_open_readonly(const char *path, int flags, int *errorp)
{
	return zip_open(path, flags, errorp);
}

int subsurface_zip_close(struct zip *zip)
{
	return zip_close(zip);
}

/* win32 console */
void subsurface_console_init()
{
	/* NOP */
}

void subsurface_console_exit()
{
	/* NOP */
}

bool subsurface_user_is_root()
{
	return false;
}

/* called from QML manager */
void checkPendingIntents()
{
	QAndroidJniObject activity = QtAndroid::androidActivity();
	if (activity.isValid()) {
		activity.callMethod<void>("checkPendingIntents");
		qDebug() << "checkPendingIntents ";
		return;
	}
	qDebug() << "checkPendingIntents: Activity not valid";
}

QString getAndroidHWInfo()
{
	return QStringLiteral("%1/%2/%3")
			.arg(QAndroidJniObject::getStaticObjectField<jstring>("android/os/Build", "MODEL").toString())
			.arg(QAndroidJniObject::getStaticObjectField<jstring>("android/os/Build", "BRAND").toString())
			.arg(QAndroidJniObject::getStaticObjectField<jstring>("android/os/Build", "PRODUCT").toString());
}
