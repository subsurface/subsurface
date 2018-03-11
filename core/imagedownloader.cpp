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

// Note: this is a global instead of a function-local variable on purpose.
// We don't want this to be generated in a different thread context if
// ImageDownloader::instance() is called from a worker thread.
static VideoFrameExtractor frameExtractor;
VideoFrameExtractor *VideoFrameExtractor::instance()
{
	return &frameExtractor;
}

VideoFrameExtractor::VideoFrameExtractor() : player(nullptr),
	processingItem(false),
	doneExtracting(false)
{
	connect(this, &VideoFrameExtractor::extracted, Thumbnailer::instance(), &Thumbnailer::frameExtracted);
	connect(this, &VideoFrameExtractor::failed, Thumbnailer::instance(), &Thumbnailer::imageDownloadFailed);
}

void VideoFrameExtractor::extract(QString originalFilename, QString filename)
{
	qDebug() << QString("VideoFrameExtractor: extract %1 (=%2)").arg(originalFilename, qPrintable(filename));
	workQueue.enqueue({originalFilename, filename});
	processItem();
}

void VideoFrameExtractor::playerStateChanged(QMediaPlayer::State state)
{
	qDebug() << "VideoFrameExtractor: state changed to " << (int)state;
	if (state != QMediaPlayer::StoppedState)
		return;
	qDebug() << "VideoFrameExtractor: state stopped";
	if (!doneExtracting) {
		qInfo() << "VideoFrameExtractor: stopped before getting image!";
		// State changed to stopped without a frame having been processed.
		// Count this as an error.
		emit failed(originalFilename);
		doneExtracting = true;
	}
	processingItem = false;
	processItem();
}

void VideoFrameExtractor::processItem()
{
	qDebug() << "VideoFrameExtractor: process item - queue length " << workQueue.size();
	if (processingItem || workQueue.empty())
		return;

	if (!player) {
		player = new QMediaPlayer();
		connect(player, &QMediaPlayer::stateChanged, this, &VideoFrameExtractor::playerStateChanged);
		player->setVideoOutput(this);
		player->setVolume(0);
	}
	processingItem = true;
	auto item = workQueue.dequeue();
	qDebug() << QString("VideoFrameExtractor: processing item %1 (=%2)").arg(item.first, item.second);

	// Create a new extractor. It will delete itself on either failure or success.
	QString filename = item.second;
	originalFilename = item.first;
	player->setMedia(QUrl::fromLocalFile(filename));
	doneExtracting = false;
	player->play();
}

bool VideoFrameExtractor::present(const QVideoFrame &frame_in)
{
	qDebug() << "VideoFrameExtractor: present frame " << workQueue.size();
	// We're getting a additional present() calls after the first (not yet sure why). Let's just nop if we do.
	if (doneExtracting) {
		qDebug() << "VideoFrameExtractor: present frame after stopping" << workQueue.size();
		return false;
	}
	doneExtracting = true;

	QVideoFrame frame(frame_in);	// Copy so that we can map
	frame.map(QAbstractVideoBuffer::ReadOnly);
	QImage::Format format = QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
	bool ret = format != QImage::Format_Invalid;
	if (ret) {
		QImage img(frame.bits(), frame.width(), frame.height(), format);
		qDebug() << "VideoFrameExtractor: extracted " << originalFilename;
		emit extracted(originalFilename, img);
	} else {
		qInfo() << "VideoFrameExtractor: failed frame extraction from " << originalFilename;
		emit failed(originalFilename);
	}

	qDebug() << "VideoFrameExtractor: stopping player";
	player->stop();
	player->setMedia(QMediaContent());
	return ret;
}

QList<QVideoFrame::PixelFormat> VideoFrameExtractor::supportedPixelFormats(QAbstractVideoBuffer::HandleType) const
{
	return {
		QVideoFrame::Format_ARGB32,
		QVideoFrame::Format_RGB32,
		QVideoFrame::Format_RGB24,
		QVideoFrame::Format_Jpeg
	};
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

static std::pair<QImage,bool> loadImage(const QString &originalFileName, const QString &fileName)
{
	std::pair<QImage,bool> res { {}, false };
	QImageReader reader(fileName);
	res.first = reader.read();
	if (res.first.isNull()) {
		res.second = isVideoFile(fileName);
		if (!res.second)
			qInfo() << "Error loading image" << fileName << (int)reader.error() << reader.errorString();
	}
	if (res.second) // Got a video - process frames in background
		QMetaObject::invokeMethod(VideoFrameExtractor::instance(), "extract", Qt::AutoConnection, Q_ARG(QString, originalFileName), Q_ARG(QString, fileName));
	return res;
}

// Returns: thumbnail, still loading
static std::pair<QImage,bool> getHashedImage(const QString &file)
{
	std::pair<QImage,bool> thumb { {}, false };
	QUrl url = QUrl::fromUserInput(localFilePath(file));
	if (url.isLocalFile())
		thumb = loadImage(file, url.toLocalFile());
	if (thumb.second)
		return thumb;	// Got a video - wait for background processing to finish.
	if (thumb.first.isNull()) {
		// This did not load anything. Let's try to get the image from other sources
		// Let's try to load it locally via its hash
		QString filenameLocal = localFilePath(qPrintable(file));
		qDebug() << QStringLiteral("Translated filename: %1 -> %2").arg(file, filenameLocal);
		if (filenameLocal.isNull()) {
			// That didn't produce a local filename.
			// Try the cloud server
			// TODO: This is dead code at the moment.
			loadPicture(file, true);
			thumb.second = true;
		} else {
			// Load locally from translated file name
			thumb = loadImage(file, filenameLocal);
			if (thumb.second)
				return thumb;	// Got a video - wait for background processing to finish.
			if (!thumb.first.isNull()) {
				// Make sure the hash still matches the image file
				hashPicture(filenameLocal);
			} else {
				// Interpret filename as URL
				loadPicture(filenameLocal, false);
				thumb.second = true;
			}
		}
	} else {
		// We loaded successfully. Now, make sure hash is up to date.
		hashPicture(file);
	}
	return thumb;
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
	// getHashedImage() only returns an image for image files.
	// Videos will be processed in the background.
	auto res = getHashedImage(filename);
	if (res.second)
		return;
	QImage thumbnail = res.first;

	if (thumbnail.isNull()) {
		imageDownloadFailed(filename);
		return;
	} else {
		int size = maxThumbnailSize();
		thumbnail = thumbnail.scaled(size, size, Qt::KeepAspectRatio);
		QMutexLocker l(&lock);
		thumbnailCache.insert(filename, thumbnail);
		workingOn.remove(filename);
	}
	emit thumbnailChanged(filename, thumbnail, false);
}

void Thumbnailer::frameExtracted(QString filename, QImage thumbnail)
{
	if (thumbnail.isNull()) {
		imageDownloadFailed(filename);
		return;
	} else {
		int size = maxThumbnailSize();
		thumbnail = thumbnail.scaled(size, size, Qt::KeepAspectRatio);
		QMutexLocker l(&lock);
		videoThumbnailCache.insert(filename, thumbnail);
		workingOn.remove(filename);
	}
	emit thumbnailChanged(filename, thumbnail, true);
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

	QImage res;
	QString filename = entry.filename;
	entry.isVideo = videoThumbnailCache.contains(filename);
	if (entry.isVideo)
		res = videoThumbnailCache.value(filename);
	else if (thumbnailCache.contains(filename))
		res = thumbnailCache.value(filename);

	// If thumbnails were written by an earlier version, they might be smaller than needed.
	// Rescale in such a case to avoid resizing artifacts.
	if (!res.isNull() && (res.size().width() >= size || res.size().height() >= size))
		return res;

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
