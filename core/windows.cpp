// SPDX-License-Identifier: GPL-2.0
/* windows.c */
/* implements Windows specific functions */
#include "ssrf.h"
#include <io.h>
#include "dive.h"
#include "device.h"
#include "libdivecomputer.h"
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
#include <string>

/* this function converts a win32's utf-16 2 byte string to utf-8.
 * note: the standard library's <codecvt> was deprecated and is in
 * an ominous state, so use the native Windows version for now.
 */
static std::string utf16_to_utf8_fl(const std::wstring &utf16, const char *file, int line)
{
	assert(file != NULL);
	assert(line);
	/* estimate buffer size */
	const int sz = WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), -1, NULL, 0, NULL, NULL);
	if (!sz) {
		report_info("%s:%d: cannot estimate buffer size", file, line);
		return std::string();
	}
	std::string utf8(sz, ' '); // Note: includes the terminating '\0', just in case.
	if (WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), -1, &utf8[0], sz, NULL, NULL)) {
		utf8.resize(sz - 1); // Chop off final '\0' byte
		return utf8;
	}
	report_info("%s:%d: cannot convert string", file, line);
	return std::string();
}

/* this function converts a utf-8 string to win32's utf-16 2 byte string.
 */
static std::wstring utf8_to_utf16_fl(const char *utf8, const char *file, int line)
{
	assert(file != NULL);
	assert(line);
	/* estimate buffer size */
	const int sz = strlen(utf8) + 1;
	std::wstring utf16(sz, ' '); // Note: includes the terminating '\0', just in case.
	int actual_size = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &utf16[0], sz);
	if (actual_size) {
		utf16.resize(actual_size - 1); // Chop off final '\0' character
		return utf16;
	}
	report_info("%s:%d: cannot convert string", file, line);
	return std::wstring();
}

#define utf16_to_utf8(s) utf16_to_utf8_fl(s, __FILE__, __LINE__)

/* this function returns the Win32 Roaming path for the current user as UTF-8.
 * it never returns an empty string but fallsback to .\ instead!
 */
static std::wstring system_default_path()
{
	wchar_t wpath[MAX_PATH] = { 0 };
	const char *fname = "system_default_path()";

	/* obtain the user path via SHGetFolderPathW.
	 * this API is deprecated but still supported on modern Win32.
	 * fallback to .\ if it fails.
	 */
	std::wstring path;
	if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, wpath))) {
		path = wpath;
	} else {
		report_info("%s: cannot obtain path!", fname);
		path = L'.';
	}
	return path + L"\\Subsurface";
}

/* obtain the Roaming path and append "\\<USERNAME>.xml" to it.
 */
static std::wstring make_default_filename()
{
	wchar_t username[UNLEN + 1] = { 0 };
	DWORD username_len = UNLEN + 1;
	GetUserNameW(username, &username_len);
	std::wstring filename = username;
	filename += L".xml";

	std::wstring path = system_default_path();
	return path + L"\\" + filename;
}

extern "C" {

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

#define utf8_to_utf16(s) utf8_to_utf16_fl(s, __FILE__, __LINE__)

/* '\' not included at the end.
 */
const char *system_default_directory(void)
{
	static std::string path = utf16_to_utf8(system_default_path());
	return path.c_str();
}

const char *system_default_filename(void)
{
	static std::string path = utf16_to_utf8(make_default_filename());
	return path.c_str();
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
	if (!path || !newpath)
		return -1;

	std::wstring wpath = utf8_to_utf16(path);
	std::wstring wnewpath = utf8_to_utf16(newpath);

	if (!wpath.empty() && !wnewpath.empty())
		return _wrename(wpath.c_str(), wnewpath.c_str());
	return -1;
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
		report_info("folder not found or path is not a folder: %s", path);
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
			report_info("cannot obtain exclusive write access for folder: %u", (unsigned int)errorCode );
		return EXIT_FAILURE;
	} else {
		if (verbose)
			report_info("exclusive write access obtained...closing handle!");
		CloseHandle(h);

		// attempt to rename
		BOOL result = MoveFile(path, newpath);
		if (!result) {
			errorCode = GetLastError();
			if (verbose)
				report_info("rename failed: %u", (unsigned int)errorCode);
			return EXIT_FAILURE;
		}
		if (verbose > 1)
			report_info("folder rename success: %s ---> %s", path, newpath);
	}
	return EXIT_SUCCESS;
}

int subsurface_open(const char *path, int oflags, mode_t mode)
{
	if (!path)
		return -1;
	std::wstring wpath = utf8_to_utf16(path);
	if (!wpath.empty())
		return _wopen(wpath.c_str(), oflags, mode);
	return -1;
}

FILE *subsurface_fopen(const char *path, const char *mode)
{
	if (!path)
		return NULL;
	std::wstring wpath = utf8_to_utf16(path);
	if (!wpath.empty()) {
		const int len = strlen(mode);
		std::wstring wmode(len, ' ');
		for (int i = 0; i < len; i++)
			wmode[i] = (wchar_t)mode[i];
		return _wfopen(wpath.c_str(), wmode.c_str());
	}
	return NULL;
}

/* here we return a void pointer instead of _WDIR or DIR pointer */
void *subsurface_opendir(const char *path)
{
	if (!path)
		return NULL;
	std::wstring wpath = utf8_to_utf16(path);
	if (!wpath.empty())
		return (void *)_wopendir(wpath.c_str());
	return NULL;
}

int subsurface_access(const char *path, int mode)
{
	if (!path)
		return -1;
	std::wstring wpath = utf8_to_utf16(path);
	if (!wpath.empty())
		return _waccess(wpath.c_str(), mode);
	return -1;
}

int subsurface_stat(const char* path, struct stat* buf)
{
	if (!path)
		return -1;
	std::wstring wpath = utf8_to_utf16(path);
	if (!wpath.empty())
		return wstat(wpath.c_str(), buf);
	return -1;
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
#ifndef WIN32_CONSOLE_APP
static struct {
	bool allocated;
	UINT cp;
	FILE *out, *err;
} console_desc;
#endif

void subsurface_console_init(void)
{
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
		std::wstring path = system_default_path();
		std::wstring wpath_out = path + L"\\subsurface_out.log";
		std::wstring wpath_err = path + L"\\subsurface_err.log";
		console_desc.out = _wfreopen(wpath_out.c_str(), L"w", stdout);
		console_desc.err = _wfreopen(wpath_err.c_str(), L"w", stderr);
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

}
