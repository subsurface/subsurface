// SPDX-License-Identifier: GPL-2.0
#include "dive.h"
#include "metrics.h"
#include "divelist.h"
#include "qthelper.h"
#include "imagedownloader.h"
#include "qt-models/divepicturemodel.h"
#include <unistd.h>
#include <QString>
#include <QImageReader>
#include <QDataStream>

#include <QtConcurrent>

static QUrl cloudImageURL(const char *filename)
{
	QString hash = hashString(filename);
	return QUrl::fromUserInput(QString("https://cloud.subsurface-divelog.org/images/").append(hash));
}

// Note: this is a global instead of a function-local variable on purpose.
// We don't want this to be generated in a different thread context if
// ImageDownloader::instance() is called from a worker thread.
static ImageDownloader imageDownloader;
ImageDownloader *ImageDownloader::instance()
{
	return &imageDownloader;
}

ImageDownloader::ImageDownloader()
{
	connect(&manager, &QNetworkAccessManager::finished, this, &ImageDownloader::saveImage);
}

void ImageDownloader::load(QString filename, bool fromHash)
{
	QUrl url = fromHash ? cloudImageURL(qPrintable(filename)) : QUrl::fromUserInput(filename);

	if (!url.isValid())
		emit failed(filename);

	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::User, filename);
	request.setAttribute(static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 1), fromHash);
	manager.get(request);
}

void ImageDownloader::saveImage(QNetworkReply *reply)
{
	QString filename = reply->request().attribute(QNetworkRequest::User).toString();

	if (reply->error() != QNetworkReply::NoError) {
		bool fromHash = reply->request().attribute(static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 1)).toBool();
		if (fromHash)
			load(filename, false);
		else
			emit failed(filename);
	} else {
		QByteArray imageData = reply->readAll();
		QCryptographicHash hash(QCryptographicHash::Sha1);
		hash.addData(imageData);
		QString path = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first();
		QDir dir(path);
		if (!dir.exists())
			dir.mkpath(path);
		QFile imageFile(path.append("/").append(hash.result().toHex()));
		if (imageFile.open(QIODevice::WriteOnly)) {
			qDebug() << "Write image to" << imageFile.fileName();
			QDataStream stream(&imageFile);
			stream.writeRawData(imageData.data(), imageData.length());
			imageFile.waitForBytesWritten(-1);
			imageFile.close();
			learnHash(filename, imageFile.fileName(), hash.result());
		}
		emit loaded(filename);
	}

	reply->deleteLater();
}

static void loadPicture(QString filename, bool fromHash)
{
	// This has to be done in UI main thread, because QNetworkManager refuses
	// to treat requests from other threads.
	QMetaObject::invokeMethod(ImageDownloader::instance(), "load", Qt::AutoConnection, Q_ARG(QString, filename), Q_ARG(bool, fromHash));
}

static bool isVideoFile(const QString &filename)
{
	// At the moment, we're very crude:
	// If a file exists and ends in a supported filename, we consider it a video.
	if (std::none_of(videoExtensionsList.begin(), videoExtensionsList.end(),
			 [&filename](const QString &extension)
			 { return filename.endsWith(extension, Qt::CaseInsensitive); }))
		return false;
	QFileInfo fi(filename);
	return fi.exists() && fi.isFile();
}

static std::pair<QImage,bool> loadImage(const QString &fileName, const char *format = nullptr)
{
	std::pair<QImage,bool> res { {}, false };
	QImageReader reader(fileName, format);
	res.first = reader.read();
	if (res.first.isNull()) {
		res.second = isVideoFile(fileName);
		if (!res.second)
			qInfo() << "Error loading image" << fileName << (int)reader.error() << reader.errorString();
	}
	return res;
}

// Returns: thumbnail, isVideo, still loading
// Currently, if we suspect a video, return a null image and true.
// TODO: return an actual still frame from the video.
static std::tuple<QImage,bool,bool> getHashedImage(const QString &file)
{
	std::pair<QImage,bool> thumb { {}, false };
	bool stillLoading = false;
	QUrl url = QUrl::fromUserInput(localFilePath(file));
	if (url.isLocalFile())
		thumb = loadImage(url.toLocalFile());
	if (thumb.first.isNull() && !thumb.second) {
		// This did not load anything. Let's try to get the image from other sources
		// Let's try to load it locally via its hash
		QString filenameLocal = localFilePath(qPrintable(file));
		qDebug() << QStringLiteral("Translated filename: %1 -> %2").arg(file, filenameLocal);
		if (filenameLocal.isNull()) {
			// That didn't produce a local filename.
			// Try the cloud server
			// TODO: This is dead code at the moment.
			loadPicture(file, true);
			stillLoading = true;
		} else {
			// Load locally from translated file name
			thumb = loadImage(filenameLocal);
			if (!thumb.first.isNull() || thumb.second) {
				// Make sure the hash still matches the image file
				hashPicture(filenameLocal);
			} else {
				// Interpret filename as URL
				loadPicture(filenameLocal, false);
				stillLoading = true;
			}
		}
	} else {
		// We loaded successfully. Now, make sure hash is up to date.
		hashPicture(file);
	}
	return { thumb.first, thumb.second, stillLoading };
}

Thumbnailer *Thumbnailer::instance()
{
	static Thumbnailer self;
	return &self;
}

Thumbnailer::Thumbnailer()
{
	connect(ImageDownloader::instance(), &ImageDownloader::loaded, this, &Thumbnailer::imageDownloaded);
	connect(ImageDownloader::instance(), &ImageDownloader::failed, this, &Thumbnailer::imageDownloadFailed);
}

void Thumbnailer::processItem(QString filename)
{
	auto res = getHashedImage(filename);
	if (std::get<2>(res))
		return;
	QImage thumbnail = std::get<0>(res);
	bool isVideo = std::get<1>(res);

	if (thumbnail.isNull() && !isVideo) {
		imageDownloadFailed(filename);
		return;
	} else {
		int size = maxThumbnailSize();
		thumbnail = isVideo ?
			QImage(":video-icon").scaled(size, size, Qt::KeepAspectRatio) :
			thumbnail.scaled(size, size, Qt::KeepAspectRatio);
		QMutexLocker l(&lock);
		if (isVideo)
			videoThumbnailCache.insert(filename, QImage());
		else
			thumbnailCache.insert(filename, thumbnail);
		workingOn.remove(filename);
	}
	emit thumbnailChanged(filename, thumbnail, isVideo);
}

void Thumbnailer::imageDownloaded(QString filename)
{
	// Image was downloaded and the filename connected with a hash.
	// Try thumbnailing again.
	QtConcurrent::run([this, filename]() { processItem(filename); });
}

void Thumbnailer::imageDownloadFailed(QString filename)
{
	int size = maxThumbnailSize();
	// TODO: Don't misuse filter close icon
	QImage thumbnail = QImage(":filter-close").scaled(size, size, Qt::KeepAspectRatio);
	emit thumbnailChanged(filename, thumbnail, false);
	QMutexLocker l(&lock);
	workingOn.remove(filename);
}

void Thumbnailer::writeHashes(QDataStream &stream) const
{
	QMutexLocker l(&lock);
	stream << thumbnailCache;
	stream << videoThumbnailCache;
}

void Thumbnailer::readHashes(QDataStream &stream)
{
	QMutexLocker l(&lock);
	stream >> thumbnailCache;
	stream >> videoThumbnailCache;
}

QImage Thumbnailer::getThumbnail(PictureEntry &entry)
{
	int size = maxThumbnailSize();
	QMutexLocker l(&lock);

	// Currently we only save a null picture in the videoThumbnailCache
	// as a marker that this was identified as a video. In the future,
	// use the videoThumbnail cache to save an actual still image.
	entry.isVideo = videoThumbnailCache.contains(entry.filename);
	if (entry.isVideo)
		return QImage(":video-icon").scaled(size, size, Qt::KeepAspectRatio);

	QString filename = entry.filename;
	if (thumbnailCache.contains(filename)) {
		// If thumbnails were written by an earlier version, they might be smaller than needed.
		// Rescale in such a case to avoid resizing artifacts.
		QImage res = thumbnailCache.value(filename);
		if (!res.isNull() && (res.size().width() >= size || res.size().height() >= size))
			return res;
	}

	// We didn't find an entry for this picture - schedule thumbnail calculation and return dummy icon
	if (!workingOn.contains(filename)) {
		workingOn.insert(filename);
		QtConcurrent::run([this, filename]() { processItem(filename); });
	}
	return QImage(":photo-icon").scaled(size, size, Qt::KeepAspectRatio);
}

static const int maxZoom = 3;	// Maximum zoom: thrice of standard size

int Thumbnailer::defaultThumbnailSize()
{
	return defaultIconMetrics().sz_pic;
}

int Thumbnailer::maxThumbnailSize()
{
	return defaultThumbnailSize() * maxZoom;
}

int Thumbnailer::thumbnailSize(double zoomLevel)
{
	// Calculate size of thumbnails. The standard size is defaultIconMetrics().sz_pic.
	// We use exponential scaling so that the central point is the standard
	// size and the minimum and maximum extreme points are a third respectively
	// three times the standard size.
	// Naturally, these three zoom levels are then represented by
	// -1.0 (minimum), 0 (standard) and 1.0 (maximum). The actual size is
	// calculated as standard_size*3.0^zoomLevel.
	return static_cast<int>(round(defaultThumbnailSize() * pow(maxZoom, zoomLevel)));
}
