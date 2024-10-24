// SPDX-License-Identifier: GPL-2.0
#include "qPrefMedia.h"
#include "qPrefPrivate.h"

static const QString group = QStringLiteral("Media");

qPrefMedia *qPrefMedia::instance()
{
	static qPrefMedia *self = new qPrefMedia;
	return self;
}

void qPrefMedia::loadSync(bool doSync)
{
	disk_extract_video_thumbnails(doSync);
	disk_extract_video_thumbnails_position(doSync);
	disk_ffmpeg_executable(doSync);
	disk_subtitles_format_string(doSync);
	disk_auto_recalculate_thumbnails(doSync);
	disk_auto_recalculate_thumbnails(doSync);
}

HANDLE_PREFERENCE_BOOL(Media, "auto_recalculate_thumbnails", auto_recalculate_thumbnails);
HANDLE_PREFERENCE_BOOL(Media, "extract_video_thumbnails", extract_video_thumbnails);
HANDLE_PREFERENCE_INT(Media, "extract_video_thumbnails_position", extract_video_thumbnails_position);
HANDLE_PREFERENCE_TXT(Media, "ffmpeg_executable", ffmpeg_executable);
HANDLE_PREFERENCE_TXT(Media, "subtitles_format_string", subtitles_format_string);
