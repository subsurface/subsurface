// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFPRIVATE_H
#define QPREFPRIVATE_H

// Header used by all qPref<class> implementations to avoid duplicating code
#include "qPref.h"
#include <QSettings>
#include <QVariant>
#include <QObject>
#include "core/qthelper.h"

// implementation class of the interface classes
class qPrefPrivate : public QObject {
    Q_OBJECT

public:
	friend class qPrefAnimations;
	friend class qPrefCloudStorage;
	friend class qPrefDisplay;

private:
    static qPrefPrivate *instance();

	QSettings setting;

	// Helper functions
	static void copy_txt(const char **name, const QString& string);

    qPrefPrivate(QObject *parent = NULL);
};



//******* Macros to generate disk function
#define DISK_LOADSYNC_BOOL(usegroup, name, field) \
void qPref ## usegroup::disk_ ## field(bool doSync) \
{ \
	if (doSync)	\
		qPrefPrivate::instance()->setting.setValue(group + name, prefs.field); \
	else \
		prefs.field = qPrefPrivate::instance()->setting.value(group + name, default_prefs.field).toBool(); \
}

#define DISK_LOADSYNC_DOUBLE(usegroup, name, field) \
void qPref ## usegroup::disk_ ## field(bool doSync) \
{ \
	if (doSync)	\
		qPrefPrivate::instance()->setting.setValue(group + name, prefs.field); \
	else \
		prefs.field = qPrefPrivate::instance()->setting.value(group + name, default_prefs.field).toDouble(); \
}

#define DISK_LOADSYNC_ENUM(usegroup, name, type, field) \
void qPref ## usegroup::disk_ ## field(bool doSync) \
{ \
	if (doSync)	\
		qPrefPrivate::instance()->setting.setValue(group + name, prefs.field); \
	else \
		prefs.field = (enum type)qPrefPrivate::instance()->setting.value(group + name, default_prefs.field).toInt(); \
}

#define DISK_LOADSYNC_INT(usegroup, name, field) \
void qPref ## usegroup::disk_ ## field(bool doSync) \
{ \
	if (doSync) \
		qPrefPrivate::instance()->setting.setValue(group + name, prefs.field); \
	else \
		prefs.field = qPrefPrivate::instance()->setting.value(group + name, default_prefs.field).toInt(); \
}

#define DISK_LOADSYNC_INT_DEF(usegroup, name, field, defval) \
void qPref ## usegroup::disk_ ## field(bool doSync) \
{ \
	if (doSync)	\
		qPrefPrivate::instance()->setting.setValue(group + name, prefs.field); \
	else \
		prefs.field = qPrefPrivate::instance()->setting.value(group + name, defval).toInt(); \
}

#define DISK_LOADSYNC_TXT(usegroup, name, field) \
void qPref ## usegroup::disk_ ## field(bool doSync) \
{ \
	if (doSync)	\
		qPrefPrivate::instance()->setting.setValue(group + name, prefs.field); \
	else \
		prefs.field = copy_qstring(qPrefPrivate::instance()->setting.value(group + name, default_prefs.field).toString()); \
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
		qPrefPrivate::instance()->copy_txt(&prefs.field, value); \
		disk_ ## field(true); \
		emit field ## _changed(value); \
	} \
}

//******* Macros to generate set/set/loadsync combined
#define HANDLE_PREFERENCE_BOOL(usegroup, name, field) \
SET_PREFERENCE_BOOL(usegroup, field); \
DISK_LOADSYNC_BOOL(usegroup, name, field);

#define HANDLE_PREFERENCE_DOUBLE(usegroup, name, field) \
SET_PREFERENCE_DOUBLE(usegroup, field); \
DISK_LOADSYNC_DOUBLE(usegroup, name, field);

#define HANDLE_PREFERENCE_ENUM(usegroup, type, name, field) \
SET_PREFERENCE_ENUM(usegroup, type, field); \
DISK_LOADSYNC_ENUM(usegroup, name, type, field);

#define HANDLE_PREFERENCE_INT(usegroup, name, field) \
SET_PREFERENCE_INT(usegroup, field); \
DISK_LOADSYNC_INT(usegroup, name, field);

#define HANDLE_PREFERENCE_TXT(usegroup, name, field) \
SET_PREFERENCE_TXT(usegroup, field); \
DISK_LOADSYNC_TXT(usegroup, name, field);

#endif
