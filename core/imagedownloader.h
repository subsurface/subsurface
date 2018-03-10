// SPDX-License-Identifier: GPL-2.0
#ifndef IMAGEDOWNLOADER_H
#define IMAGEDOWNLOADER_H

#include <QImage>
#include <QFuture>
#include <QNetworkReply>
#include <QThreadPool>

class ImageDownloader : public QObject {
	Q_OBJECT
public:
	ImageDownloader(const QString &filename);
	void load(bool fromHash);

private:
	bool loadFromUrl(const QUrl &);	// return true on success
	void saveImage(QNetworkReply *reply, bool &success);
	QString filename;
};

class PictureEntry;
class Thumbnailer : public QObject {
	Q_OBJECT
public:
	static Thumbnailer *instance();

	// Schedule a thumbnail for fetching or calculation.
	// Returns a placehlder thumbnail. The actual thumbnail will be sent
	// via a signal later.
	QImage fetchThumbnail(PictureEntry &entry, int size);

	// If we change dive, clear all unfinished thumbnail creations
	void clearWorkQueue();
signals:
	void thumbnailChanged(QString filename, QImage thumbnail);
private:
	Thumbnailer();
	void processItem(QString filename, int size);

	mutable QMutex lock;
	QThreadPool pool;

	QMap<QString,QFuture<void>> workingOn;
};

QImage getHashedImage(const QString &filename);

#endif // IMAGEDOWNLOADER_H
