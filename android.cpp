/* implements Android specific functions */
#include "dive.h"
#include "display.h"
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>

extern "C" {

const char system_divelist_default_font[] = "Roboto";
const int system_divelist_default_font_size = 8;

const char *system_default_filename(void)
{
	/* Replace this when QtCore/QStandardPaths getExternalStorageDirectory landed */
	QAndroidJniObject externalStorage = QAndroidJniObject::callStaticObjectMethod("android/os/Environment", "getExternalStorageDirectory", "()Ljava/io/File;");
	QAndroidJniObject externalStorageAbsolute = externalStorage.callObjectMethod( "getAbsolutePath", "()Ljava/lang/String;" );
	QString system_default_filename = externalStorageAbsolute.toString()+"/subsurface.xml";
	QAndroidJniEnvironment env;
	if (env->ExceptionCheck()) {
		// FIXME: Handle exception here.
		env->ExceptionClear();
		return strdup("/sdcard/subsurface.xml");
	}
	return strdup(system_default_filename.toUtf8().data());
}

int enumerate_devices (device_callback_t callback, void *userdata)
{
	/* FIXME: we need to enumerate in some other way on android */
	/* qtserialport maybee? */
	return -1;
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
