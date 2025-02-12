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
	// If synchronous is false, returns a placeholder thumbnail.
	// The actual thumbnail will be sent via a signal later.
	// If synchronous is true, try to fetch the actual thumbnail.
	// In this mode only precalculated thumbnails or thumbnails
	// from pictures are returned. Video extraction and remote
	// images are not supported.
	QImage fetchThumbnail(const QString &filename, bool synchronous);

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
	void frameExtracted(QString filename, QImage thumbnail, duration_t duration, duration_t offset);
	void frameExtractionFailed(QString filename, duration_t duration);
	void frameExtractionInvalid(QString filename, duration_t duration);
signals:
	void thumbnailChanged(QString filename, QImage thumbnail, duration_t duration);
private:
	struct Thumbnail {
		QImage img;
		mediatype_t type;
		duration_t duration;
	};

	Thumbnailer();
	Thumbnail fetchVideoThumbnail(const QString &filename, const QString &originalFilename, duration_t duration, bool unknownFiletype = false);
	Thumbnail extractVideoThumbnail(const QString &picture_filename, duration_t duration);
	Thumbnail addPictureThumbnailToCache(const QString &picture_filename, const QImage &thumbnail);
	Thumbnail addVideoThumbnailToCache(const QString &picture_filename, duration_t duration, const QImage &thumbnail, duration_t position);
	Thumbnail addUnknownThumbnailToCache(const QString &picture_filename);
	void recalculate(QString filename);
	void processItem(QString filename, bool tryDownload);
	Thumbnail getThumbnailFromCache(const QString &picture_filename);
	Thumbnail getPictureThumbnailFromStream(QDataStream &stream);
	Thumbnail getVideoThumbnailFromStream(QDataStream &stream, const QString &filename);
	Thumbnail fetchImage(const QString &filename, const QString &originalFilename, bool tryDownload);
	Thumbnail getHashedImage(const QString &filename, bool tryDownload);
	void markVideoThumbnail(QImage &img);

	mutable QMutex lock;
	QThreadPool pool;
	QImage failImage;		// Shown when image-fetching fails
	QImage dummyImage;		// Shown before thumbnail is fetched
	QImage videoImage;		// Place holder for videos
	QImage videoOverlayImage;	// Overlay for video thumbnails
	QImage unknownImage;		// Place holder for files where we couldn't determine the type

	QMap<QString,QFuture<void>> workingOn;
};

#endif // IMAGEDOWNLOADER_H
