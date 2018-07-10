// SPDX-License-Identifier: GPL-2.0
#ifndef VIDEOFRAMEEXTRACTOR_H
#define VIDEOFRAMEEXTRACTOR_H

#include "core/units.h"

#include <QMutex>
#include <QFuture>
#include <QThreadPool>
#include <QQueue>
#include <QString>
#include <QPair>

class VideoFrameExtractor : public QObject {
	Q_OBJECT
public:
	VideoFrameExtractor();
	static VideoFrameExtractor *instance();
signals:
	void extracted(QString filename, QImage, duration_t duration, duration_t offset);
	// There are two failure modes:
	//	failed() -> we failed to start ffmpeg. Write a thumbnail signalling "maybe try again".
	//	invalid() -> we started ffmpeg, but that couldn't extract an image. Signal "this file is broken".
	void failed(QString filename, duration_t duration);
	void invalid(QString filename, duration_t duration);
public slots:
	void extract(QString originalFilename, QString filename, duration_t duration);
	void clearWorkQueue();
private:
	void processItem(QString originalFilename, QString filename, duration_t duration);
	void fail(const QString &originalFilename, duration_t duration, bool isInvalid);
	mutable QMutex lock;
	QThreadPool pool;
	QMap<QString, QFuture<void>> workingOn;
};

#endif
