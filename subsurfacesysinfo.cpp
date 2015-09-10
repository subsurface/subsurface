/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2014 Intel Corporation
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "subsurfacesysinfo.h"
#include <QString>

#ifdef Q_OS_UNIX
#include <sys/utsname.h>
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)

#ifndef QStringLiteral
#	define QStringLiteral QString::fromUtf8
#endif

#ifdef Q_OS_UNIX
#include <qplatformdefs.h>
#endif

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#endif

// --- this is a copy of Qt 5.4's src/corelib/global/archdetect.cpp ---

// main part: processor type
#if defined(Q_PROCESSOR_ALPHA)
#	define ARCH_PROCESSOR "alpha"
#elif defined(Q_PROCESSOR_ARM)
#	define ARCH_PROCESSOR "arm"
#elif defined(Q_PROCESSOR_AVR32)
#	define ARCH_PROCESSOR "avr32"
#elif defined(Q_PROCESSOR_BLACKFIN)
#	define ARCH_PROCESSOR "bfin"
#elif defined(Q_PROCESSOR_X86_32)
#	define ARCH_PROCESSOR "i386"
#elif defined(Q_PROCESSOR_X86_64)
#	define ARCH_PROCESSOR "x86_64"
#elif defined(Q_PROCESSOR_IA64)
#	define ARCH_PROCESSOR "ia64"
#elif defined(Q_PROCESSOR_MIPS)
#	define ARCH_PROCESSOR "mips"
#elif defined(Q_PROCESSOR_POWER)
#	define ARCH_PROCESSOR "power"
#elif defined(Q_PROCESSOR_S390)
#	define ARCH_PROCESSOR "s390"
#elif defined(Q_PROCESSOR_SH)
#	define ARCH_PROCESSOR "sh"
#elif defined(Q_PROCESSOR_SPARC)
#	define ARCH_PROCESSOR "sparc"
#else
#	define ARCH_PROCESSOR "unknown"
#endif

// endinanness
#if defined(Q_LITTLE_ENDIAN)
#	define ARCH_ENDIANNESS "little_endian"
#elif defined(Q_BIG_ENDIAN)
#	define ARCH_ENDIANNESS "big_endian"
#endif

// pointer type
#if defined(Q_OS_WIN64) || (defined(Q_OS_WINRT) && defined(_M_X64))
#	define ARCH_POINTER "llp64"
#elif defined(__LP64__) || QT_POINTER_SIZE - 0 == 8
#	define ARCH_POINTER "lp64"
#else
#	define ARCH_POINTER "ilp32"
#endif

// secondary: ABI string (includes the dash)
#if defined(__ARM_EABI__) || defined(__mips_eabi)
#	define ARCH_ABI1 "-eabi"
#elif defined(_MIPS_SIM)
#	if _MIPS_SIM == _ABIO32
#		define ARCH_ABI1 "-o32"
#	elif _MIPS_SIM == _ABIN32
#		define ARCH_ABI1 "-n32"
#	elif _MIPS_SIM == _ABI64
#		define ARCH_ABI1 "-n64"
#	elif _MIPS_SIM == _ABIO64
#		define ARCH_ABI1 "-o64"
#	endif
#else
#	define ARCH_ABI1 ""
#endif
#if defined(__ARM_PCS_VFP) || defined(__mips_hard_float)
#	define ARCH_ABI2 "-hardfloat"
#else
#	define ARCH_ABI2 ""
#endif

#define ARCH_ABI ARCH_ABI1 ARCH_ABI2

#define ARCH_FULL ARCH_PROCESSOR "-" ARCH_ENDIANNESS "-" ARCH_POINTER ARCH_ABI

// --- end of archdetect.cpp ---
// copied from Qt 5.4.1's src/corelib/global/qglobal.cpp

#if defined(Q_OS_WIN) || defined(Q_OS_CYGWIN) || defined(Q_OS_WINCE) || defined(Q_OS_WINRT)

QT_BEGIN_INCLUDE_NAMESPACE
#include "qt_windows.h"
QT_END_INCLUDE_NAMESPACE

#ifndef Q_OS_WINRT
#  ifndef Q_OS_WINCE
// Fallback for determining Windows versions >= 8 by looping using the
// version check macros. Note that it will return build number=0 to avoid
// inefficient looping.
static inline void determineWinOsVersionFallbackPost8(OSVERSIONINFO *result)
{
	result->dwBuildNumber = 0;
	DWORDLONG conditionMask = 0;
	VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(conditionMask, VER_PLATFORMID, VER_EQUAL);
	OSVERSIONINFOEX checkVersion = { sizeof(OSVERSIONINFOEX), result->dwMajorVersion, 0,
					 result->dwBuildNumber, result->dwPlatformId, {'\0'}, 0, 0, 0, 0, 0 };
	for ( ; VerifyVersionInfo(&checkVersion, VER_MAJORVERSION | VER_PLATFORMID, conditionMask); ++checkVersion.dwMajorVersion)
		result->dwMajorVersion = checkVersion.dwMajorVersion;
	conditionMask = 0;
	checkVersion.dwMajorVersion = result->dwMajorVersion;
	checkVersion.dwMinorVersion = 0;
	VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_EQUAL);
	VER_SET_CONDITION(conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(conditionMask, VER_PLATFORMID, VER_EQUAL);
	for ( ; VerifyVersionInfo(&checkVersion, VER_MAJORVERSION | VER_MINORVERSION | VER_PLATFORMID, conditionMask); ++checkVersion.dwMinorVersion)
		result->dwMinorVersion = checkVersion.dwMinorVersion;
}

#  endif // !Q_OS_WINCE

static inline OSVERSIONINFO winOsVersion()
{
	OSVERSIONINFO result = { sizeof(OSVERSIONINFO), 0, 0, 0, 0, {'\0'}};
	// GetVersionEx() has been deprecated in Windows 8.1 and will return
	// only Windows 8 from that version on.
#  if defined(_MSC_VER) && _MSC_VER >= 1800
#    pragma warning( push )
#    pragma warning( disable : 4996 )
#  endif
	GetVersionEx(&result);
#  if defined(_MSC_VER) && _MSC_VER >= 1800
#    pragma warning( pop )
#  endif
#  ifndef Q_OS_WINCE
	if (result.dwMajorVersion == 6 && result.dwMinorVersion == 2) {
		determineWinOsVersionFallbackPost8(&result);
	}
#  endif // !Q_OS_WINCE
	return result;
}
#endif // !Q_OS_WINRT

static const char *winVer_helper()
{
	switch (int(SubsurfaceSysInfo::WindowsVersion)) {
	case SubsurfaceSysInfo::WV_NT:
		return "NT";
	case SubsurfaceSysInfo::WV_2000:
		return "2000";
	case SubsurfaceSysInfo::WV_XP:
		return "XP";
	case SubsurfaceSysInfo::WV_2003:
		return "2003";
	case SubsurfaceSysInfo::WV_VISTA:
		return "Vista";
	case SubsurfaceSysInfo::WV_WINDOWS7:
		return "7";
	case SubsurfaceSysInfo::WV_WINDOWS8:
		return "8";
	case SubsurfaceSysInfo::WV_WINDOWS8_1:
		return "8.1";

	case SubsurfaceSysInfo::WV_CE:
		return "CE";
	case SubsurfaceSysInfo::WV_CENET:
		return "CENET";
	case SubsurfaceSysInfo::WV_CE_5:
		return "CE5";
	case SubsurfaceSysInfo::WV_CE_6:
		return "CE6";
	}
	// unknown, future version
	return 0;
}
#endif

#if defined(Q_OS_UNIX)
#  if (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || defined(Q_OS_FREEBSD)
#    define USE_ETC_OS_RELEASE
struct QUnixOSVersion
{
	// from /etc/os-release
	QString productType;            // $ID
	QString productVersion;         // $VERSION_ID
	QString prettyName;             // $PRETTY_NAME
};

static QString unquote(const char *begin, const char *end)
{
    if (*begin == '"') {
	Q_ASSERT(end[-1] == '"');
	return QString::fromLatin1(begin + 1, end - begin - 2);
    }
    return QString::fromLatin1(begin, end - begin);
}

static bool readEtcOsRelease(QUnixOSVersion &v)
{
	// we're avoiding QFile here
	int fd = QT_OPEN("/etc/os-release", O_RDONLY);
	if (fd == -1)
		return false;

	QT_STATBUF sbuf;
	if (QT_FSTAT(fd, &sbuf) == -1) {
		QT_CLOSE(fd);
		return false;
	}

	QByteArray buffer(sbuf.st_size, Qt::Uninitialized);
	buffer.resize(QT_READ(fd, buffer.data(), sbuf.st_size));
	QT_CLOSE(fd);

	const char *ptr = buffer.constData();
	const char *end = buffer.constEnd();
	const char *eol;
	for ( ; ptr != end; ptr = eol + 1) {
		static const char idString[] = "ID=";
		static const char prettyNameString[] = "PRETTY_NAME=";
		static const char versionIdString[] = "VERSION_ID=";

		// find the end of the line after ptr
		eol = static_cast<const char *>(memchr(ptr, '\n', end - ptr));
		if (!eol)
			eol = end - 1;

		// note: we're doing a binary search here, so comparison
		// must always be sorted
		int cmp = strncmp(ptr, idString, strlen(idString));
		if (cmp < 0)
			continue;
		if (cmp == 0) {
			ptr += strlen(idString);
			v.productType = unquote(ptr, eol);
			continue;
		}

		cmp = strncmp(ptr, prettyNameString, strlen(prettyNameString));
		if (cmp < 0)
			continue;
		if (cmp == 0) {
			ptr += strlen(prettyNameString);
			v.prettyName = unquote(ptr, eol);
			continue;
		}

		cmp = strncmp(ptr, versionIdString, strlen(versionIdString));
		if (cmp < 0)
			continue;
		if (cmp == 0) {
			ptr += strlen(versionIdString);
			v.productVersion = unquote(ptr, eol);
			continue;
		}
	}

	return true;
}
#  endif // USE_ETC_OS_RELEASE
#endif // Q_OS_UNIX

static QString unknownText()
{
	return QStringLiteral("unknown");
}

QString SubsurfaceSysInfo::buildCpuArchitecture()
{
	return QStringLiteral(ARCH_PROCESSOR);
}

QString SubsurfaceSysInfo::currentCpuArchitecture()
{
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
	// We don't need to catch all the CPU architectures in this function;
	// only those where the host CPU might be different than the build target
	// (usually, 64-bit platforms).
	SYSTEM_INFO info;
	GetNativeSystemInfo(&info);
	switch (info.wProcessorArchitecture) {
#  ifdef PROCESSOR_ARCHITECTURE_AMD64
	case PROCESSOR_ARCHITECTURE_AMD64:
		return QStringLiteral("x86_64");
#  endif
#  ifdef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
	case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
#  endif
	case PROCESSOR_ARCHITECTURE_IA64:
		return QStringLiteral("ia64");
	}
#elif defined(Q_OS_UNIX)
	long ret = -1;
	struct utsname u;

#  if defined(Q_OS_SOLARIS)
	// We need a special call for Solaris because uname(2) on x86 returns "i86pc" for
	// both 32- and 64-bit CPUs. Reference:
	// http://docs.oracle.com/cd/E18752_01/html/816-5167/sysinfo-2.html#REFMAN2sysinfo-2
	// http://fxr.watson.org/fxr/source/common/syscall/systeminfo.c?v=OPENSOLARIS
	// http://fxr.watson.org/fxr/source/common/conf/param.c?v=OPENSOLARIS;im=10#L530
	if (ret == -1)
		ret = sysinfo(SI_ARCHITECTURE_64, u.machine, sizeof u.machine);
#  endif

	if (ret == -1)
		ret = uname(&u);

	// we could use detectUnixVersion() above, but we only need a field no other function does
	if (ret != -1) {
		// the use of QT_BUILD_INTERNAL here is simply to ensure all branches build
		// as we don't often build on some of the less common platforms
#  if defined(Q_PROCESSOR_ARM) || defined(QT_BUILD_INTERNAL)
		if (strcmp(u.machine, "aarch64") == 0)
			return QStringLiteral("arm64");
		if (strncmp(u.machine, "armv", 4) == 0)
			return QStringLiteral("arm");
#  endif
#  if defined(Q_PROCESSOR_POWER) || defined(QT_BUILD_INTERNAL)
		// harmonize "powerpc" and "ppc" to "power"
		if (strncmp(u.machine, "ppc", 3) == 0)
			return QLatin1String("power") + QLatin1String(u.machine + 3);
		if (strncmp(u.machine, "powerpc", 7) == 0)
			return QLatin1String("power") + QLatin1String(u.machine + 7);
		if (strcmp(u.machine, "Power Macintosh") == 0)
			return QLatin1String("power");
#  endif
#  if defined(Q_PROCESSOR_SPARC) || defined(QT_BUILD_INTERNAL)
		// Solaris sysinfo(2) (above) uses "sparcv9", but uname -m says "sun4u";
		// Linux says "sparc64"
		if (strcmp(u.machine, "sun4u") == 0 || strcmp(u.machine, "sparc64") == 0)
			return QStringLiteral("sparcv9");
		if (strcmp(u.machine, "sparc32") == 0)
			return QStringLiteral("sparc");
#  endif
#  if defined(Q_PROCESSOR_X86) || defined(QT_BUILD_INTERNAL)
		// harmonize all "i?86" to "i386"
		if (strlen(u.machine) == 4 && u.machine[0] == 'i'
				&& u.machine[2] == '8' && u.machine[3] == '6')
			return QStringLiteral("i386");
		if (strcmp(u.machine, "amd64") == 0) // Solaris
			return QStringLiteral("x86_64");
#  endif
		return QString::fromLatin1(u.machine);
	}
#endif
	return buildCpuArchitecture();
}


QString SubsurfaceSysInfo::buildAbi()
{
	return QLatin1String(ARCH_FULL);
}

QString SubsurfaceSysInfo::kernelType()
{
#if defined(Q_OS_WINCE)
	return QStringLiteral("wince");
#elif defined(Q_OS_WIN)
	return QStringLiteral("winnt");
#elif defined(Q_OS_UNIX)
	struct utsname u;
	if (uname(&u) == 0)
		return QString::fromLatin1(u.sysname).toLower();
#endif
	return unknownText();
}

QString SubsurfaceSysInfo::kernelVersion()
{
#ifdef Q_OS_WINRT
	// TBD
	return QString();
#elif defined(Q_OS_WIN)
	const OSVERSIONINFO osver = winOsVersion();
	return QString::number(int(osver.dwMajorVersion)) + QLatin1Char('.') + QString::number(int(osver.dwMinorVersion))
			+ QLatin1Char('.') + QString::number(int(osver.dwBuildNumber));
#else
	struct utsname u;
	if (uname(&u) == 0)
		return QString::fromLatin1(u.release);
	return QString();
#endif
}

QString SubsurfaceSysInfo::productType()
{
	// similar, but not identical to QFileSelectorPrivate::platformSelectors
#if defined(Q_OS_WINPHONE)
	return QStringLiteral("winphone");
#elif defined(Q_OS_WINRT)
	return QStringLiteral("winrt");
#elif defined(Q_OS_WINCE)
	return QStringLiteral("wince");
#elif defined(Q_OS_WIN)
	return QStringLiteral("windows");

#elif defined(Q_OS_BLACKBERRY)
	return QStringLiteral("blackberry");
#elif defined(Q_OS_QNX)
	return QStringLiteral("qnx");

#elif defined(Q_OS_ANDROID)
	return QStringLiteral("android");

#elif defined(Q_OS_IOS)
	return QStringLiteral("ios");
#elif defined(Q_OS_OSX)
	return QStringLiteral("osx");
#elif defined(Q_OS_DARWIN)
	return QStringLiteral("darwin");

#elif defined(USE_ETC_OS_RELEASE) // Q_OS_UNIX
	QUnixOSVersion unixOsVersion;
	readEtcOsRelease(unixOsVersion);
	if (!unixOsVersion.productType.isEmpty())
		return unixOsVersion.productType;
#endif
	return unknownText();
}

QString SubsurfaceSysInfo::productVersion()
{
#if defined(Q_OS_IOS)
	int major = (int(MacintoshVersion) >> 4) & 0xf;
	int minor = int(MacintoshVersion) & 0xf;
	if (Q_LIKELY(major < 10 && minor < 10)) {
		char buf[4] = { char(major + '0'), '.', char(minor + '0'), '\0' };
		return QString::fromLatin1(buf, 3);
	}
	return QString::number(major) + QLatin1Char('.') + QString::number(minor);
#elif defined(Q_OS_OSX)
	int minor = int(MacintoshVersion) - 2;  // we're not running on Mac OS 9
	Q_ASSERT(minor < 100);
	char buf[] = "10.0\0";
	if (Q_LIKELY(minor < 10)) {
		buf[3] += minor;
	} else {
		buf[3] += minor / 10;
		buf[4] = '0' + minor % 10;
	}
	return QString::fromLatin1(buf);
#elif defined(Q_OS_WIN)
	const char *version = winVer_helper();
	if (version)
		return QString::fromLatin1(version).toLower();
	// fall through

	// Android and Blackberry should not fall through to the Unix code
#elif defined(Q_OS_ANDROID)
	// TBD
#elif defined(Q_OS_BLACKBERRY)
	deviceinfo_details_t *deviceInfo;
	if (deviceinfo_get_details(&deviceInfo) == BPS_SUCCESS) {
		QString bbVersion = QString::fromLatin1(deviceinfo_details_get_device_os_version(deviceInfo));
		deviceinfo_free_details(&deviceInfo);
		return bbVersion;
	}
#elif defined(USE_ETC_OS_RELEASE) // Q_OS_UNIX
	QUnixOSVersion unixOsVersion;
	readEtcOsRelease(unixOsVersion);
	if (!unixOsVersion.productVersion.isEmpty())
		return unixOsVersion.productVersion;
#endif

	// fallback
	return unknownText();
}

QString SubsurfaceSysInfo::prettyProductName()
{
#if defined(Q_OS_IOS)
	return QLatin1String("iOS ") + productVersion();
#elif defined(Q_OS_OSX)
	// get the known codenames
	const char *basename = 0;
	switch (int(MacintoshVersion)) {
	case MV_CHEETAH:
	case MV_PUMA:
	case MV_JAGUAR:
	case MV_PANTHER:
	case MV_TIGER:
		// This version of Qt does not run on those versions of OS X
		// so this case label will never be reached
		Q_UNREACHABLE();
		break;
	case MV_LEOPARD:
		basename = "Mac OS X Leopard (";
		break;
	case MV_SNOWLEOPARD:
		basename = "Mac OS X Snow Leopard (";
		break;
	case MV_LION:
		basename = "Mac OS X Lion (";
		break;
	case MV_MOUNTAINLION:
		basename = "OS X Mountain Lion (";
		break;
	case MV_MAVERICKS:
		basename = "OS X Mavericks (";
		break;
#ifdef MV_YOSEMITE
	case MV_YOSEMITE:
#else
	case 0x000C: // MV_YOSEMITE
#endif
		basename = "OS X Yosemite (";
		break;
#ifdef MV_ELCAPITAN
	case MV_ELCAPITAN :
#else
	case 0x000D: // MV_ELCAPITAN
#endif
		basename = "OS X El Capitan (";
		break;
	}
	if (basename)
		return QLatin1String(basename) + productVersion() + QLatin1Char(')');

	// a future version of OS X
	return QLatin1String("OS X ") + productVersion();
#elif defined(Q_OS_WINPHONE)
	return QLatin1String("Windows Phone ") + QLatin1String(winVer_helper());
#elif defined(Q_OS_WIN)
	return QLatin1String("Windows ") + QLatin1String(winVer_helper());
#elif defined(Q_OS_ANDROID)
	return QLatin1String("Android ") + productVersion();
#elif defined(Q_OS_BLACKBERRY)
	return QLatin1String("BlackBerry ") + productVersion();
#elif defined(Q_OS_UNIX)
#  ifdef USE_ETC_OS_RELEASE
	QUnixOSVersion unixOsVersion;
	readEtcOsRelease(unixOsVersion);
	if (!unixOsVersion.prettyName.isEmpty())
		return unixOsVersion.prettyName;
#  endif
	struct utsname u;
	if (uname(&u) == 0)
		return QString::fromLatin1(u.sysname) + QLatin1Char(' ') + QString::fromLatin1(u.release);
#endif
	return unknownText();
}

#endif // Qt >= 5.4

QString SubsurfaceSysInfo::prettyOsName()
{
	// Matches the pre-release version of Qt 5.4
	QString pretty = prettyProductName();
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_ANDROID)
	// QSysInfo::kernelType() returns lowercase ("linux" instead of "Linux")
	struct utsname u;
	if (uname(&u) == 0)
		return QString::fromLatin1(u.sysname) + QLatin1String(" (") + pretty + QLatin1Char(')');
#endif
	return pretty;
}

extern "C" {
bool isWin7Or8()
{
#ifdef Q_OS_WIN
       return (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based) >= QSysInfo::WV_WINDOWS7;
#else
       return false;
#endif
}
}
