// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFMEDIA_H
#define QPREFMEDIA_H
#include "core/pref.h"

#include <QObject>

class qPrefMedia : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool auto_recalculate_thumbnails READ auto_recalculate_thumbnails WRITE set_auto_recalculate_thumbnails NOTIFY auto_recalculate_thumbnailsChanged)
	Q_PROPERTY(bool extract_video_thumbnails READ extract_video_thumbnails WRITE set_extract_video_thumbnails NOTIFY extract_video_thumbnailsChanged)
	Q_PROPERTY(int extract_video_thumbnails_position READ extract_video_thumbnails_position WRITE set_extract_video_thumbnails_position NOTIFY extract_video_thumbnails_positionChanged)
	Q_PROPERTY(QString ffmpeg_executable READ ffmpeg_executable WRITE set_ffmpeg_executable  NOTIFY ffmpeg_executableChanged)

public:
	static qPrefMedia *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	static bool auto_recalculate_thumbnails() { return prefs.auto_recalculate_thumbnails; }
	static bool extract_video_thumbnails() { return prefs.extract_video_thumbnails; }
	static int extract_video_thumbnails_position() { return prefs.extract_video_thumbnails_position; }
	static QString ffmpeg_executable() { return QString::fromStdString(prefs.ffmpeg_executable); }

public slots:
	static void set_auto_recalculate_thumbnails(bool value);
	static void set_extract_video_thumbnails(bool value);
	static void set_extract_video_thumbnails_position(int value);
	static void set_ffmpeg_executable(const QString& value);

signals:
	void auto_recalculate_thumbnailsChanged(bool value);
	void extract_video_thumbnailsChanged(bool value);
	void extract_video_thumbnails_positionChanged(int value);
	void ffmpeg_executableChanged(const QString& value);

private:
	qPrefMedia() {}

	static void disk_auto_recalculate_thumbnails(bool doSync);
	static void disk_extract_video_thumbnails(bool doSync);
	static void disk_extract_video_thumbnails_position(bool doSync);
	static void disk_ffmpeg_executable(bool doSync);

};

#endif
