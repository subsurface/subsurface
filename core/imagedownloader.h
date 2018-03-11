// SPDX-License-Identifier: GPL-2.0
#ifndef IMAGEDOWNLOADER_H
#define IMAGEDOWNLOADER_H

#include <QImage>
#include <QFuture>
#include <QNetworkReply>
#include <QThread>
#include <QMediaPlayer>
#include <QAbstractVideoSurface>
#include <QQueue>

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

class VideoFrameExtractor : public QAbstractVideoSurface {
	Q_OBJECT
public:
	VideoFrameExtractor();
	static VideoFrameExtractor *instance();
signals:
	void extracted(QString filename, QImage);
	void failed(QString filename);
public slots:
	void extract(QString originalFilename, QString filename);
	void processItem();
	void playerStateChanged(QMediaPlayer::State state);
private:
	QMediaPlayer *player;
	QString originalFilename;
	bool processingItem;
	bool doneExtracting;
	QQueue<QPair<QString, QString>> workQueue;
	void doit();
	QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType) const;
	bool present(const QVideoFrame &frame);
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
	void frameExtracted(QString filename, QImage thumbnail);
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
