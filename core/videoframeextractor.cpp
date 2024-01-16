// SPDX-License-Identifier: GPL-2.0
#include "videoframeextractor.h"
#include "imagedownloader.h"
#include "core/pref.h"
#include "core/errorhelper.h"

#include <QtConcurrent>
#include <QProcess>

// Note: this is a global instead of a function-local variable on purpose.
// We don't want this to be generated in a different thread context if
// VideoFrameExtractor::instance() is called from a worker thread.
static VideoFrameExtractor frameExtractor;
VideoFrameExtractor *VideoFrameExtractor::instance()
{
	return &frameExtractor;
}

VideoFrameExtractor::VideoFrameExtractor()
{
	// Currently, we only process one video at a time.
	// Eventually, we might want to increase this value.
	pool.setMaxThreadCount(1);
}

void VideoFrameExtractor::extract(QString originalFilename, QString filename, duration_t duration)
{
	QMutexLocker l(&lock);
	if (!workingOn.contains(originalFilename)) {
		// We are not currently extracting this video - add it to the list.
		workingOn.insert(originalFilename, QtConcurrent::run(&pool, [this, originalFilename, filename, duration]()
				 { processItem(originalFilename, filename, duration); }));
	}
}

void VideoFrameExtractor::fail(const QString &originalFilename, duration_t duration, bool isInvalid)
{
	if (isInvalid)
		emit invalid(originalFilename, duration);
	else
		emit failed(originalFilename, duration);
	QMutexLocker l(&lock);
	workingOn.remove(originalFilename);
}

void VideoFrameExtractor::clearWorkQueue()
{
	QMutexLocker l(&lock);
	for (auto it = workingOn.begin(); it != workingOn.end(); ++it)
		it->cancel();
	workingOn.clear();
}

// Trivial helper: bring value into given range
template <typename T>
T clamp(T v, T lo, T hi)
{
	return v < lo ? lo : v > hi ? hi : v;
}

void VideoFrameExtractor::processItem(QString originalFilename, QString filename, duration_t duration)
{
	// If video frame extraction is turned off (e.g. because we failed to start ffmpeg),
	// abort immediately.
	if (!prefs.extract_video_thumbnails) {
		QMutexLocker l(&lock);
		workingOn.remove(originalFilename);
		return;
	}

	// Determine the time where we want to extract the image.
	// If the duration is < 10 sec, just snap the first frame
	duration_t position = { 0 };
	if (duration.seconds > 10) {
		// We round to second-precision. To be sure that we don't attempt reading past the
		// video's end, round down by one second.
		--duration.seconds;
		position.seconds = clamp(duration.seconds * prefs.extract_video_thumbnails_position / 100,
					 0, duration.seconds);
	}
	QString posString = QString("%1:%2:%3").arg(position.seconds / 3600, 2, 10, QChar('0'))
					       .arg((position.seconds % 3600) / 60, 2, 10, QChar('0'))
					       .arg(position.seconds % 60, 2, 10, QChar('0'));

	QProcess ffmpeg;
	ffmpeg.start(prefs.ffmpeg_executable, QStringList {
		"-ss", posString, "-i", filename, "-vframes", "1", "-q:v", "2", "-f", "image2", "-"
	});
	if (!ffmpeg.waitForStarted()) {
		// Since we couldn't sart ffmpeg, turn off thumbnailing
		// TODO: call the proper preferences-functions
		prefs.extract_video_thumbnails = false;
		report_error(qPrintable(tr("ffmpeg failed to start - video thumbnail creation suspended. To enable video thumbnailing, set working executable in preferences.")));
		return fail(originalFilename, duration, false);
	}
	if (!ffmpeg.waitForFinished()) {
		report_error(qPrintable(tr("Failed waiting for ffmpeg - video thumbnail creation suspended. To enable video thumbnailing, set working executable in preferences.")));
		return fail(originalFilename, duration, false);
	}

	QByteArray data = ffmpeg.readAll();
	QImage img;
	img.loadFromData(data);
	if (img.isNull()) {
		qInfo() << "Failed reading ffmpeg output";
		// For debugging:
		//qInfo() << "stdout: " << QString::fromUtf8(data);
		ffmpeg.setReadChannel(QProcess::StandardError);
		// For debugging:
		//QByteArray stderr_output = ffmpeg.readAll();
		//qInfo() << "stderr: " << QString::fromUtf8(stderr_output);
		return fail(originalFilename, duration, true);
	}

	emit extracted(originalFilename, std::move(img), duration, position);
	QMutexLocker l(&lock);
	workingOn.remove(originalFilename);
}
