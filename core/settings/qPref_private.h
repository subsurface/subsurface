// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFPRIVATE_H
#define QPREFPRIVATE_H

// Header used by all qPref<class> implementations to avoid duplicating code

#include <QSettings>
#include <QVariant>
#include <QObject>
#include "core/qthelper.h"

//****** Macros to be used in the set functions ******
#define COPY_TXT(name, string) \
{ \
	free((void *)prefs.name); \
	prefs.name = copy_qstring(string); \
}

//****** Macros to be used in the disk functions, which are special ******
#define LOADSYNC_BOOL(name, field) \
{ \
	if (doSync)	\
		setting.setValue(group + name, prefs.field); \
	else \
		prefs.field = setting.value(group + name, default_prefs.field).toBool(); \
}

#define LOADSYNC_DOUBLE(name, field) \
{ \
	if (doSync)	\
		setting.setValue(group + name, prefs.field); \
	else \
		prefs.field = setting.value(group + name, default_prefs.field).toDouble(); \
}

#define LOADSYNC_ENUM(name, type, field) \
{ \
	if (doSync)	\
		setting.setValue(group + name, prefs.field); \
	else \
		prefs.field = (enum type)setting.value(group + name, default_prefs.field).toInt(); \
}

#define LOADSYNC_INT(name, field) \
{ \
	if (doSync) \
		setting.setValue(group + name, prefs.field); \
	else \
		prefs.field = setting.value(group + name, default_prefs.field).toInt(); \
}

#define LOADSYNC_INT_DEF(name, field, defval) \
{ \
	if (doSync)	\
		setting.setValue(group + name, prefs.field); \
	else \
		prefs.field = setting.value(group + name, defval).toInt(); \
}

#define LOADSYNC_TXT(name, field) \
{ \
	if (doSync)	\
		setting.setValue(group + name, prefs.field); \
	else \
		prefs.field = copy_qstring(setting.value(group + name, default_prefs.field).toString()); \
}

//******* Macros to generate disk function
#define DISK_LOADSYNC_BOOL(class, name, field) \
void qPref ## class::disk_ ## field(bool doSync) \
{ \
	LOADSYNC_BOOL(name, field); \
}

#define DISK_LOADSYNC_DOUBLE(class, name, field) \
void qPref ## class::disk_ ## field(bool doSync) \
{ \
	LOADSYNC_DOUBLE(name, field); \
}

#define DISK_LOADSYNC_ENUM(class, name, type, field) \
void qPref ## class::disk_ ## field(bool doSync) \
{ \
	LOADSYNC_ENUM(name, type, field); \
}

#define DISK_LOADSYNC_INT(class, name, field) \
void qPref ## class::disk_ ## field(bool doSync) \
{ \
	LOADSYNC_DOUBLE(name, field); \
}

#define DISK_LOADSYNC_INT_DEF(class, name, field, defval) \
void qPref ## class::disk_ ## field(bool doSync) \
{ \
	LOADSYNC_INT_DEF(name, field, defval); \
}

#define DISK_LOADSYNC_TXT(class, name, field) \
void qPref ## class::disk_ ## field(bool doSync) \
{ \
	LOADSYNC_TXT(name, field); \
}
#endif
