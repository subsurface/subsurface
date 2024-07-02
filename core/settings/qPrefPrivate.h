// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFPRIVATE_H
#define QPREFPRIVATE_H

// Header used by all qPref<class> implementations to avoid duplicating code
#include "core/qthelper.h"
#include "qPref.h"

#include <QObject>
#include <QDebug>
#include <QVariant>


// implementation class of the interface classes
class qPrefPrivate {

public:
	// Helper functions
	static void propSetValue(const QString &key, const QVariant &value, const QVariant &defaultValue);
	static void propSetValue(const QString &key, const std::string &value, const std::string &defaultValue);
	static QVariant propValue(const QString &key, const QVariant &defaultValue);
	static QVariant propValue(const QString &key, const std::string &defaultValue);

private:
	qPrefPrivate() {}
};

// helper function to ensure there's a '/' between group and name
extern QString keyFromGroupAndName(QString group, QString name);

//******* Macros to generate disk function
#define DISK_LOADSYNC_BOOL_EXT(usegroup, name, field, usestruct) \
	void qPref##usegroup::disk_##field(bool doSync) \
	{ \
		static bool current_state; \
		if (doSync) { \
			if (current_state != prefs.usestruct field) { \
				current_state = prefs.usestruct field; \
				qPrefPrivate::propSetValue(keyFromGroupAndName(group, name), prefs.usestruct field, default_prefs.usestruct field); \
			} \
		} else { \
			prefs.usestruct field = qPrefPrivate::propValue(keyFromGroupAndName(group, name), default_prefs.usestruct field).toBool(); \
			current_state = prefs.usestruct field; \
		} \
	}
#define DISK_LOADSYNC_BOOL(usegroup, name, field) \
	DISK_LOADSYNC_BOOL_EXT(usegroup, name, field, )

#define DISK_LOADSYNC_DOUBLE_EXT(usegroup, name, field, usestruct) \
	void qPref##usegroup::disk_##field(bool doSync) \
	{ \
		static double current_state; \
		if (doSync) { \
			if (current_state != prefs.usestruct field) { \
				current_state = prefs.usestruct field; \
				qPrefPrivate::propSetValue(keyFromGroupAndName(group, name), prefs.usestruct field, default_prefs.usestruct field); \
			} \
		} else { \
			prefs.usestruct field = qPrefPrivate::propValue(keyFromGroupAndName(group, name), default_prefs.usestruct field).toDouble(); \
			current_state = prefs.usestruct field; \
		} \
	}
#define DISK_LOADSYNC_DOUBLE(usegroup, name, field) \
	DISK_LOADSYNC_DOUBLE_EXT(usegroup, name, field, )

#define DISK_LOADSYNC_ENUM_EXT(usegroup, name, type, field, usestruct) \
	void qPref##usegroup::disk_##field(bool doSync) \
	{ \
		static enum type current_state; \
		if (doSync) { \
			if (current_state != prefs.usestruct field) { \
				current_state = prefs.usestruct field; \
				qPrefPrivate::propSetValue(keyFromGroupAndName(group, name), prefs.usestruct field, default_prefs.usestruct field); \
			} \
		} else { \
			prefs.usestruct field = (enum type)qPrefPrivate::propValue(keyFromGroupAndName(group, name), default_prefs.usestruct field).toInt(); \
			current_state = prefs.usestruct field; \
		} \
	}
#define DISK_LOADSYNC_ENUM(usegroup, name, type, field) \
	DISK_LOADSYNC_ENUM_EXT(usegroup, name, type, field, )

#define DISK_LOADSYNC_INT_EXT(usegroup, name, field, usestruct) \
	void qPref##usegroup::disk_##field(bool doSync) \
	{ \
		static int current_state; \
		if (doSync) { \
			if (current_state != prefs.usestruct field) { \
				current_state = prefs.usestruct field; \
				qPrefPrivate::propSetValue(keyFromGroupAndName(group, name), prefs.usestruct field, default_prefs.usestruct field); \
			} \
		} else { \
			prefs.usestruct field = qPrefPrivate::propValue(keyFromGroupAndName(group, name), default_prefs.usestruct field).toInt(); \
			current_state = prefs.usestruct field; \
		} \
	}
#define DISK_LOADSYNC_INT(usegroup, name, field) \
	DISK_LOADSYNC_INT_EXT(usegroup, name, field, )

#define DISK_LOADSYNC_INT_DEF_EXT(usegroup, name, field, defval, usestruct) \
	void qPref##usegroup::disk_##field(bool doSync) \
	{ \
		static int current_state; \
		if (doSync) { \
			if (current_state != prefs.usestruct field) { \
				current_state = prefs.usestruct field; \
				qPrefPrivate::propSetValue(keyFromGroupAndName(group, name), prefs.usestruct field, default_prefs.usestruct field); \
			} \
		} else { \
			prefs.usestruct field = qPrefPrivate::propValue(keyFromGroupAndName(group, name), defval).toInt(); \
			current_state = prefs.usestruct field; \
		} \
	}
#define DISK_LOADSYNC_INT_DEF(usegroup, name, field, defval) \
	DISK_LOADSYNC_INT_DEF_EXT(usegroup, name, field, defval, )

#define DISK_LOADSYNC_STRUCT_EXT(usegroup, name, field, var, usestruct)  \
	void qPref##usegroup::disk_##field(bool doSync) \
	{ \
		static int current_state; \
		if (doSync) { \
			if (current_state != prefs.usestruct field . var) { \
				current_state = prefs.usestruct field . var; \
				qPrefPrivate::propSetValue(keyFromGroupAndName(group, name), prefs.usestruct field . var, default_prefs.usestruct field . var); \
			} \
		} else { \
			prefs.usestruct field . var = qPrefPrivate::propValue(keyFromGroupAndName(group, name), default_prefs.usestruct field . var).toInt(); \
			current_state = prefs.usestruct field . var; \
		} \
	}
#define DISK_LOADSYNC_STRUCT(usegroup, name, field, var) \
	DISK_LOADSYNC_STRUCT_EXT(usegroup, name, field, var, )

#define DISK_LOADSYNC_TXT_EXT(usegroup, name, field, usestruct) \
	void qPref##usegroup::disk_##field(bool doSync) \
	{ \
		static std::string current_state; \
		if (doSync) { \
			if (current_state != prefs.usestruct field) { \
				current_state = prefs.usestruct field; \
				qPrefPrivate::propSetValue(keyFromGroupAndName(group, name), QString::fromStdString(prefs.usestruct field), QString::fromStdString(default_prefs.usestruct field)); \
			} \
		} else { \
			prefs.usestruct field = qPrefPrivate::propValue(keyFromGroupAndName(group, name), QString::fromStdString(default_prefs.usestruct field)).toString().toStdString(); \
			current_state = prefs.usestruct field; \
		} \
	}
#define DISK_LOADSYNC_TXT_EXT_ALT(usegroup, name, field, usestruct, alt) \
	void qPref##usegroup::disk_##field##alt(bool doSync) \
	{ \
		static std::string current_state; \
		if (doSync) { \
			if (current_state != prefs.usestruct ## alt .field) { \
				current_state = prefs.usestruct ## alt .field; \
				qPrefPrivate::propSetValue(keyFromGroupAndName(group, name), QString::fromStdString(prefs.usestruct ##alt .field), QString::fromStdString(default_prefs.usestruct ##alt .field)); \
			} \
		} else { \
			prefs.usestruct ##alt .field = qPrefPrivate::propValue(keyFromGroupAndName(group, name), QString::fromStdString(default_prefs.usestruct ##alt .field)).toString().toStdString(); \
			current_state = prefs.usestruct ##alt .field; \
		} \
	}
#define DISK_LOADSYNC_TXT(usegroup, name, field) \
	DISK_LOADSYNC_TXT_EXT(usegroup, name, field, )

//******* Macros to generate set function
#define SET_PREFERENCE_BOOL_EXT(usegroup, field, usestruct) \
	void qPref##usegroup::set_##field(bool value) \
	{ \
		if (value != prefs.usestruct field) { \
			prefs.usestruct field = value; \
			disk_##field(true); \
			emit instance()->field##Changed(value); \
		} \
	}
#define SET_PREFERENCE_BOOL(usegroup, field) \
	SET_PREFERENCE_BOOL_EXT(usegroup, field, )

#define SET_PREFERENCE_DOUBLE_EXT(usegroup, field, usestruct) \
	void qPref##usegroup::set_##field(double value) \
	{ \
		if (value != prefs.usestruct field) { \
			prefs.usestruct field = value; \
			disk_##field(true); \
			emit instance()->field##Changed(value); \
		} \
	}
#define SET_PREFERENCE_DOUBLE(usegroup, field) \
	SET_PREFERENCE_DOUBLE_EXT(usegroup, field, )

#define SET_PREFERENCE_ENUM_EXT(usegroup, type, field, usestruct) \
	void qPref##usegroup::set_##field(type value) \
	{ \
		if (value != prefs.usestruct field) { \
			prefs.usestruct field = value; \
			disk_##field(true); \
			emit instance()->field##Changed(value); \
		} \
	}
#define SET_PREFERENCE_ENUM(usegroup, type, field) \
	SET_PREFERENCE_ENUM_EXT(usegroup, type, field, )

#define SET_PREFERENCE_INT_EXT(usegroup, field, usestruct) \
	void qPref##usegroup::set_##field(int value) \
	{ \
		if (value != prefs.usestruct field) { \
			prefs.usestruct field = value; \
			disk_##field(true); \
			emit instance()->field##Changed(value); \
		} \
	}
#define SET_PREFERENCE_INT(usegroup, field) \
	SET_PREFERENCE_INT_EXT(usegroup, field, )

#define SET_PREFERENCE_STRUCT_EXT(usegroup, field, var, usestruct) \
	void qPref##usegroup::set_##field(int value) \
	{ \
		if (value != prefs.usestruct field . var) { \
			prefs.usestruct field . var = value; \
			disk_##field(true); \
			emit instance()->field##Changed(value); \
		} \
	}
#define SET_PREFERENCE_STRUCT(usegroup, type, field, var) \
	SET_PREFERENCE_STRUCT_EXT(usegroup, type, field, var, )

#define SET_PREFERENCE_TXT_EXT(usegroup, field, usestruct) \
	void qPref##usegroup::set_##field(const QString &value) \
	{ \
		if (value.toStdString() != prefs.usestruct field) { \
			prefs.usestruct field = value.toStdString(); \
			disk_##field(true); \
			emit instance()->field##Changed(value); \
		} \
	}

#define SET_PREFERENCE_TXT_EXT_ALT(usegroup, field, usestruct, alt) \
	void qPref##usegroup::set_##field##alt(const QString &value) \
	{ \
		if (value.toStdString() != prefs.usestruct ##alt .field) { \
			prefs.usestruct ##alt .field = value.toStdString(); \
			disk_##field##alt(true); \
			emit instance()->field##alt##Changed(value); \
		} \
	}

#define SET_PREFERENCE_TXT(usegroup, field) \
	SET_PREFERENCE_TXT_EXT(usegroup, field, )

//******* Macros to generate set/set/loadsync combined
#define HANDLE_PREFERENCE_BOOL_EXT(usegroup, name, field, usestruct) \
	SET_PREFERENCE_BOOL_EXT(usegroup, field, usestruct); \
	DISK_LOADSYNC_BOOL_EXT(usegroup, name, field, usestruct);
#define HANDLE_PREFERENCE_BOOL(usegroup, name, field) \
	HANDLE_PREFERENCE_BOOL_EXT(usegroup, name, field, )

#define HANDLE_PREFERENCE_DOUBLE_EXT(usegroup, name, field, usestruct) \
	SET_PREFERENCE_DOUBLE_EXT(usegroup, field, usestruct); \
	DISK_LOADSYNC_DOUBLE_EXT(usegroup, name, field, usestruct);
#define HANDLE_PREFERENCE_DOUBLE(usegroup, name, field) \
	HANDLE_PREFERENCE_DOUBLE_EXT(usegroup, name, field, )

#define HANDLE_PREFERENCE_ENUM_EXT(usegroup, type, name, field, usestruct) \
	SET_PREFERENCE_ENUM_EXT(usegroup, type, field, usestruct); \
	DISK_LOADSYNC_ENUM_EXT(usegroup, name, type, field, usestruct);
#define HANDLE_PREFERENCE_ENUM(usegroup, type, name, field) \
	HANDLE_PREFERENCE_ENUM_EXT(usegroup, type, name, field, )

#define HANDLE_PREFERENCE_INT_EXT(usegroup, name, field, usestruct) \
	SET_PREFERENCE_INT_EXT(usegroup, field, usestruct); \
	DISK_LOADSYNC_INT_EXT(usegroup, name, field, usestruct);
#define HANDLE_PREFERENCE_INT(usegroup, name, field) \
	HANDLE_PREFERENCE_INT_EXT(usegroup, name, field, )

#define HANDLE_PREFERENCE_INT_DEF_EXT(usegroup, name, field, defval, usestruct) \
	SET_PREFERENCE_INT_EXT(usegroup, field, usestruct); \
	DISK_LOADSYNC_INT_DEF_EXT(usegroup, name, field, defval, usestruct);
#define HANDLE_PREFERENCE_INT_DEF(usegroup, name, field, defval) \
	HANDLE_PREFERENCE_INT_DEF_EXT(usegroup, name, field, defval, )

#define HANDLE_PREFERENCE_STRUCT_EXT(usegroup, name, field, var, usestruct) \
	SET_PREFERENCE_STRUCT_EXT(usegroup, field, var, usestruct) \
	DISK_LOADSYNC_STRUCT_EXT(usegroup, name, field, var, usestruct)
#define HANDLE_PREFERENCE_STRUCT(usegroup, name, field, var) \
	HANDLE_PREFERENCE_STRUCT_EXT(usegroup, name, field, var, )

#define HANDLE_PREFERENCE_TXT_EXT(usegroup, name, field, usestruct) \
	SET_PREFERENCE_TXT_EXT(usegroup, field, usestruct); \
	DISK_LOADSYNC_TXT_EXT(usegroup, name, field, usestruct);
#define HANDLE_PREFERENCE_TXT(usegroup, name, field) \
	HANDLE_PREFERENCE_TXT_EXT(usegroup, name, field, )

#define HANDLE_PREFERENCE_TXT_EXT_ALT(usegroup, name, field, usestruct, alt) \
	SET_PREFERENCE_TXT_EXT_ALT(usegroup, field, usestruct, alt); \
	DISK_LOADSYNC_TXT_EXT_ALT(usegroup, name, field, usestruct, alt);

#define HANDLE_PROP_QPOINTF(useclass, name, field) \
	void qPref##useclass::set_##field(const QPointF& value) \
	{ \
		if (value != st_##field) { \
			st_##field = value; \
			qPrefPrivate::propSetValue(name, st_##field, QPointF()); \
			emit instance()->field##Changed(value); \
		} \
	} \
	void qPref##useclass::load_##field() \
	{ \
		st_##field = qPrefPrivate::propValue(name, st_##field##_default).toPointF(); \
	}

#define HANDLE_PROP_QSTRING(useclass, name, field) \
	void qPref##useclass::set_##field(const QString& value) \
	{ \
		if (value != st_##field) { \
			st_##field = value; \
			qPrefPrivate::propSetValue(name, st_##field, ""); \
			emit instance()->field##Changed(value); \
		} \
	} \
	void qPref##useclass::load_##field() \
	{ \
		st_##field = qPrefPrivate::propValue(name, st_##field##_default).toString(); \
	}

#define HANDLE_PROP_BOOL(useclass, name, field) \
	void qPref##useclass::set_##field(bool value) \
	{ \
		if (value != st_##field) { \
			st_##field = value; \
			qPrefPrivate::propSetValue(name, st_##field, false); \
			emit instance()->field##Changed(value); \
		} \
	} \
	void qPref##useclass::load_##field() \
	{ \
		st_##field = qPrefPrivate::propValue(name, st_##field##_default).toBool(); \
	}

#define HANDLE_PROP_DOUBLE(useclass, name, field) \
	void qPref##useclass::set_##field(double value) \
	{ \
		if (value != st_##field) { \
			st_##field = value; \
			qPrefPrivate::propSetValue(name, st_##field, 0.0); \
			emit instance()->field##Changed(value); \
		} \
	} \
	void qPref##useclass::load_##field() \
	{ \
		st_##field = qPrefPrivate::propValue(name, st_##field##_default).toDouble(); \
	}

#define HANDLE_PROP_INT(useclass, name, field) \
	void qPref##useclass::set_##field(int value) \
	{ \
		if (value != st_##field) { \
			st_##field = value; \
			qPrefPrivate::propSetValue(name, st_##field, 0); \
			emit instance()->field##Changed(value); \
		} \
	} \
	void qPref##useclass::load_##field() \
	{ \
		st_##field = qPrefPrivate::propValue(name, st_##field##_default).toInt(); \
	}

#define HANDLE_PROP_QBYTEARRAY(useclass, name, field) \
	void qPref##useclass::set_##field(const QByteArray& value) \
	{ \
		if (value != st_##field) { \
			st_##field = value; \
			qPrefPrivate::propSetValue(name, st_##field, QByteArray()); \
			emit instance()->field##Changed(value); \
		} \
	} \
	void qPref##useclass::load_##field() \
	{ \
		st_##field = qPrefPrivate::propValue(name, st_##field##_default).toByteArray(); \
	}
#endif
