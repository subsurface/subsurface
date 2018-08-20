// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFGENERAL_H
#define QPREFGENERAL_H
#include "core/pref.h"

#include <QObject>

class qPrefGeneral : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool auto_recalculate_thumbnails READ auto_recalculate_thumbnails WRITE set_auto_recalculate_thumbnails NOTIFY auto_recalculate_thumbnails_changed);
	Q_PROPERTY(QString default_cylinder READ default_cylinder WRITE set_default_cylinder NOTIFY default_cylinder_changed);
	Q_PROPERTY(QString default_filename READ default_filename WRITE set_default_filename NOTIFY default_filename_changed);
	Q_PROPERTY(enum def_file_behavior default_file_behavior READ default_file_behavior WRITE set_default_file_behavior NOTIFY default_file_behavior_changed);
	Q_PROPERTY(int defaultsetpoint READ defaultsetpoint WRITE set_defaultsetpoint NOTIFY defaultsetpoint_changed);
	Q_PROPERTY(bool extract_video_thumbnails READ extract_video_thumbnails WRITE set_extract_video_thumbnails NOTIFY extract_video_thumbnails_changed);
	Q_PROPERTY(int extract_video_thumbnails_position READ extract_video_thumbnails_position WRITE set_extract_video_thumbnails_position NOTIFY extract_video_thumbnails_position_changed);
	Q_PROPERTY(QString ffmpeg_executable READ ffmpeg_executable WRITE set_ffmpeg_executable  NOTIFY ffmpeg_executable_changed);
	Q_PROPERTY(int o2consumption READ o2consumption WRITE set_o2consumption NOTIFY o2consumption_changed);
	Q_PROPERTY(int pscr_ratio READ pscr_ratio WRITE set_pscr_ratio NOTIFY pscr_ratio_changed);
	Q_PROPERTY(bool use_default_file READ use_default_file WRITE set_use_default_file NOTIFY use_default_file_changed);
	Q_PROPERTY(QString diveshareExport_uid READ diveshareExport_uid WRITE set_diveshareExport_uid NOTIFY diveshareExport_uid_changed);
	Q_PROPERTY(bool diveshareExport_private READ diveshareExport_private WRITE set_diveshareExport_private NOTIFY diveshareExport_private_changed);

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

signals:
	void auto_recalculate_thumbnails_changed(bool value);
	void default_cylinder_changed(const QString& value);
	void default_filename_changed(const QString& value);
	void default_file_behavior_changed(enum def_file_behavior value);
	void defaultsetpoint_changed(int value);
	void extract_video_thumbnails_changed(bool value);
	void extract_video_thumbnails_position_changed(int value);
	void ffmpeg_executable_changed(const QString& value);
	void o2consumption_changed(int value);
	void pscr_ratio_changed(int value);
	void use_default_file_changed(bool value);
	void diveshareExport_uid_changed(const QString& value);
	void diveshareExport_private_changed(bool value);

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

	// class variables are load only
	static void load_diveshareExport_uid();
	static void load_diveshareExport_private();

	// class variables
	static QString st_diveshareExport_uid;
	static bool st_diveshareExport_private;
};

#endif
