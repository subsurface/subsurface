/* implements Android specific functions */
#include "dive.h"
#include "display.h"
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <libusb.h>
#include <errno.h>

#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroid>

#define FTDI_VID 0x0403
#define USB_SERVICE "usb"

extern "C" {

const char android_system_divelist_default_font[] = "Roboto";
const char *system_divelist_default_font = android_system_divelist_default_font;
double system_divelist_default_font_size = -1;

int get_usb_fd(uint16_t idVendor, uint16_t idProduct);
void subsurface_OS_pref_setup(void)
{
	// Abusing this function to get a decent place where we can wire in
	// our open callback into libusb
#ifdef libusb_android_open_callback_func
	libusb_set_android_open_callback(get_usb_fd);
#elif __ANDROID__
#error we need libusb_android_open_callback
#endif
}

bool subsurface_ignore_font(const char *font)
{
	// there are no old default fonts that we would want to ignore
	return false;
}

void subsurface_user_info(struct user_info *user)
{ /* Encourage use of at least libgit2-0.20 */ }

static const char *system_default_path_append(const char *append)
{
	// Qt appears to find a working path for us - let's just go with that
	QString path = QStandardPaths::standardLocations(QStandardPaths::DataLocation).first();

	if (append)
		path += QString("/%1").arg(append);

	return strdup(path.toUtf8().data());
}

const char *system_default_directory(void)
{
	static const char *path = NULL;
	if (!path)
		path = system_default_path_append(NULL);
	return path;
}

const char *system_default_filename(void)
{
	static const char *filename = "subsurface.xml";
	static const char *path = NULL;
	if (!path)
		path = system_default_path_append(filename);
	return path;
}

int enumerate_devices(device_callback_t callback, void *userdata, int dc_type)
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
		if(vendorid == idVendor && productid == idProduct) // Found the requested device
			break;
	}
	if (i == num_devices) {
		// No device found.
		errno = ENOENT;
		return -1;
	}

	jboolean hasPermission = usbManager.callMethod<jboolean>("hasPermission", "(Landroid/hardware/usb/UsbDevice;)Z", usbDevice.object());
	if (!hasPermission) {
		// You do not have permission to use the usbDevice.
		// Please remove and reinsert the USB device.
		// Could also give an dialogbox asking for permission.
		errno = EPERM;
		return -1;
	}

	// An device is present and we also have permission to use the device.
	// Open the device and get its file descriptor.
	QAndroidJniObject usbDeviceConnection = usbManager.callObjectMethod("openDevice", "(Landroid/hardware/usb/UsbDevice;)Landroid/hardware/usb/UsbDeviceConnection;", usbDevice.object());
	if (usbDeviceConnection.object() == NULL) {
		// Some error occurred while opening the device. Exit.
		errno = EINVAL;
		return -1;
	}

	// Finally get the required file descriptor.
	fd = usbDeviceConnection.callMethod<jint>("getFileDescriptor", "()I");
	if (fd == -1) {
		// The device is not opened. Some error.
		errno = ENODEV;
		return -1;
	}
	return fd;
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

struct zip *subsurface_zip_open_readonly(const char *path, int flags, int *errorp)
{
	return zip_open(path, flags, errorp);
}

int subsurface_zip_close(struct zip *zip)
{
	return zip_close(zip);
}

/* win32 console */
void subsurface_console_init(bool dedicated)
{
	/* NOP */
}

void subsurface_console_exit(void)
{
	/* NOP */
}
}
