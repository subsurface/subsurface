// SPDX-License-Identifier: GPL-2.0
#ifndef IMAGEDOWNLOADER_H
#define IMAGEDOWNLOADER_H

#include "metadata.h"
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
	void load(QUrl url, QString filename);
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

	// Schedule multiple thumbnails for forced recalculation
	void calculateThumbnails(const QVector<QString> &filenames);

	// If we change dive, clear all unfinished thumbnail creations
	void clearWorkQueue();
	static int maxThumbnailSize();
	static int defaultThumbnailSize();
	static int thumbnailSize(double zoomLevel);
public slots:
	void imageDownloaded(QString filename);
	void imageDownloadFailed(QString filename);
signals:
	void thumbnailChanged(QString filename, QImage thumbnail, mediatype_t type);
private:
	Thumbnailer();
	void recalculate(QString filename);
	void processItem(QString filename, bool tryDownload);
	std::pair<QImage, mediatype_t> getThumbnailFromCache(const QString &picture_filename);
	std::pair<QImage, mediatype_t> fetchImage(const QString &filename, const QString &originalFilename, bool tryDownload);
	std::pair<QImage, mediatype_t> getHashedImage(const QString &filename, bool tryDownload);

	mutable QMutex lock;
	QThreadPool pool;
	QImage failImage;		// Shown when image-fetching fails
	QImage dummyImage;		// Shown before thumbnail is fetched
	QImage videoImage;		// Place holder for videos

	QMap<QString,QFuture<void>> workingOn;
};

#endif // IMAGEDOWNLOADER_H
