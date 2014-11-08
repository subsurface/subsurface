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
#include <QFile>
#include <QSettings>

#ifndef QStringLiteral
#	define QStringLiteral QString::fromUtf8
#endif

#ifdef Q_OS_UNIX
#include <sys/utsname.h>
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

#if defined(Q_OS_WIN) || defined(Q_OS_CYGWIN) || defined(Q_OS_WINCE) || defined(Q_OS_WINRT)

QT_BEGIN_INCLUDE_NAMESPACE
#include "qt_windows.h"
QT_END_INCLUDE_NAMESPACE

#ifndef Q_OS_WINRT
static inline OSVERSIONINFO winOsVersion()
{
	OSVERSIONINFO result = { sizeof(OSVERSIONINFO), 0, 0, 0, 0, {'\0'}};
	// GetVersionEx() has been deprecated in Windows 8.1 and will return
	// only Windows 8 from that version on.
#	if defined(_MSC_VER) && _MSC_VER >= 1800
#		pragma warning( push )
#		pragma warning( disable : 4996 )
#	endif
	GetVersionEx(&result);
#	if defined(_MSC_VER) && _MSC_VER >= 1800
#		pragma warning( pop )
#	endif
#	ifndef Q_OS_WINCE
		if (result.dwMajorVersion == 6 && result.dwMinorVersion == 2) {
			// This could be Windows 8.1 or higher. Note that as of Windows 9,
			// the major version needs to be checked as well.
			DWORDLONG conditionMask = 0;
			VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
			VER_SET_CONDITION(conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
			VER_SET_CONDITION(conditionMask, VER_PLATFORMID, VER_EQUAL);
			OSVERSIONINFOEX checkVersion = { sizeof(OSVERSIONINFOEX), result.dwMajorVersion, result.dwMinorVersion,
							 result.dwBuildNumber, result.dwPlatformId, {'\0'}, 0, 0, 0, 0, 0 };
			for ( ; VerifyVersionInfo(&checkVersion, VER_MAJORVERSION | VER_MINORVERSION | VER_PLATFORMID, conditionMask); ++checkVersion.dwMinorVersion)
				result.dwMinorVersion = checkVersion.dwMinorVersion;
		}
#	endif // !Q_OS_WINCE
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
#	if (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || defined(Q_OS_FREEBSD)
#		define USE_ETC_OS_RELEASE
#	endif
struct QUnixOSVersion
{
	// from uname(2)
	QString sysName;
	QString sysNameLower;
	QString sysRelease;

#  if !defined(Q_OS_ANDROID) && !defined(Q_OS_BLACKBERRY) && !defined(Q_OS_MAC)
	// from /etc/os-release or guessed
	QString versionIdentifier;      // ${ID}_$VERSION_ID
	QString versionText;            // $PRETTY_NAME
#  endif
};


#ifdef USE_ETC_OS_RELEASE
static bool readEtcOsRelease(QUnixOSVersion &v)
{
	QFile osRelease("/etc/os-release");
	if (osRelease.exists()) {
		QSettings parse("/etc/os-release", QSettings::IniFormat);
		if (parse.contains("PRETTY_NAME")) {
			v.versionText = parse.value("PRETTY_NAME").toString();
		}
		return true;
	}
	return false;
}
#endif // USE_ETC_OS_RELEASE

static QUnixOSVersion detectUnixVersion()
{
	QUnixOSVersion v;
	struct utsname u;
	if (uname(&u) != -1) {
		v.sysName = QString::fromLatin1(u.sysname);
		v.sysNameLower = v.sysName.toLower();
		v.sysRelease = QString::fromLatin1(u.release);
	} else {
		v.sysName = QLatin1String("Detection failed");
		// leave sysNameLower & sysRelease unset
	}

#	if !defined(Q_OS_ANDROID) && !defined(Q_OS_BLACKBERRY) && !defined(Q_OS_MAC)
#		ifdef USE_ETC_OS_RELEASE
		if (readEtcOsRelease(v))
			return v;
#	endif

	if (!v.sysNameLower.isEmpty()) {
		// will produce "qnx_6.5" or "sunos_5.9"
		v.versionIdentifier = v.sysNameLower + QLatin1Char('_') + v.sysRelease;
	}
#  endif

	return v;
}
#endif

static QString unknownText()
{
	return QStringLiteral("unknown");
}

QString SubsurfaceSysInfo::cpuArchitecture()
{
	return QStringLiteral(ARCH_PROCESSOR);
}

QString SubsurfaceSysInfo::fullCpuArchitecture()
{
	return QLatin1String(ARCH_FULL);
}

QString SubsurfaceSysInfo::osType()
{
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
#elif defined(Q_OS_LINUX)
	return QStringLiteral("linux");

#elif defined(Q_OS_IOS)
	return QStringLiteral("ios");
#elif defined(Q_OS_OSX)
	return QStringLiteral("osx");
#elif defined(Q_OS_DARWIN)
	return QStringLiteral("darwin");

#elif defined(Q_OS_FREEBSD_KERNEL)
	return QStringLiteral("freebsd");
#elif defined(Q_OS_UNIX)
	QUnixOSVersion unixOsVersion = detectUnixVersion();
	if (!unixOsVersion.sysNameLower.isEmpty())
		return unixOsVersion.sysNameLower;
#endif
	return unknownText();
}

QString SubsurfaceSysInfo::osKernelVersion()
{
#ifdef Q_OS_WINRT
	// TBD
	return QString();
#elif defined(Q_OS_WIN)
	const OSVERSIONINFO osver = winOsVersion();
	return QString::number(int(osver.dwMajorVersion)) + QLatin1Char('.') + QString::number(int(osver.dwMinorVersion))
			+ QLatin1Char('.') + QString::number(int(osver.dwBuildNumber));
#else
	return detectUnixVersion().sysRelease;
#endif
}

QString SubsurfaceSysInfo::osVersion()
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
#elif defined(Q_OS_UNIX)
	QUnixOSVersion unixOsVersion = detectUnixVersion();
	if (!unixOsVersion.versionIdentifier.isEmpty())
		return unixOsVersion.versionIdentifier;
#endif

	// fallback
	return unknownText();
}

QString SubsurfaceSysInfo::prettyOsName()
{
#if defined(Q_OS_IOS)
	return QLatin1String("iOS ") + osVersion();
#elif defined(Q_OS_OSX)
	// get the known codenames
	const char *basename = 0;
	switch (int(MacintoshVersion)) {
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
#ifdef MV_MAVERICKS
	case MV_MAVERICKS:
#else
	case 0x000B: // MV_MAVERICKS
#endif
		basename = "OS X Mavericks (";
		break;
#ifdef MV_YOSEMITE
	case MV_YOSEMITE:
#else
	case 0x000C: // MV_YOSEMITE
#endif
		basename = "OS X Yosemite (";
		break;
	}
	if (basename)
		return QLatin1String(basename) + osVersion() + QLatin1Char(')');

	// a future version of OS X
	return QLatin1String("OS X ") + osVersion();
#elif defined(Q_OS_WINPHONE)
	return QLatin1String("Windows Phone ") + QLatin1String(winVer_helper());
#elif defined(Q_OS_WIN)
	return QLatin1String("Windows ") + QLatin1String(winVer_helper());
#elif defined(Q_OS_ANDROID)
	return QLatin1String("Android ") + osVersion();
#elif defined(Q_OS_BLACKBERRY)
	return QLatin1String("BlackBerry ") + osVersion();
#elif defined(Q_OS_UNIX)
	QUnixOSVersion unixOsVersion = detectUnixVersion();
	if (unixOsVersion.versionText.isEmpty())
		return unixOsVersion.sysName;
	else
		return unixOsVersion.sysName + QLatin1String(" (") + unixOsVersion.versionText + QLatin1Char(')');
#else
	return unknownText();
#endif
}

// detect if the OS we are running on is 64 or 32bit
// we only care when building on Intel for 32bit
QString SubsurfaceSysInfo::osArch()
{
	QString res = "";
#if defined(Q_PROCESSOR_X86_32)
#if defined(Q_OS_UNIX)
	struct utsname u;
	if (uname(&u) != -1) {
		res = u.machine;
	}
#elif defined(Q_OS_WIN)

	/* this code is from
	 * http://mark.koli.ch/reliably-checking-os-bitness-32-or-64-bit-on-windows-with-a-tiny-c-app
	 * there is no license given, but 5 lines of code should be fine to reuse unless explicitly forbidden */
	typedef BOOL (WINAPI *IW64PFP)(HANDLE, BOOL *);
	BOOL os64 = FALSE;
	IW64PFP IW64P = (IW64PFP)GetProcAddress(
				GetModuleHandle((LPCSTR)"kernel32"), "IsWow64Process");

	if(IW64P != NULL){
		IW64P(GetCurrentProcess(), &os64);
	}
	res = os64 ? "x86_64" : "i386";
#endif
#endif
	return res;
}

extern "C" {
bool isWin7Or8()
{
	QString os = SubsurfaceSysInfo::prettyOsName();
	return os == "Windows 7" || os.startsWith("Windows 8");
}
}
