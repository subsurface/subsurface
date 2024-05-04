// SPDX-License-Identifier: GPL-2.0
#ifndef NAMECMP_H
#define NAMECMP_H

#include <QXmlStreamReader>

// this is annoying Qt5 / Qt6 incompatibility where we can't compare against string literals anymore
static inline int nameCmp(QXmlStreamReader &r, const char * cs)
{
	return r.name().compare(QLatin1String(cs));
}

#endif // NAMECMP_H
