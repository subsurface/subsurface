// SPDX-License-Identifier: GPL-2.0
#include "dive.h"
#include "metrics.h"
#include "divelist.h"
#include "qthelper.h"
#include "imagedownloader.h"
#include "qt-models/divepicturemodel.h"
#include "metadata.h"
#include <unistd.h>
#include <QString>
#include <QImageReader>
#include <QDataStream>
#include <QSvgRenderer>
#include <QPainter>

#include <QtConcurrent>

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

void ImageDownloader::load(QUrl url, QString filename)
{
	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::User, filename);
	manager.get(request);
}

void ImageDownloader::saveImage(QNetworkReply *reply)
{
	QString filename = reply->request().attribute(QNetworkRequest::User).toString();

	if (reply->error() != QNetworkReply::NoError) {
		emit failed(filename);
	} else {
		QByteArray imageData = reply->readAll();
		if (imageData.isEmpty()) {
			emit failed(filename);
		} else {
			QString path = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first();
			QDir dir(path);
			if (!dir.exists())
				dir.mkpath(path);
			QCryptographicHash hash(QCryptographicHash::Sha1);
			hash.addData(filename.toUtf8());
			QFile imageFile(path.append("/").append(hash.result().toHex()));
			if (imageFile.open(QIODevice::WriteOnly)) {
				qDebug() << "Write image to" << imageFile.fileName();
				QDataStream stream(&imageFile);
				stream.writeRawData(imageData.data(), imageData.length());
				imageFile.waitForBytesWritten(-1);
				imageFile.close();
				learnPictureFilename(filename, imageFile.fileName());
			}
			emit loaded(filename);
		}
	}

	reply->deleteLater();
}

static bool hasVideoFileExtension(const QString &filename)
{
	for (const QString &ext: videoExtensionsList)
		if (filename.endsWith(ext, Qt::CaseInsensitive))
			return true;
	return false;
}

// Fetch a picture from the given filename and determine its type (picture of video).
// If this is a non-remote file, fetch it from disk. Remote files are fetched from the
// net in a background thread. In such a case, the output-type is set to MEDIATYPE_STILL_LOADING.
// If the input-flag "tryDownload" is set to false, no download attempt is made. This is to
// prevent infinite loops, where failed image downloads would be repeated ad infinitum.
// Returns: fetched image, type
Thumbnailer::Thumbnail Thumbnailer::fetchImage(const QString &filename, const QString &originalFilename, bool tryDownload)
{
	QUrl url = QUrl::fromUserInput(filename);
	if (url.isLocalFile()) {
		// We try to determine the type first by peeking into the file.
		QString filename = url.toLocalFile();
		metadata md;
		mediatype_t type = get_metadata(qPrintable(filename), &md);

		// For io error or video, return early with the appropriate dummy-icon.
		if (type == MEDIATYPE_IO_ERROR)
			return { failImage, MEDIATYPE_IO_ERROR };
		else if (type == MEDIATYPE_VIDEO)
			return { videoImage, MEDIATYPE_VIDEO };

		// Try if Qt can parse this image. If it does, use this as a thumbnail.
		QImage thumb(filename);
		if (!thumb.isNull())
			return { thumb, MEDIATYPE_PICTURE };

		// Neither our code, nor Qt could determine the type of this object from looking at the data.
		// Try to check for a video-file extension.
		if (hasVideoFileExtension(filename))
			return { videoImage, MEDIATYPE_VIDEO };

		// Give up: we simply couldn't determine what this thing is.
		return { unknownImage, MEDIATYPE_UNKNOWN };
	} else if (tryDownload) {
		// This has to be done in UI main thread, because QNetworkManager refuses
		// to treat requests from other threads. invokeMethod() is Qt's way of calling a
		// function in a different thread, namely the thread the called object is associated to.
		QMetaObject::invokeMethod(ImageDownloader::instance(), "load", Qt::AutoConnection, Q_ARG(QUrl, url), Q_ARG(QString, originalFilename));
		return { QImage(), MEDIATYPE_STILL_LOADING };
	}
	return { QImage(), MEDIATYPE_IO_ERROR };
}

// Fetch a picture based on its original filename. If there is a translated filename (obtained either
// by the find-moved-picture functionality or the filename of the local cache of a remote picture),
// try that first. If fetching from the translated filename fails, this could mean that the image
// was downloaded previously, but for some reason the cached picture was lost. Therefore, in such a
// case, try the canonical filename. If that likewise fails, give up. For input and output parameters
// see fetchImage() above.
Thumbnailer::Thumbnail Thumbnailer::getHashedImage(const QString &filename, bool tryDownload)
{
	QString localFilename = localFilePath(filename);

	// If there is a translated filename, try that first.
	// Note that we set the default type to io-error, so that if we didn't try
	// the local filename first, we will load the file from the canonical filename.
	Thumbnail thumbnail { QImage(), MEDIATYPE_IO_ERROR };
	if (localFilename != filename)
		thumbnail = fetchImage(localFilename, filename, tryDownload);

	// If fetching from the local filename failed (or we didn't even try),
	// use the canonical filename. This might for example happen if we downloaded
	// a file, but for some reason lost the cached file.
	if (thumbnail.type == MEDIATYPE_IO_ERROR)
		thumbnail = fetchImage(filename, filename, tryDownload);

	if (thumbnail.type == MEDIATYPE_IO_ERROR)
		qInfo() << "Error loading image" << filename << "[local:" << localFilename << "]";
	return thumbnail;
}

static QImage renderIcon(const char *id, int size)
{
	QImage res(size, size, QImage::Format_RGB32);
	res.fill(Qt::white);
	QSvgRenderer svg{QString(id)};
	QPainter painter(&res);
	svg.render(&painter);
	return res;
}

Thumbnailer::Thumbnailer() : failImage(renderIcon(":filter-close", maxThumbnailSize())), // TODO: Don't misuse filter close icon
			     dummyImage(renderIcon(":camera-icon", maxThumbnailSize())),
			     videoImage(renderIcon(":video-icon", maxThumbnailSize())),
			     unknownImage(renderIcon(":unknown-icon", maxThumbnailSize()))
{
	// Currently, we only process one image at a time. Stefan Fuchs reported problems when
	// calculating multiple thumbnails at once and this hopefully helps.
	pool.setMaxThreadCount(1);
	connect(ImageDownloader::instance(), &ImageDownloader::loaded, this, &Thumbnailer::imageDownloaded);
	connect(ImageDownloader::instance(), &ImageDownloader::failed, this, &Thumbnailer::imageDownloadFailed);
}

Thumbnailer *Thumbnailer::instance()
{
	static Thumbnailer self;
	return &self;
}

Thumbnailer::Thumbnail Thumbnailer::getThumbnailFromCache(const QString &picture_filename)
{
	QString filename = thumbnailFileName(picture_filename);
	if (filename.isEmpty())
		return { QImage(), MEDIATYPE_UNKNOWN };
	QFile file(filename);

	if (prefs.auto_recalculate_thumbnails) {
		// Check if thumbnails is older than the (local) image file
		QString filenameLocal = localFilePath(qPrintable(picture_filename));
		QFileInfo pictureInfo(filenameLocal);
		QFileInfo thumbnailInfo(file);
		if (pictureInfo.exists() && thumbnailInfo.exists()) {
			QDateTime pictureTime = pictureInfo.lastModified();
			QDateTime thumbnailTime = thumbnailInfo.lastModified();
			if (pictureTime.isValid() && thumbnailTime.isValid() && thumbnailTime < pictureTime) {
				// Both files exist, have valid timestamps and thumbnail was calculated before picture.
				// Return an empty thumbnail to signal recalculation of the thumbnail
				return { QImage(), MEDIATYPE_UNKNOWN };
			}
		}
	}

	if (!file.open(QIODevice::ReadOnly))
		return { QImage(), MEDIATYPE_UNKNOWN };
	QDataStream stream(&file);

	// Each thumbnail file is composed of a media-type and an image file.
	quint32 type;
	QImage res;
	stream >> type;
	stream >> res;

	// Thumbnails of videos currently not supported - replace by dummy
	// TODO: Perhaps extract thumbnails
	if (type == MEDIATYPE_VIDEO)
		res = videoImage;
	else if (type == MEDIATYPE_UNKNOWN)
		res = unknownImage;

	return { res, (mediatype_t)type };
}

void Thumbnailer::addThumbnailToCache(const Thumbnail &thumbnail, const QString &picture_filename)
{
	if (thumbnail.img.isNull())
		return;

	QString filename = thumbnailFileName(picture_filename);
	QSaveFile file(filename);
	if (!file.open(QIODevice::WriteOnly))
		return;
	QDataStream stream(&file);

	stream << (quint32)thumbnail.type;
	if (thumbnail.type == MEDIATYPE_PICTURE) // TODO: Perhaps also support caching of video thumbnails
		stream << thumbnail.img;
	file.commit();
}

void Thumbnailer::recalculate(QString filename)
{
	Thumbnail thumbnail = getHashedImage(filename, true);

	// If we couldn't load the image from disk -> leave old thumbnail.
	// The case "load from web" is a bit inconsistent: it will call into processItem() later
	// and therefore a "broken" image symbol may be shown.
	if (thumbnail.type == MEDIATYPE_STILL_LOADING || thumbnail.type == MEDIATYPE_IO_ERROR)
		return;
	addThumbnailToCache(thumbnail, filename);

	QMutexLocker l(&lock);
	emit thumbnailChanged(filename, thumbnail.img);
	workingOn.remove(filename);
}

void Thumbnailer::processItem(QString filename, bool tryDownload)
{
	Thumbnail thumbnail = getThumbnailFromCache(filename);

	if (thumbnail.img.isNull()) {
		thumbnail = getHashedImage(filename, tryDownload);
		if (thumbnail.type == MEDIATYPE_STILL_LOADING)
			return;

		if (thumbnail.img.isNull()) {
			thumbnail.img = failImage;
		} else {
			int size = maxThumbnailSize();
			thumbnail.img = thumbnail.img.scaled(size, size, Qt::KeepAspectRatio);
			addThumbnailToCache(thumbnail, filename);
		}
	}

	QMutexLocker l(&lock);
	emit thumbnailChanged(filename, thumbnail.img);
	workingOn.remove(filename);
}

void Thumbnailer::imageDownloaded(QString filename)
{
	// Image was downloaded -> try thumbnailing again.
	QMutexLocker l(&lock);
	workingOn[filename] = QtConcurrent::run(&pool, [this, filename]() { processItem(filename, false); });
}

void Thumbnailer::imageDownloadFailed(QString filename)
{
	emit thumbnailChanged(filename, failImage);
	QMutexLocker l(&lock);
	workingOn.remove(filename);
}

QImage Thumbnailer::fetchThumbnail(const QString &filename)
{
	QMutexLocker l(&lock);

	// We are not currently fetching this thumbnail - add it to the list.
	if (!workingOn.contains(filename)) {
		workingOn.insert(filename,
				 QtConcurrent::run(&pool, [this, filename]() { processItem(filename, true); }));
	}
	return dummyImage;
}

void Thumbnailer::calculateThumbnails(const QVector<QString> &filenames)
{
	QMutexLocker l(&lock);
	for (const QString &filename: filenames) {
		if (!workingOn.contains(filename)) {
			workingOn.insert(filename,
					 QtConcurrent::run(&pool, [this, filename]() { recalculate(filename); }));
		}
	}
}

void Thumbnailer::clearWorkQueue()
{
	QMutexLocker l(&lock);
	for (auto it = workingOn.begin(); it != workingOn.end(); ++it)
		it->cancel();
	workingOn.clear();
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
