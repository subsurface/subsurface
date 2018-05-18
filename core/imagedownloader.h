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
	static ImageDownloader *instance();
	ImageDownloader();
public slots:
	void load(QString filename, bool fromHash);
signals:
	void loaded(QString filename);
	void failed(QString filename);
private:
	QNetworkAccessManager manager;
	void loadFromUrl(const QString &filename, const QUrl &);
	void saveImage(QNetworkReply *reply);
};

struct PictureEntry;
class Thumbnailer : public QObject {
	Q_OBJECT
public:
	static Thumbnailer *instance();

	// Schedule a thumbnail for fetching or calculation.
	// Returns a placehlder thumbnail. The actual thumbnail will be sent
	// via a signal later.
	QImage fetchThumbnail(PictureEntry &entry);

	// If we change dive, clear all unfinished thumbnail creations
	void clearWorkQueue();
	static int maxThumbnailSize();
	static int defaultThumbnailSize();
	static int thumbnailSize(double zoomLevel);
public slots:
	void imageDownloaded(QString filename);
	void imageDownloadFailed(QString filename);
signals:
	void thumbnailChanged(QString filename, QImage thumbnail);
private:
	Thumbnailer();
	void processItem(QString filename, bool tryDownload);

	mutable QMutex lock;
	QThreadPool pool;
	QImage failImage;		// Shown when image-fetching fails
	QImage dummyImage;		// Shown before thumbnail is fetched

	QMap<QString,QFuture<void>> workingOn;
};

#endif // IMAGEDOWNLOADER_H
