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

	// Get thumbnail from cache. If it is a video, the video flag will of entry be set.
	QImage getThumbnail(PictureEntry &entry, int maxSize);
	void writeHashes(QDataStream &) const;
	void readHashes(QDataStream &);
signals:
	void thumbnailChanged(QString filename, QImage thumbnail, bool isVideo);
private:
	void processItem(QString filename, int size);

	mutable QMutex lock;

	QHash<QString, QImage> thumbnailCache;
	QHash<QString, QImage> videoThumbnailCache;
	QSet<QString> workingOn;
};

// Currently, if we suspect a video, return a null image and true.
// TODO: return an actual still frame from the video.
std::pair<QImage, bool> getHashedImage(const QString &filename);

#endif // IMAGEDOWNLOADER_H
