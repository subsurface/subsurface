// SPDX-License-Identifier: GPL-2.0
#ifndef IMAGEDOWNLOADER_H
#define IMAGEDOWNLOADER_H

#include <QImage>
#include <QFuture>
#include <QNetworkReply>
#include <QThread>

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

class PictureEntry;
class Thumbnailer : public QObject {
	Q_OBJECT
public:
	static Thumbnailer *instance();

	// Get thumbnail from cache. If it is a video, the video flag will of entry be set.
	QImage getThumbnail(PictureEntry &entry);
	void writeHashes(QDataStream &) const;
	void readHashes(QDataStream &);
	static int maxThumbnailSize();
	static int defaultThumbnailSize();
	static int thumbnailSize(double zoomLevel);
public slots:
	void imageDownloaded(QString filename);
	void imageDownloadFailed(QString filename);
signals:
	void thumbnailChanged(QString filename, QImage thumbnail, bool isVideo);
private:
	Thumbnailer();
	void processItem(QString filename);

	mutable QMutex lock;

	QHash<QString, QImage> thumbnailCache;
	QHash<QString, QImage> videoThumbnailCache;
	QSet<QString> workingOn;
};

#endif // IMAGEDOWNLOADER_H
