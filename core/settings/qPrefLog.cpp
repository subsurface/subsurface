// SPDX-License-Identifier: GPL-2.0
#include "qPrefLog.h"
#include "qPrefPrivate.h"

static const QString group = QStringLiteral("LogSettings");

qPrefLog *qPrefLog::instance()
{
	static qPrefLog *self = new qPrefLog;
	return self;
}

void qPrefLog::loadSync(bool doSync)
{
	disk_default_filename(doSync);
	disk_default_file_behavior(doSync);
	disk_use_default_file(doSync);
	disk_extraEnvironmentalDefault(doSync);
	disk_salinityEditDefault(doSync);
	disk_show_average_depth(doSync);
}

void qPrefLog::set_default_file_behavior(enum def_file_behavior value)
{
	if (value != prefs.default_file_behavior ||
		prefs.default_file_behavior != UNDEFINED_DEFAULT_FILE) {

		if (value == UNDEFINED_DEFAULT_FILE) {
			// undefined, so check if there's a filename set and
			// use that, otherwise go with no default file
			prefs.default_file_behavior = prefs.default_filename.empty() ? NO_DEFAULT_FILE : LOCAL_DEFAULT_FILE;
		} else {
			prefs.default_file_behavior = value;
		}
		disk_default_file_behavior(true);
		emit instance()->default_file_behaviorChanged(value);
	}
}
void qPrefLog::disk_default_file_behavior(bool doSync)
{
	if (doSync) {
		qPrefPrivate::propSetValue(keyFromGroupAndName(group, "default_file_behavior"), prefs.default_file_behavior, default_prefs.default_file_behavior);
	} else {
		prefs.default_file_behavior = (enum def_file_behavior)qPrefPrivate::propValue(keyFromGroupAndName(group, "default_file_behavior"), default_prefs.default_file_behavior).toInt();
		if (prefs.default_file_behavior == UNDEFINED_DEFAULT_FILE)
			// undefined, so check if there's a filename set and
			// use that, otherwise go with no default file
			prefs.default_file_behavior = prefs.default_filename.empty() ? NO_DEFAULT_FILE : LOCAL_DEFAULT_FILE;
	}
}

HANDLE_PREFERENCE_TXT(Log, "default_filename", default_filename);

HANDLE_PREFERENCE_BOOL(Log, "extraEnvironmentalDefault", extraEnvironmentalDefault);

HANDLE_PREFERENCE_BOOL(Log, "use_default_file", use_default_file);

HANDLE_PREFERENCE_BOOL(Log, "salinityEditDefault", salinityEditDefault);

HANDLE_PREFERENCE_BOOL(Log, "show_average_depth", show_average_depth);
