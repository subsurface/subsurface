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

#ifndef SUBSURFACESYSINFO_H
#define SUBSURFACESYSINFO_H

#include <QtGlobal>

#if QT_VERSION < 0x050000
#if defined(__arm__) || defined(__TARGET_ARCH_ARM) || defined(_M_ARM) || defined(__arm64__)
#	define Q_PROCESSOR_ARM
#	if defined(__arm64__)
#		define Q_PROCESSOR_ARM_64
#	else
#		define Q_PROCESSOR_ARM_32
#	endif
#	if defined(__ARM64_ARCH_8__)
#		define Q_PROCESSOR_ARM_V8
#		define Q_PROCESSOR_ARM_V7
#		define Q_PROCESSOR_ARM_V6
#		define Q_PROCESSOR_ARM_V5
#	elif defined(__ARM_ARCH_7__) \
	|| defined(__ARM_ARCH_7A__) \
	|| defined(__ARM_ARCH_7R__) \
	|| defined(__ARM_ARCH_7M__) \
	|| defined(__ARM_ARCH_7S__) \
	|| defined(_ARM_ARCH_7) \
	|| (defined(__TARGET_ARCH_ARM) && __TARGET_ARCH_ARM-0 >= 7) \
	|| (defined(_M_ARM) && _M_ARM-0 >= 7)
#		define Q_PROCESSOR_ARM_V7
#		define Q_PROCESSOR_ARM_V6
#		define Q_PROCESSOR_ARM_V5
#	elif defined(__ARM_ARCH_6__) \
	|| defined(__ARM_ARCH_6J__) \
	|| defined(__ARM_ARCH_6T2__) \
	|| defined(__ARM_ARCH_6Z__) \
	|| defined(__ARM_ARCH_6K__) \
	|| defined(__ARM_ARCH_6ZK__) \
	|| defined(__ARM_ARCH_6M__) \
	|| (defined(__TARGET_ARCH_ARM) && __TARGET_ARCH_ARM-0 >= 6) \
	|| (defined(_M_ARM) && _M_ARM-0 >= 6)
#		define Q_PROCESSOR_ARM_V6
#		define Q_PROCESSOR_ARM_V5
#	elif defined(__ARM_ARCH_5TEJ__) \
	|| defined(__ARM_ARCH_5TE__) \
	|| (defined(__TARGET_ARCH_ARM) && __TARGET_ARCH_ARM-0 >= 5) \
	|| (defined(_M_ARM) && _M_ARM-0 >= 5)
#		define Q_PROCESSOR_ARM_V5
#	endif
#	if defined(__ARMEL__)
#		define Q_BYTE_ORDER Q_LITTLE_ENDIAN
#	elif defined(__ARMEB__)
#		define Q_BYTE_ORDER Q_BIG_ENDIAN
#	else
// Q_BYTE_ORDER not defined, use endianness auto-detection
#endif


#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
#	define Q_PROCESSOR_X86_32
#	define Q_BYTE_ORDER Q_LITTLE_ENDIAN

#	if defined(_M_IX86)
#		define Q_PROCESSOR_X86     (_M_IX86/100)
#	elif defined(__i686__) || defined(__athlon__) || defined(__SSE__)
#		define Q_PROCESSOR_X86     6
#	elif defined(__i586__) || defined(__k6__)
#		define Q_PROCESSOR_X86     5
#	elif defined(__i486__)
#		define Q_PROCESSOR_X86     4
#	else
#		define Q_PROCESSOR_X86     3
#endif

#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(_M_X64)
#	define Q_PROCESSOR_X86       6
#	define Q_PROCESSOR_X86_64
#	define Q_BYTE_ORDER Q_LITTLE_ENDIAN
#	define Q_PROCESSOR_WORDSIZE   8

#elif defined(__ia64) || defined(__ia64__) || defined(_M_IA64)
#	define Q_PROCESSOR_IA64
#	define Q_PROCESSOR_WORDSIZE   8

#elif defined(__mips) || defined(__mips__) || defined(_M_MRX000)
#	define Q_PROCESSOR_MIPS
#	if defined(_MIPS_ARCH_MIPS1) || (defined(__mips) && __mips - 0 >= 1)
#		define Q_PROCESSOR_MIPS_I
#	endif
#	if defined(_MIPS_ARCH_MIPS2) || (defined(__mips) && __mips - 0 >= 2)
#		define Q_PROCESSOR_MIPS_II
#	endif
#	if defined(_MIPS_ARCH_MIPS32) || defined(__mips32)
#		define Q_PROCESSOR_MIPS_32
#	endif
#	if defined(_MIPS_ARCH_MIPS3) || (defined(__mips) && __mips - 0 >= 3)
#		define Q_PROCESSOR_MIPS_III
#	endif
#	if defined(_MIPS_ARCH_MIPS4) || (defined(__mips) && __mips - 0 >= 4)
#		define Q_PROCESSOR_MIPS_IV
#	endif
#	if defined(_MIPS_ARCH_MIPS5) || (defined(__mips) && __mips - 0 >= 5)
#		define Q_PROCESSOR_MIPS_V
#	endif
#	if defined(_MIPS_ARCH_MIPS64) || defined(__mips64)
#		define Q_PROCESSOR_MIPS_64
#	endif
#	if defined(__MIPSEL__)
#		define Q_BYTE_ORDER Q_LITTLE_ENDIAN
#	elif defined(__MIPSEB__)
#		define Q_BYTE_ORDER Q_BIG_ENDIAN
#	else
// Q_BYTE_ORDER not defined, use endianness auto-detection
#endif


#elif defined(__ppc__) || defined(__ppc) || defined(__powerpc__) \
	|| defined(_ARCH_COM) || defined(_ARCH_PWR) || defined(_ARCH_PPC)  \
	|| defined(_M_MPPC) || defined(_M_PPC)
#	define Q_PROCESSOR_POWER
#	if defined(__ppc64__) || defined(__powerpc64__) || defined(__64BIT__)
#		define Q_PROCESSOR_POWER_64
#	else
#		define Q_PROCESSOR_POWER_32
#	endif

#elif defined(__s390__)
#	define Q_PROCESSOR_S390
#	if defined(__s390x__)
#		define Q_PROCESSOR_S390_X
#	endif
#	define Q_BYTE_ORDER Q_BIG_ENDIAN


#endif

// Some processors support either endian format, try to detect which we are using.
#if !defined(Q_BYTE_ORDER)
#	if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == Q_BIG_ENDIAN || __BYTE_ORDER__ == Q_LITTLE_ENDIAN)
// Reuse __BYTE_ORDER__ as-is, since our Q_*_ENDIAN #defines match the preprocessor defaults
#		define Q_BYTE_ORDER __BYTE_ORDER__
#	elif defined(__BIG_ENDIAN__) || defined(_big_endian__) || defined(_BIG_ENDIAN)
#		define Q_BYTE_ORDER Q_BIG_ENDIAN
#	elif defined(__LITTLE_ENDIAN__) || defined(_little_endian__) || defined(_LITTLE_ENDIAN) \
	|| defined(_WIN32_WCE) || defined(WINAPI_FAMILY) // Windows CE is always little-endian according to MSDN.
#		define Q_BYTE_ORDER Q_LITTLE_ENDIAN
#	else
#		error "Unable to determine byte order!"
#	endif
#endif

#ifndef Q_PROCESSOR_WORDSIZE
#	ifdef __SIZEOF_POINTER__
/* GCC & friends define this */
#		define Q_PROCESSOR_WORDSIZE        __SIZEOF_POINTER__
#	elif defined(_LP64) || defined(__LP64__) || defined(WIN64) || defined(_WIN64)
#		define Q_PROCESSOR_WORDSIZE        8
#	else
#		define Q_PROCESSOR_WORDSIZE        QT_POINTER_SIZE
#	endif
#endif
#endif // Qt < 5.0.0

class QString;
class SubsurfaceSysInfo {
public:
	enum Sizes {
		WordSize = (sizeof(void *)<<3)
	};

#ifndef QStringLiteral
	// no lambdas, not GCC, or GCC in C++98 mode with 4-byte wchar_t
	// fallback, return a temporary QString
	// source code is assumed to be encoded in UTF-8

# define QStringLiteral(str) QString::fromUtf8("" str "", sizeof(str) - 1)
#endif


#if defined(QT_BUILD_QMAKE)
	enum Endian {
		BigEndian,
		LittleEndian
	};
	/* needed to bootstrap qmake */
	static const int ByteOrder;
#elif defined(Q_BYTE_ORDER)
	enum Endian {
		BigEndian,
		LittleEndian

#  ifdef Q_QDOC
		, ByteOrder = <platform-dependent>
#  elif Q_BYTE_ORDER == Q_BIG_ENDIAN
		, ByteOrder = BigEndian
#  elif Q_BYTE_ORDER == Q_LITTLE_ENDIAN
		, ByteOrder = LittleEndian
#  else
#    error "Undefined byte order"
#  endif
	};
#endif
#if defined(Q_OS_WIN) || defined(Q_OS_CYGWIN)
	enum WinVersion {
		WV_32s      = 0x0001,
		WV_95       = 0x0002,
		WV_98       = 0x0003,
		WV_Me       = 0x0004,
		WV_DOS_based= 0x000f,

		/* codenames */
		WV_NT       = 0x0010,
		WV_2000     = 0x0020,
		WV_XP       = 0x0030,
		WV_2003     = 0x0040,
		WV_VISTA    = 0x0080,
		WV_WINDOWS7 = 0x0090,
		WV_WINDOWS8 = 0x00a0,
		WV_WINDOWS8_1 = 0x00b0,
		WV_NT_based = 0x00f0,

		/* version numbers */
		WV_4_0      = WV_NT,
		WV_5_0      = WV_2000,
		WV_5_1      = WV_XP,
		WV_5_2      = WV_2003,
		WV_6_0      = WV_VISTA,
		WV_6_1      = WV_WINDOWS7,
		WV_6_2      = WV_WINDOWS8,
		WV_6_3      = WV_WINDOWS8_1,

		WV_CE       = 0x0100,
		WV_CENET    = 0x0200,
		WV_CE_5     = 0x0300,
		WV_CE_6     = 0x0400,
		WV_CE_based = 0x0f00
	};
	static const WinVersion WindowsVersion;
	static WinVersion windowsVersion();

#endif
#ifdef Q_OS_MAC
#  define Q_MV_IOS(major, minor) (QSysInfo::MV_IOS | major << 4 | minor)
	enum MacVersion {
		MV_Unknown = 0x0000,

		/* version */
		MV_9 = 0x0001,
		MV_10_0 = 0x0002,
		MV_10_1 = 0x0003,
		MV_10_2 = 0x0004,
		MV_10_3 = 0x0005,
		MV_10_4 = 0x0006,
		MV_10_5 = 0x0007,
		MV_10_6 = 0x0008,
		MV_10_7 = 0x0009,
		MV_10_8 = 0x000A,
		MV_10_9 = 0x000B,
		MV_10_10 = 0x000C,

		/* codenames */
		MV_CHEETAH = MV_10_0,
		MV_PUMA = MV_10_1,
		MV_JAGUAR = MV_10_2,
		MV_PANTHER = MV_10_3,
		MV_TIGER = MV_10_4,
		MV_LEOPARD = MV_10_5,
		MV_SNOWLEOPARD = MV_10_6,
		MV_LION = MV_10_7,
		MV_MOUNTAINLION = MV_10_8,
		MV_MAVERICKS = MV_10_9,
		MV_YOSEMITE = MV_10_10,

		/* iOS */
		MV_IOS     = 1 << 8,
		MV_IOS_4_3 = Q_MV_IOS(4, 3),
		MV_IOS_5_0 = Q_MV_IOS(5, 0),
		MV_IOS_5_1 = Q_MV_IOS(5, 1),
		MV_IOS_6_0 = Q_MV_IOS(6, 0),
		MV_IOS_6_1 = Q_MV_IOS(6, 1),
		MV_IOS_7_0 = Q_MV_IOS(7, 0),
		MV_IOS_7_1 = Q_MV_IOS(7, 1),
		MV_IOS_8_0 = Q_MV_IOS(8, 0)
	};
	static const MacVersion MacintoshVersion;
	static MacVersion macVersion();
#endif
	static QString unknownText();
	static QString cpuArchitecture();
	static QString fullCpuArchitecture();
	static QString osType();
	static QString osKernelVersion();
	static QString osVersion();
	static QString prettyOsName();
};


#endif // SUBSURFACESYSINFO_H
