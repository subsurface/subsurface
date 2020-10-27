// SPDX-License-Identifier: LGPL-2.1+
#include "subsurfacesysinfo.h"
#include <QSysInfo>

#ifdef Q_OS_WIN
extern "C" bool isWin7Or8()
{
	return (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based) >= QSysInfo::WV_WINDOWS7;
}
#endif
