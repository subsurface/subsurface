// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFGENERAL_H
#define QPREFGENERAL_H
#include "core/pref.h"

#include <QObject>

class qPrefGeneral : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool auto_recalculate_thumbnails READ auto_recalculate_thumbnails WRITE set_auto_recalculate_thumbnails NOTIFY auto_recalculate_thumbnailsChanged);
	Q_PROPERTY(QString default_cylinder READ default_cylinder WRITE set_default_cylinder NOTIFY default_cylinderChanged);
	Q_PROPERTY(QString default_filename READ default_filename WRITE set_default_filename NOTIFY default_filenameChanged);
	Q_PROPERTY(enum def_file_behavior default_file_behavior READ default_file_behavior WRITE set_default_file_behavior NOTIFY default_file_behaviorChanged);
	Q_PROPERTY(int defaultsetpoint READ defaultsetpoint WRITE set_defaultsetpoint NOTIFY defaultsetpointChanged);
	Q_PROPERTY(bool extract_video_thumbnails READ extract_video_thumbnails WRITE set_extract_video_thumbnails NOTIFY extract_video_thumbnailsChanged);
	Q_PROPERTY(int extract_video_thumbnails_position READ extract_video_thumbnails_position WRITE set_extract_video_thumbnails_position NOTIFY extract_video_thumbnails_positionChanged);
	Q_PROPERTY(QString ffmpeg_executable READ ffmpeg_executable WRITE set_ffmpeg_executable  NOTIFY ffmpeg_executableChanged);
	Q_PROPERTY(int o2consumption READ o2consumption WRITE set_o2consumption NOTIFY o2consumptionChanged);
	Q_PROPERTY(int pscr_ratio READ pscr_ratio WRITE set_pscr_ratio NOTIFY pscr_ratioChanged);
	Q_PROPERTY(bool use_default_file READ use_default_file WRITE set_use_default_file NOTIFY use_default_fileChanged);
	Q_PROPERTY(QString diveshareExport_uid READ diveshareExport_uid WRITE set_diveshareExport_uid NOTIFY diveshareExport_uidChanged);
	Q_PROPERTY(bool diveshareExport_private READ diveshareExport_private WRITE set_diveshareExport_private NOTIFY diveshareExport_privateChanged);
	Q_PROPERTY(bool filterFullTextNotes READ filterFullTextNotes WRITE set_filterFullTextNotes NOTIFY filterFullTextNotesChanged)
	Q_PROPERTY(bool filterCaseSensitive READ filterCaseSensitive WRITE set_filterCaseSensitive NOTIFY filterCaseSensitiveChanged)

public:
	qPrefGeneral(QObject *parent = NULL);
	static qPrefGeneral *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { return loadSync(false); }
	static void sync() { return loadSync(true); }

public:
	static bool auto_recalculate_thumbnails() { return prefs.auto_recalculate_thumbnails; }
	static QString default_cylinder() { return prefs.default_cylinder; }
	static QString default_filename() { return prefs.default_filename; }
	static enum def_file_behavior default_file_behavior() { return prefs.default_file_behavior; }
	static int defaultsetpoint() { return prefs.defaultsetpoint; }
	static bool extract_video_thumbnails() { return prefs.extract_video_thumbnails; }
	static int extract_video_thumbnails_position() { return prefs.extract_video_thumbnails_position; }
	static QString ffmpeg_executable() { return prefs.ffmpeg_executable; }
	static int o2consumption() { return prefs.o2consumption; }
	static int pscr_ratio() { return prefs.pscr_ratio; }
	static bool use_default_file() { return prefs.use_default_file; }
	static QString diveshareExport_uid() { return st_diveshareExport_uid; }
	static bool diveshareExport_private() { return st_diveshareExport_private; }
	static bool filterFullTextNotes() { return prefs.filterFullTextNotes; }
	static bool filterCaseSensitive() { return prefs.filterCaseSensitive; }

public slots:
	static void set_auto_recalculate_thumbnails(bool value);
	static void set_default_cylinder(const QString& value);
	static void set_default_filename(const QString& value);
	static void set_default_file_behavior(enum def_file_behavior value);
	static void set_defaultsetpoint(int value);
	static void set_extract_video_thumbnails(bool value);
	static void set_extract_video_thumbnails_position(int value);
	static void set_ffmpeg_executable(const QString& value);
	static void set_o2consumption(int value);
	static void set_pscr_ratio(int value);
	static void set_use_default_file(bool value);
	static void set_diveshareExport_uid(const QString& value);
	static void set_diveshareExport_private(bool value);
	static void set_filterFullTextNotes(bool value);
	static void set_filterCaseSensitive(bool value);

signals:
	void auto_recalculate_thumbnailsChanged(bool value);
	void default_cylinderChanged(const QString& value);
	void default_filenameChanged(const QString& value);
	void default_file_behaviorChanged(enum def_file_behavior value);
	void defaultsetpointChanged(int value);
	void extract_video_thumbnailsChanged(bool value);
	void extract_video_thumbnails_positionChanged(int value);
	void ffmpeg_executableChanged(const QString& value);
	void o2consumptionChanged(int value);
	void pscr_ratioChanged(int value);
	void use_default_fileChanged(bool value);
	void diveshareExport_uidChanged(const QString& value);
	void diveshareExport_privateChanged(bool value);
	void filterFullTextNotesChanged(bool value);
	void filterCaseSensitiveChanged(bool value);

private:
	static void disk_auto_recalculate_thumbnails(bool doSync);
	static void disk_default_cylinder(bool doSync);
	static void disk_default_filename(bool doSync);
	static void disk_default_file_behavior(bool doSync);
	static void disk_defaultsetpoint(bool doSync);
	static void disk_extract_video_thumbnails(bool doSync);
	static void disk_extract_video_thumbnails_position(bool doSync);
	static void disk_ffmpeg_executable(bool doSync);
	static void disk_o2consumption(bool doSync);
	static void disk_pscr_ratio(bool doSync);
	static void disk_use_default_file(bool doSync);
	static void disk_filterFullTextNotes(bool doSync);
	static void disk_filterCaseSensitive(bool doSync);

	// class variables are load only
	static void load_diveshareExport_uid();
	static void load_diveshareExport_private();

	// class variables
	static QString st_diveshareExport_uid;
	static bool st_diveshareExport_private;
};

#endif
