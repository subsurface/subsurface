/* windows.c */
/* implements Windows specific functions */
#include "dive.h"
#include "display.h"
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <dirent.h>
#include <zip.h>

const char system_divelist_default_font[] = "Sans 8";

const char *system_default_filename(void)
{
	char datapath[MAX_PATH];
	const char *user;
	char *buffer;
	int len;

	/* I don't think this works on Windows */
	user = getenv("USERNAME");
	if (! SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, datapath))) {
		datapath[0] = '.';
		datapath[1] = '\0';
	}
	len = strlen(datapath) + strlen(user) + 17;
	buffer = malloc(len);
	snprintf(buffer, len, "%s\\Subsurface\\%s.xml", datapath, user);
	return buffer;
}

int enumerate_devices (device_callback_t callback, void *userdata)
{
	// Open the registry key.
	HKEY hKey;
	int index = -1;
	LONG rc = RegOpenKeyEx (HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_QUERY_VALUE, &hKey);
	if (rc != ERROR_SUCCESS) {
		return -1;
	}

	// Get the number of values.
	DWORD count = 0;
	rc = RegQueryInfoKey (hKey, NULL, NULL, NULL, NULL, NULL, NULL, &count, NULL, NULL, NULL, NULL);
	if (rc != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return -1;
	}
	DWORD i;
	for (i = 0; i < count; ++i) {
		// Get the value name, data and type.
		char name[512], data[512];
		DWORD name_len = sizeof (name);
		DWORD data_len = sizeof (data);
		DWORD type = 0;
		rc = RegEnumValue (hKey, i, name, &name_len, NULL, &type, (LPBYTE) data, &data_len);
		if (rc != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			return -1;
		}

		// Ignore non-string values.
		if (type != REG_SZ)
			continue;

		// Prevent a possible buffer overflow.
		if (data_len >= sizeof (data)) {
			RegCloseKey(hKey);
			return -1;
		}

		// Null terminate the string.
		data[data_len] = 0;

		callback (data, userdata);
		index++;
		if (is_default_dive_computer_device(name))
			index = i;
	}

	RegCloseKey(hKey);
	return index;
}

/* this function converts a utf-8 string to win32's utf-16 2 byte string.
 * the caller function should manage the allocated memory.
 */
static wchar_t *utf8_to_utf16_fl(const char *utf8, char *file, int line)
{
	assert(utf8 != NULL);
	assert(file != NULL);
	assert(line);
	/* estimate buffer size */
	const int sz = strlen(utf8) + 1;
	wchar_t *utf16 = (wchar_t *)malloc(sizeof(wchar_t) * sz);
	if (!utf16) {
		fprintf(stderr, "%s:%d: %s %d.", file, line, "cannot allocate buffer of size", sz);
		return NULL;
	}
	if (MultiByteToWideChar(CP_UTF8, 0, utf8, -1, utf16, sz))
		return utf16;
	fprintf(stderr, "%s:%d: %s", file, line, "cannot convert string.");
	free((void *)utf16);
	return NULL;
}

#define utf8_to_utf16(s) utf8_to_utf16_fl(s, __FILE__, __LINE__)

/* bellow we provide a set of wrappers for some I/O functions to use wchar_t.
 * on win32 this solves the issue that we need paths to be utf-16 encoded.
 */
int subsurface_rename(const char *path, const char *newpath)
{
	int ret = -1;
	if (!path || !newpath)
		return -1;

	wchar_t *wpath = utf8_to_utf16(path);
	wchar_t *wnewpath = utf8_to_utf16(newpath);

	if (wpath && wnewpath)
		ret = _wrename(wpath, wnewpath);
	free((void *)wpath);
	free((void *)wnewpath);
	return ret;
}

int subsurface_open(const char *path, int oflags, mode_t mode)
{
	int ret = -1;
	if (!path)
		return -1;
	wchar_t *wpath = utf8_to_utf16(path);
	if (wpath) {
		ret = _wopen(wpath, oflags, mode);
		free((void *)wpath);
		return ret;
	}
	return ret;
}

FILE *subsurface_fopen(const char *path, const char *mode)
{
	FILE *ret = NULL;
	if (!path)
		return ret;
	wchar_t *wpath = utf8_to_utf16(path);
	if (wpath) {
		const int len = strlen(mode);
		wchar_t wmode[len + 1];
		for (int i = 0; i < len; i++)
			wmode[i] = (wchar_t)mode[i];
		wmode[len] = 0;
		ret = _wfopen(wpath, wmode);
		free((void *)wpath);
		return ret;
	}
	return ret;
}

/* here we return a void pointer instead of _WDIR or DIR pointer */
void *subsurface_opendir(const char *path)
{
	_WDIR *ret = NULL;
	if (!path)
		return ret;
	wchar_t *wpath = utf8_to_utf16(path);
	if (wpath) {
		ret = _wopendir(wpath);
		free((void *)wpath);
		return (void *)ret;
	}
	return (void *)ret;
}

#ifndef O_BINARY
#define O_BINARY 0
#endif

struct zip *subsurface_zip_open_readonly(const char *path, int flags, int *errorp)
{
#if defined(LIBZIP_VERSION_MAJOR)
	/* libzip 0.10 has zip_fdopen, let's use it since zip_open doesn't have a
	 * wchar_t version */
	int fd = subsurface_open(path, O_RDONLY | O_BINARY, 0);
	struct zip *ret = zip_fdopen(fd, flags, errorp);
	if (!ret)
		close(fd);
	return ret;
#else
	return zip_open(path, flags, errorp);
#endif
}

int subsurface_zip_close(struct zip *zip)
{
	return zip_close(zip);
}
