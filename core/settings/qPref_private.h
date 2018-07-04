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
#define DISK_LOADSYNC_BOOL(usegroup, name, field) \
void qPref ## usegroup::disk_ ## field(bool doSync) \
{ \
	LOADSYNC_BOOL(name, field); \
}

#define DISK_LOADSYNC_DOUBLE(usegroup, name, field) \
void qPref ## usegroup::disk_ ## field(bool doSync) \
{ \
	LOADSYNC_DOUBLE(name, field); \
}

#define DISK_LOADSYNC_ENUM(usegroup, name, type, field) \
void qPref ## usegroup::disk_ ## field(bool doSync) \
{ \
	LOADSYNC_ENUM(name, type, field); \
}

#define DISK_LOADSYNC_INT(usegroup, name, field) \
void qPref ## usegroup::disk_ ## field(bool doSync) \
{ \
	LOADSYNC_DOUBLE(name, field); \
}

#define DISK_LOADSYNC_INT_DEF(usegroup, name, field, defval) \
void qPref ## usegroup::disk_ ## field(bool doSync) \
{ \
	LOADSYNC_INT_DEF(name, field, defval); \
}

#define DISK_LOADSYNC_TXT(usegroup, name, field) \
void qPref ## usegroup::disk_ ## field(bool doSync) \
{ \
	LOADSYNC_TXT(name, field); \
}

//******* Macros to generate get function
#define GET_PREFERENCE_BOOL(usegroup, field) \
bool qPref ## usegroup::field () const \
{ \
	return prefs.field; \
}

#define GET_PREFERENCE_DOUBLE(usegroup, field) \
double  qPref ## usegroup::field () const \
{ \
	return prefs.field; \
}

#define GET_PREFERENCE_ENUM(usegroup, type, field) \
struct type  qPref ## usegroup:: ## field () const \
{ \
	return prefs.field; \
}

#define GET_PREFERENCE_INT(usegroup, field) \
int qPref ## usegroup::field () const \
{ \
	return prefs.field; \
}

#define GET_PREFERENCE_TXT(usegroup, field) \
const QString qPref ## usegroup::field () const \
{ \
	return prefs.field; \
}

//******* Macros to generate set function
#define SET_PREFERENCE_BOOL(usegroup, field) \
void qPref ## usegroup::set_ ## field (bool value) \
{ \
	if (value != prefs.field) { \
		prefs.field = value; \
		disk_ ## field(true); \
		emit field ## _changed(value); \
	} \
}

#define SET_PREFERENCE_DOUBLE(usegroup, field) \
void qPref ## usegroup::set_ ## field (double value) \
{ \
	if (value != prefs.field) { \
		prefs.field = value; \
		disk_ ## field(true); \
		emit field ## _changed(value); \
	} \
}

#define SET_PREFERENCE_ENUM(usegroup, type, field) \
void qPref ## usegroup::set_ ## field (type value) \
{ \
	if (value != prefs.field) { \
		prefs.field = value; \
		disk_ ## field(true); \
		emit field ## _changed(value); \
	} \
}

#define SET_PREFERENCE_INT(usegroup, field) \
void qPref ## usegroup::set_ ## field (int value) \
{ \
	if (value != prefs.field) { \
		prefs.field = value; \
		disk_ ## field(true); \
		emit field ## _changed(value); \
	} \
}

#define SET_PREFERENCE_TXT(usegroup, field) \
void qPref ## usegroup::set_ ## field (const QString& value) \
{ \
	if (value != prefs.field) { \
		COPY_TXT(field, value); \
		disk_ ## field(true); \
		emit field ## _changed(value); \
	} \
}

//******* Macros to generate set/set/loadsync combined
#define HANDLE_PREFERENCE_BOOL(usegroup, name, field) \
GET_PREFERENCE_BOOL(usegroup, field); \
SET_PREFERENCE_BOOL(usegroup, field); \
DISK_LOADSYNC_BOOL(usegroup, name, field);

#define HANDLE_PREFERENCE_DOUBLE(usegroup, name, field) \
GET_PREFERENCE_DOUBLE(usegroup, field); \
SET_PREFERENCE_DOUBLE(usegroup, field); \
DISK_LOADSYNC_DOUBLE(usegroup, name, field);

#define HANDLE_PREFERENCE_ENUM(usegroup, type, name, field) \
GET_PREFERENCE_ENUM(usegroup, type, field); \
SET_PREFERENCE_ENUM(usegroup, type, field); \
DISK_LOADSYNC_ENUM(usegroup, name, type, field);

#define HANDLE_PREFERENCE_INT(usegroup, name, field) \
GET_PREFERENCE_INT(usegroup, field); \
SET_PREFERENCE_INT(usegroup, field); \
DISK_LOADSYNC_INT(usegroup, name, field);

#define HANDLE_PREFERENCE_TXT(usegroup, name, field) \
GET_PREFERENCE_TXT(usegroup, field); \
SET_PREFERENCE_TXT(usegroup, field); \
DISK_LOADSYNC_TXT(usegroup, name, field);

#endif
