// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFPRIVATE_H
#define QPREFPRIVATE_H

// Header used by all qPref<class> implementations to avoid duplicating code
#include "core/qthelper.h"
#include "qPref.h"
#include <QObject>
#include <QSettings>
#include <QVariant>

// implementation class of the interface classes
class qPrefPrivate : public QObject {
	Q_OBJECT

public:
	friend class qPrefAnimations;
	friend class qPrefCloudStorage;
	friend class qPrefDisplay;
	friend class qPrefDiveComputer;
	friend class qPrefDivePlanner;
	friend class qPrefFacebook;
	friend class qPrefLanguage;
	friend class qPrefLocationService;
	friend class qPrefProxy;
	friend class qPrefTechnicalDetails;
	friend class qPrefUnits;
	friend class qPrefUpdateManager;

private:
	static qPrefPrivate *instance();

	QSettings setting;

	// Helper functions
	static void copy_txt(const char **name, const QString &string);

	qPrefPrivate(QObject *parent = NULL);
};


//******* Macros to generate disk function
#define DISK_LOADSYNC_BOOL_EXT(usegroup, name, field, usestruct)                                                                               \
	void qPref##usegroup::disk_##field(bool doSync)                                                                                        \
	{                                                                                                                                      \
		if (doSync)                                                                                                                    \
			qPrefPrivate::instance()->setting.setValue(group + name, prefs.usestruct field);                                       \
		else                                                                                                                           \
			prefs.usestruct field = qPrefPrivate::instance()->setting.value(group + name, default_prefs.usestruct field).toBool(); \
	}
#define DISK_LOADSYNC_BOOL(usegroup, name, field) \
	DISK_LOADSYNC_BOOL_EXT(usegroup, name, field, )

#define DISK_LOADSYNC_DOUBLE_EXT(usegroup, name, field, usestruct)                                                                               \
	void qPref##usegroup::disk_##field(bool doSync)                                                                                          \
	{                                                                                                                                        \
		if (doSync)                                                                                                                      \
			qPrefPrivate::instance()->setting.setValue(group + name, prefs.usestruct field);                                         \
		else                                                                                                                             \
			prefs.usestruct field = qPrefPrivate::instance()->setting.value(group + name, default_prefs.usestruct field).toDouble(); \
	}
#define DISK_LOADSYNC_DOUBLE(usegroup, name, field) \
	DISK_LOADSYNC_DOUBLE_EXT(usegroup, name, field, )

#define DISK_LOADSYNC_ENUM_EXT(usegroup, name, type, field, usestruct)                                                                                   \
	void qPref##usegroup::disk_##field(bool doSync)                                                                                                  \
	{                                                                                                                                                \
		if (doSync)                                                                                                                              \
			qPrefPrivate::instance()->setting.setValue(group + name, prefs.usestruct field);                                                 \
		else                                                                                                                                     \
			prefs.usestruct field = (enum type)qPrefPrivate::instance()->setting.value(group + name, default_prefs.usestruct field).toInt(); \
	}
#define DISK_LOADSYNC_ENUM(usegroup, name, type, field) \
	DISK_LOADSYNC_ENUM_EXT(usegroup, name, type, field, )

#define DISK_LOADSYNC_INT_EXT(usegroup, name, field, usestruct)                                                                               \
	void qPref##usegroup::disk_##field(bool doSync)                                                                                       \
	{                                                                                                                                     \
		if (doSync)                                                                                                                   \
			qPrefPrivate::instance()->setting.setValue(group + name, prefs.usestruct field);                                      \
		else                                                                                                                          \
			prefs.usestruct field = qPrefPrivate::instance()->setting.value(group + name, default_prefs.usestruct field).toInt(); \
	}
#define DISK_LOADSYNC_INT(usegroup, name, field) \
	DISK_LOADSYNC_INT_EXT(usegroup, name, field, )

#define DISK_LOADSYNC_INT_DEF_EXT(usegroup, name, field, defval, usestruct)                                            \
	void qPref##usegroup::disk_##field(bool doSync)                                                                \
	{                                                                                                              \
		if (doSync)                                                                                            \
			qPrefPrivate::instance()->setting.setValue(group + name, prefs.usestruct field);               \
		else                                                                                                   \
			prefs.usestruct field = qPrefPrivate::instance()->setting.value(group + name, defval).toInt(); \
	}
#define DISK_LOADSYNC_INT_DEF(usegroup, name, field, defval) \
	DISK_LOADSYNC_INT_DEF_EXT(usegroup, name, field, defval, )

#define DISK_LOADSYNC_STRUCT_EXT(usegroup, name, field, var, usestruct)                                                                                   \
	void qPref##usegroup::disk_##field(bool doSync)                                                                                                  \
	{                                                                                                                                                \
		if (doSync)                                                                                                                              \
			qPrefPrivate::instance()->setting.setValue(group + name, prefs.usestruct field . var);                                                 \
		else                                                                                                                                     \
			prefs.usestruct field . var = qPrefPrivate::instance()->setting.value(group + name, default_prefs.usestruct field . var).toInt(); \
	}
#define DISK_LOADSYNC_STRUCT(usegroup, name, field, var) \
	DISK_LOADSYNC_STRUCT_EXT(usegroup, name, field, var, )

#define DISK_LOADSYNC_TXT_EXT(usegroup, name, field, usestruct)                                                                                                \
	void qPref##usegroup::disk_##field(bool doSync)                                                                                                        \
	{                                                                                                                                                      \
		if (doSync)                                                                                                                                    \
			qPrefPrivate::instance()->setting.setValue(group + name, prefs.usestruct field);                                                       \
		else                                                                                                                                           \
			prefs.usestruct field = copy_qstring(qPrefPrivate::instance()->setting.value(group + name, default_prefs.usestruct field).toString()); \
	}
#define DISK_LOADSYNC_TXT(usegroup, name, field) \
	DISK_LOADSYNC_TXT_EXT(usegroup, name, field, )

//******* Macros to generate set function
#define SET_PREFERENCE_BOOL_EXT(usegroup, field, usestruct) \
	void qPref##usegroup::set_##field(bool value)       \
	{                                                   \
		if (value != prefs.usestruct field) {       \
			prefs.usestruct field = value;      \
			disk_##field(true);                 \
			emit field##_changed(value);        \
		}                                           \
	}
#define SET_PREFERENCE_BOOL(usegroup, field) \
	SET_PREFERENCE_BOOL_EXT(usegroup, field, )

#define SET_PREFERENCE_DOUBLE_EXT(usegroup, field, usestruct) \
	void qPref##usegroup::set_##field(double value)       \
	{                                                     \
		if (value != prefs.usestruct field) {         \
			prefs.usestruct field = value;        \
			disk_##field(true);                   \
			emit field##_changed(value);          \
		}                                             \
	}
#define SET_PREFERENCE_DOUBLE(usegroup, field) \
	SET_PREFERENCE_DOUBLE_EXT(usegroup, field, )

#define SET_PREFERENCE_ENUM_EXT(usegroup, type, field, usestruct) \
	void qPref##usegroup::set_##field(type value)             \
	{                                                         \
		if (value != prefs.usestruct field) {             \
			prefs.usestruct field = value;            \
			disk_##field(true);                       \
			emit field##_changed(value);              \
		}                                                 \
	}
#define SET_PREFERENCE_ENUM(usegroup, type, field) \
	SET_PREFERENCE_ENUM_EXT(usegroup, type, field, )

#define SET_PREFERENCE_INT_EXT(usegroup, field, usestruct) \
	void qPref##usegroup::set_##field(int value)       \
	{                                                  \
		if (value != prefs.usestruct field) {      \
			prefs.usestruct field = value;     \
			disk_##field(true);                \
			emit field##_changed(value);       \
		}                                          \
	}
#define SET_PREFERENCE_INT(usegroup, field) \
	SET_PREFERENCE_INT_EXT(usegroup, field, )

#define SET_PREFERENCE_STRUCT_EXT(usegroup, type, field, var, usestruct) \
	void qPref##usegroup::set_##field(type value)             \
	{                                                         \
		if (value. var != prefs.usestruct field . var) {             \
			prefs.usestruct field . var = value . var;            \
			disk_##field(true);                       \
			emit field##_changed(value);              \
		}                                                 \
	}
#define SET_PREFERENCE_STRUCT(usegroup, type, field, var) \
	SET_PREFERENCE_STRUCT_EXT(usegroup, type, field, var, )

#define SET_PREFERENCE_TXT_EXT(usegroup, field, usestruct)                                 \
	void qPref##usegroup::set_##field(const QString &value)                            \
	{                                                                                  \
		if (value != prefs.usestruct field) {                                      \
			qPrefPrivate::instance()->copy_txt(&prefs.usestruct field, value); \
			disk_##field(true);                                                \
			emit field##_changed(value);                                       \
		}                                                                          \
	}
#define SET_PREFERENCE_TXT(usegroup, field) \
	SET_PREFERENCE_TXT_EXT(usegroup, field, )

//******* Macros to generate set/set/loadsync combined
#define HANDLE_PREFERENCE_BOOL_EXT(usegroup, name, field, usestruct) \
	SET_PREFERENCE_BOOL_EXT(usegroup, field, usestruct);         \
	DISK_LOADSYNC_BOOL_EXT(usegroup, name, field, usestruct);
#define HANDLE_PREFERENCE_BOOL(usegroup, name, field) \
	HANDLE_PREFERENCE_BOOL_EXT(usegroup, name, field, )

#define HANDLE_PREFERENCE_DOUBLE_EXT(usegroup, name, field, usestruct) \
	SET_PREFERENCE_DOUBLE_EXT(usegroup, field, usestruct);         \
	DISK_LOADSYNC_DOUBLE_EXT(usegroup, name, field, usestruct);
#define HANDLE_PREFERENCE_DOUBLE(usegroup, name, field) \
	HANDLE_PREFERENCE_DOUBLE_EXT(usegroup, name, field, )

#define HANDLE_PREFERENCE_ENUM_EXT(usegroup, type, name, field, usestruct) \
	SET_PREFERENCE_ENUM_EXT(usegroup, type, field, usestruct);         \
	DISK_LOADSYNC_ENUM_EXT(usegroup, name, type, field, usestruct);
#define HANDLE_PREFERENCE_ENUM(usegroup, type, name, field) \
	HANDLE_PREFERENCE_ENUM_EXT(usegroup, type, name, field, )

#define HANDLE_PREFERENCE_INT_EXT(usegroup, name, field, usestruct) \
	SET_PREFERENCE_INT_EXT(usegroup, field, usestruct);         \
	DISK_LOADSYNC_INT_EXT(usegroup, name, field, usestruct);
#define HANDLE_PREFERENCE_INT(usegroup, name, field) \
	HANDLE_PREFERENCE_INT_EXT(usegroup, name, field, )

#define HANDLE_PREFERENCE_INT_DEF_EXT(usegroup, name, field, defval, usestruct) \
	SET_PREFERENCE_INT_EXT(usegroup, field, usestruct);         \
	DISK_LOADSYNC_INT_DEF_EXT(usegroup, name, field, defval, usestruct);
#define HANDLE_PREFERENCE_INT_DEF(usegroup, name, field, defval) \
	HANDLE_PREFERENCE_INT_DEF_EXT(usegroup, name, field, defval, )

#define HANDLE_PREFERENCE_STRUCT_EXT(usegroup, type, name, field, var, usestruct) \
	SET_PREFERENCE_STRUCT_EXT(usegroup, type, field, var, usestruct) \
	DISK_LOADSYNC_STRUCT_EXT(usegroup, name, field, var, usestruct)
#define HANDLE_PREFERENCE_STRUCT(usegroup, type, name, field, var) \
	HANDLE_PREFERENCE_STRUCT_EXT(usegroup, type, name, field, var, )

#define HANDLE_PREFERENCE_TXT_EXT(usegroup, name, field, usestruct) \
	SET_PREFERENCE_TXT_EXT(usegroup, field, usestruct);         \
	DISK_LOADSYNC_TXT_EXT(usegroup, name, field, usestruct);
#define HANDLE_PREFERENCE_TXT(usegroup, name, field) \
	HANDLE_PREFERENCE_TXT_EXT(usegroup, name, field, )

#endif
