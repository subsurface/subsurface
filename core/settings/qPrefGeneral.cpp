// SPDX-License-Identifier: GPL-2.0
#include "qPrefGeneral.h"
#include "qPrefPrivate.h"


static const QString group = QStringLiteral("GeneralSettings");

QString qPrefGeneral::st_diveshareExport_uid;
static const QString st_diveshareExport_uid_default;

bool qPrefGeneral::st_diveshareExport_private;
static const bool st_diveshareExport_private_default = false;

qPrefGeneral::qPrefGeneral(QObject *parent) : QObject(parent)
{
}

qPrefGeneral *qPrefGeneral::instance()
{
	static qPrefGeneral *self = new qPrefGeneral;
	return self;
}


void qPrefGeneral::loadSync(bool doSync)
{
	disk_auto_recalculate_thumbnails(doSync);
	disk_auto_recalculate_thumbnails(doSync);
	disk_default_cylinder(doSync);
	disk_default_filename(doSync);
	disk_default_file_behavior(doSync);
	disk_defaultsetpoint(doSync);
	disk_extract_video_thumbnails(doSync);
	disk_extract_video_thumbnails_position(doSync);
	disk_ffmpeg_executable(doSync);
	disk_o2consumption(doSync);
	disk_pscr_ratio(doSync);
	disk_use_default_file(doSync);
	disk_filterFullTextNotes(doSync);
	disk_filterCaseSensitive(doSync);

	if (!doSync) {
		load_diveshareExport_uid();
		load_diveshareExport_private();
	}
}

HANDLE_PREFERENCE_BOOL(General, "auto_recalculate_thumbnails", auto_recalculate_thumbnails);

HANDLE_PREFERENCE_TXT(General, "default_cylinder", default_cylinder);

HANDLE_PREFERENCE_TXT(General, "default_filename", default_filename);


void qPrefGeneral::set_default_file_behavior(enum def_file_behavior value)
{
	if (value != prefs.default_file_behavior ||
		prefs.default_file_behavior != UNDEFINED_DEFAULT_FILE) {

	if (value == UNDEFINED_DEFAULT_FILE) {
		// undefined, so check if there's a filename set and
		// use that, otherwise go with no default file
		prefs.default_file_behavior = QString(prefs.default_filename).isEmpty() ? NO_DEFAULT_FILE : LOCAL_DEFAULT_FILE;
	} else {
		prefs.default_file_behavior = value;
	}
	disk_default_file_behavior(true);
	emit instance()->default_file_behaviorChanged(value);
	}
}
void qPrefGeneral::disk_default_file_behavior(bool doSync)
{
	if (doSync) {
		qPrefPrivate::propSetValue(keyFromGroupAndName(group, "default_file_behavior"), prefs.default_file_behavior, default_prefs.default_file_behavior);
	} else {
		prefs.default_file_behavior = (enum def_file_behavior)qPrefPrivate::propValue(keyFromGroupAndName(group, "default_file_behavior"), default_prefs.default_file_behavior).toInt();
		if (prefs.default_file_behavior == UNDEFINED_DEFAULT_FILE)
			// undefined, so check if there's a filename set and
			// use that, otherwise go with no default file
			prefs.default_file_behavior = QString(prefs.default_filename).isEmpty() ? NO_DEFAULT_FILE : LOCAL_DEFAULT_FILE;
	}
}

HANDLE_PREFERENCE_INT(General, "defaultsetpoint", defaultsetpoint);

HANDLE_PREFERENCE_BOOL(General, "extract_video_thumbnails", extract_video_thumbnails);

HANDLE_PREFERENCE_INT(General, "extract_video_thumbnails_position", extract_video_thumbnails_position);

HANDLE_PREFERENCE_TXT(General, "ffmpeg_executable", ffmpeg_executable);

HANDLE_PREFERENCE_INT(General, "o2consumption", o2consumption);

HANDLE_PREFERENCE_INT(General, "pscr_ratio", pscr_ratio);

HANDLE_PREFERENCE_BOOL(General, "use_default_file", use_default_file);

HANDLE_PROP_QSTRING(General, "diveshareExport/uid", diveshareExport_uid);

HANDLE_PROP_BOOL(General, "diveshareExport/private", diveshareExport_private);

HANDLE_PREFERENCE_BOOL(General, "filterFullTextNotes", filterFullTextNotes);

HANDLE_PREFERENCE_BOOL(General, "filterCaseSensitive", filterCaseSensitive);
