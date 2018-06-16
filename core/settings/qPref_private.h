// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFPRIVATE_H
#define QPREFPRIVATE_H

// Header used by all qPref<class> implementations to avoid duplicating code

#include <QObject>
#include <QSettings>
#include <QVariant>
#include "core/qthelper.h"

#define COPY_TXT(name, string) \
{ \
	free((void *)prefs.name); \
	prefs.name = copy_qstring(string); \
}

#define LOADSYNC_INT(name, field) \
{ \
	QSettings s; \
	if (doSync) \
		s.setValue(group + name, prefs.field); \
	else \
		prefs.field = s.value(group + name, default_prefs.field).toInt(); \
}

#define LOADSYNC_INT_DEF(name, field, defval) \
{ \
	QSettings s; \
	if (doSync)	\
		s.setValue(group + name, prefs.field); \
	else \
		prefs.field = s.value(group + name, defval).toInt(); \
}

#define LOADSYNC_ENUM(name, type, field) \
{ \
	QSettings s; \
	if (doSync)	\
		s.setValue(group + name, prefs.field); \
	else \
		prefs.field = (enum type)s.value(group + name, default_prefs.field).toInt(); \
}

#define LOADSYNC_BOOL(name, field) \
{ \
	QSettings s; \
	if (doSync)	\
		s.setValue(group + name, prefs.field); \
	else \
		prefs.field = s.value(group + name, default_prefs.field).toBool(); \
}

#define LOADSYNC_DOUBLE(name, field) \
{ \
	QSettings s; \
	if (doSync)	\
		s.setValue(group + name, prefs.field); \
	else \
		prefs.field = s.value(group + name, default_prefs.field).toDouble(); \
}

#define LOADSYNC_TXT(name, field) \
{ \
	QSettings s; \
	if (doSync)	\
		s.setValue(group + name, prefs.field); \
	else \
		prefs.field = copy_qstring(s.value(group + name, default_prefs.field).toString()); \
}

#endif
