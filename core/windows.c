// SPDX-License-Identifier: GPL-2.0
/* windows.c */
/* implements Windows specific functions */
#include "ssrf.h"
#include <io.h>
#include "dive.h"
#include "display.h"
#include "file.h"
#include "errorhelper.h"
#include "subsurfacesysinfo.h"
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x500
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <dirent.h>
#include <zip.h>
#include <lmcons.h>

const char non_standard_system_divelist_default_font[] = "Calibri";
const char current_system_divelist_default_font[] = "Segoe UI";
const char *system_divelist_default_font = non_standard_system_divelist_default_font;
double system_divelist_default_font_size = -1;

void subsurface_OS_pref_setup(void)
{
	if (isWin7Or8())
		system_divelist_default_font = current_system_divelist_default_font;
}

bool subsurface_ignore_font(const char *font)
{
	// if this is running on a recent enough version of Windows and the font
	// passed in is the pre 4.3 default font, ignore it
	if (isWin7Or8() && strcmp(font, non_standard_system_divelist_default_font) == 0)
		return true;
	return false;
}

/* this function converts a win32's utf-16 2 byte string to utf-8.
 * the caller function should manage the allocated memory.
 */
static char *utf16_to_utf8_fl(const wchar_t *utf16, char *file, int line)
{
	assert(utf16 != NULL);
	assert(file != NULL);
	assert(line);
	/* estimate buffer size */
	const int sz = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, NULL, 0, NULL, NULL);
	if (!sz) {
		fprintf(stderr, "%s:%d: cannot estimate buffer size\n", file, line);
		return NULL;
	}
	char *utf8 = (char *)malloc(sz);
	if (!utf8) {
		fprintf(stderr, "%s:%d: cannot allocate buffer of size: %d\n", file, line, sz);
		return NULL;
	}
	if (WideCharToMultiByte(CP_UTF8, 0, utf16, -1, utf8, sz, NULL, NULL)) {
		return utf8;
	}
	fprintf(stderr, "%s:%d: cannot convert string\n", file, line);
	free((void *)utf8);
	return NULL;
}

#define utf16_to_utf8(s) utf16_to_utf8_fl(s, __FILE__, __LINE__)

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
		fprintf(stderr, "%s:%d: cannot allocate buffer of size: %d\n", file, line, sz);
		return NULL;
	}
	if (MultiByteToWideChar(CP_UTF8, 0, utf8, -1, utf16, sz))
		return utf16;
	fprintf(stderr, "%s:%d: cannot convert string\n", file, line);
	free((void *)utf16);
	return NULL;
}

#define utf8_to_utf16(s) utf8_to_utf16_fl(s, __FILE__, __LINE__)

/* this function returns the Win32 Roaming path for the current user as UTF-8.
 * it never returns NULL but fallsback to .\ instead!
 * the append argument will append a wchar_t string to the end of the path.
 */
static wchar_t *system_default_path_append(const wchar_t *append)
{
	wchar_t wpath[MAX_PATH] = { 0 };
	const char *fname = "system_default_path_append()";

	/* obtain the user path via SHGetFolderPathW.
	 * this API is deprecated but still supported on modern Win32.
	 * fallback to .\ if it fails.
	 */
	if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, wpath))) {
		fprintf(stderr, "%s: cannot obtain path!\n", fname);
		wpath[0] = L'.';
		wpath[1] = L'\0';
	}

	wcscat(wpath, L"\\Subsurface");
	if (append) {
		wcscat(wpath, L"\\");
		wcscat(wpath, append);
	}

	wchar_t *result = wcsdup(wpath);
	if (!result)
		fprintf(stderr, "%s: cannot allocate memory for path!\n", fname);
	return result;
}

/* by passing NULL to system_default_path_append() we obtain the pure path.
 * '\' not included at the end.
 */
const char *system_default_directory(void)
{
	static const char *path = NULL;
	if (!path) {
		wchar_t *wpath = system_default_path_append(NULL);
		path = utf16_to_utf8(wpath);
		free((void *)wpath);
	}
	return path;
}

/* obtain the Roaming path and append "\\<USERNAME>.xml" to it.
 */
const char *system_default_filename(void)
{
	static const char *path = NULL;
	if (!path) {
		wchar_t username[UNLEN + 1] = { 0 };
		DWORD username_len = UNLEN + 1;
		GetUserNameW(username, &username_len);
		wchar_t filename[UNLEN + 5] = { 0 };
		wcscat(filename, username);
		wcscat(filename, L".xml");
		wchar_t *wpath = system_default_path_append(filename);
		path = utf16_to_utf8(wpath);
		free((void *)wpath);
	}
	return path;
}

int enumerate_devices(device_callback_t callback, void *userdata, unsigned int transport)
{
	int index = -1;
	DWORD i;
	if (transport & DC_TRANSPORT_SERIAL) {
		// Open the registry key.
		HKEY hKey;
		LONG rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_QUERY_VALUE, &hKey);
		if (rc != ERROR_SUCCESS) {
			return -1;
		}

		// Get the number of values.
		DWORD count = 0;
		rc = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &count, NULL, NULL, NULL, NULL);
		if (rc != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			return -1;
		}
		for (i = 0; i < count; ++i) {
			// Get the value name, data and type.
			char name[512], data[512];
			DWORD name_len = sizeof(name);
			DWORD data_len = sizeof(data);
			DWORD type = 0;
			rc = RegEnumValue(hKey, i, name, &name_len, NULL, &type, (LPBYTE)data, &data_len);
			if (rc != ERROR_SUCCESS) {
				RegCloseKey(hKey);
				return -1;
			}

			// Ignore non-string values.
			if (type != REG_SZ)
				continue;

			// Prevent a possible buffer overflow.
			if (data_len >= sizeof(data)) {
				RegCloseKey(hKey);
				return -1;
			}

			// Null terminate the string.
			data[data_len] = 0;

			callback(data, userdata);
			index++;
			if (is_default_dive_computer_device(name))
				index = i;
		}

		RegCloseKey(hKey);
	}
	if (transport & DC_TRANSPORT_USBSTORAGE) {
		int i;
		int count_drives = 0;
		const int bufdef = 512;
		const char *dlabels[] = {"UEMISSDA", "GARMIN", NULL};
		char bufname[bufdef], bufval[bufdef], *p;
		DWORD bufname_len;

		/* add drive letters that match labels */
		memset(bufname, 0, bufdef);
		bufname_len = bufdef;
		if (GetLogicalDriveStringsA(bufname_len, bufname)) {
			p = bufname;

			while (*p) {
				memset(bufval, 0, bufdef);
				if (GetVolumeInformationA(p, bufval, bufdef, NULL, NULL, NULL, NULL, 0)) {
					for (i = 0; dlabels[i] != NULL; i++)
						if (!strcmp(bufval, dlabels[i])) {
							char data[512];
							snprintf(data, sizeof(data), "%s (%s)", p, dlabels[i]);
							callback(data, userdata);
							if (is_default_dive_computer_device(p))
								index = count_drives;
							count_drives++;
						}
				}
				p = &p[strlen(p) + 1];
			}
			if (count_drives == 1) /* we found exactly one Uemis "drive" */
				index = 0; /* make it the selected "device" */
		}
	}
	return index;
}

/* bellow we provide a set of wrappers for some I/O functions to use wchar_t.
 * on win32 this solves the issue that we need paths to be utf-16 encoded.
 */
int subsurface_rename(const char *path, const char *newpath)
{
	int ret = -1;
	if (!path || !newpath)
		return ret;

	wchar_t *wpath = utf8_to_utf16(path);
	wchar_t *wnewpath = utf8_to_utf16(newpath);

	if (wpath && wnewpath)
		ret = _wrename(wpath, wnewpath);
	free((void *)wpath);
	free((void *)wnewpath);
	return ret;
}

// if the QDir based rename fails, we try this one
int subsurface_dir_rename(const char *path, const char *newpath)
{
	// check if the folder exists
	BOOL exists = FALSE;
	DWORD attrib = GetFileAttributes(path);
	if (attrib != INVALID_FILE_ATTRIBUTES && attrib & FILE_ATTRIBUTE_DIRECTORY)
		exists = TRUE;
	if (!exists && verbose) {
		fprintf(stderr, "folder not found or path is not a folder: %s\n", path);
		return EXIT_FAILURE;
	}

	// list of error codes:
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms681381(v=vs.85).aspx
	DWORD errorCode;

	// if this fails something has already obatained (more) exclusive access to the folder
	HANDLE h = CreateFile(path, GENERIC_WRITE, FILE_SHARE_WRITE |
			      FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	if (h == INVALID_HANDLE_VALUE) {
		errorCode = GetLastError();
		if (verbose)
			fprintf(stderr, "cannot obtain exclusive write access for folder: %u\n", (unsigned int)errorCode );
		return EXIT_FAILURE;
	} else {
		if (verbose)
			fprintf(stderr, "exclusive write access obtained...closing handle!");
		CloseHandle(h);

		// attempt to rename
		BOOL result = MoveFile(path, newpath);
		if (!result) {
			errorCode = GetLastError();
			if (verbose)
				fprintf(stderr, "rename failed: %u\n", (unsigned int)errorCode);
			return EXIT_FAILURE;
		}
		if (verbose > 1)
			fprintf(stderr, "folder rename success: %s ---> %s\n", path, newpath);
	}
	return EXIT_SUCCESS;
}

int subsurface_open(const char *path, int oflags, mode_t mode)
{
	int ret = -1;
	if (!path)
		return ret;
	wchar_t *wpath = utf8_to_utf16(path);
	if (wpath)
		ret = _wopen(wpath, oflags, mode);
	free((void *)wpath);
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
	}
	free((void *)wpath);
	return ret;
}

/* here we return a void pointer instead of _WDIR or DIR pointer */
void *subsurface_opendir(const char *path)
{
	_WDIR *ret = NULL;
	if (!path)
		return ret;
	wchar_t *wpath = utf8_to_utf16(path);
	if (wpath)
		ret = _wopendir(wpath);
	free((void *)wpath);
	return (void *)ret;
}

int subsurface_access(const char *path, int mode)
{
	int ret = -1;
	if (!path)
		return ret;
	wchar_t *wpath = utf8_to_utf16(path);
	if (wpath)
		ret = _waccess(wpath, mode);
	free((void *)wpath);
	return ret;
}

int subsurface_stat(const char* path, struct stat* buf)
{
	int ret = -1;
	if (!path)
		return ret;
	wchar_t *wpath = utf8_to_utf16(path);
	if (wpath)
		ret = wstat(wpath, buf);
	free((void *)wpath);
	return ret;
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

/* win32 console */
static struct {
	bool allocated;
	UINT cp;
	FILE *out, *err;
} console_desc;

void subsurface_console_init(void)
{
	UNUSED(console_desc);
	/* if this is a console app already, do nothing */
#ifndef WIN32_CONSOLE_APP

	/* just in case of multiple calls */
	memset((void *)&console_desc, 0, sizeof(console_desc));

	/* if AttachConsole(ATTACH_PARENT_PROCESS) returns true the parent process
	 * is a terminal. based on the result, either redirect to that terminal or
	 * to log files.
	 */
	console_desc.allocated = AttachConsole(ATTACH_PARENT_PROCESS);
	if (console_desc.allocated) {
		console_desc.cp = GetConsoleCP();
		SetConsoleOutputCP(CP_UTF8); /* make the ouput utf8 */
		console_desc.out = freopen("CON", "w", stdout);
		console_desc.err = freopen("CON", "w", stderr);
	} else {
		verbose = 1; /* set the verbose level to '1' */
		wchar_t *wpath_out = system_default_path_append(L"subsurface_out.log");
		wchar_t *wpath_err = system_default_path_append(L"subsurface_err.log");
		if (wpath_out && wpath_err) {
			console_desc.out = _wfreopen(wpath_out, L"w", stdout);
			console_desc.err = _wfreopen(wpath_err, L"w", stderr);
		}
		free((void *)wpath_out);
		free((void *)wpath_err);
	}

	puts(""); /* add an empty line */
#endif
}

void subsurface_console_exit(void)
{
#ifndef WIN32_CONSOLE_APP
	/* close handles */
	if (console_desc.out)
		fclose(console_desc.out);
	if (console_desc.err)
		fclose(console_desc.err);
	/* reset code page and free */
	if (console_desc.allocated) {
		SetConsoleOutputCP(console_desc.cp);
		FreeConsole();
	}
#endif
}

bool subsurface_user_is_root()
{
	/* FIXME: Detect admin rights */
	return false;
}
