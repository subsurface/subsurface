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
				hashPicture(filename);	// hashPicture transforms canonical into local filename
			}
			emit loaded(filename);
		}
	}

	reply->deleteLater();
}

// Fetch a picture from the given filename. If this is a non-remote filename, fetch it from disk.
// Remote files are fetched from the net in a background thread. In such a case, the output-flag
// "stillLoading" is set to true.
// If the input-flag "tryDownload" is set to false, no download attempt is made. This is to
// prevent infinite loops, where failed image downloads would be repeated ad infinitum.
// Returns: fetched image, stillLoading flag
static std::pair<QImage, bool> fetchImage(const QString &filename, const QString &originalFilename, bool tryDownload)
{
	QImage thumb;
	bool stillLoading = false;
	QUrl url = QUrl::fromUserInput(filename);
	if (url.isLocalFile()) {
		thumb.load(url.toLocalFile());
		// If we loaded successfully, make sure the hash is up to date.
		// Note that hashPicture() takes the *original* filename.
		if (!thumb.isNull())
			hashPicture(originalFilename);
	} else if (tryDownload) {
		// This has to be done in UI main thread, because QNetworkManager refuses
		// to treat requests from other threads. invokeMethod() is Qt's way of calling a
		// function in a different thread, namely the thread the called object is associated to.
		QMetaObject::invokeMethod(ImageDownloader::instance(), "load", Qt::AutoConnection, Q_ARG(QUrl, url), Q_ARG(QString, originalFilename));
		stillLoading = true;
	}
	return { thumb, stillLoading };
}

// Fetch a picture based on its original filename. If there is a translated filename (obtained either
// by the find-moved-picture functionality or the filename of the local cache of a remote picture),
// try that first. If fetching from the translated filename fails, this could mean that the image
// was downloaded previously, but for some reason the cached picture was lost. Therefore, in such a
// case, try the canonical filename. If that likewise fails, give up. For input and output parameters
// see fetchImage() above.
static std::pair<QImage, bool> getHashedImage(const QString &filename, bool tryDownload)
{
	QImage thumb;
	bool stillLoading = false;
	QString localFilename = localFilePath(filename);

	// If there is a translated filename, try that first
	if (localFilename != filename)
		std::tie(thumb, stillLoading) = fetchImage(localFilename, filename, tryDownload);

	// Note that the translated filename should never be a remote file and therefore checking for
	// stillLoading is currently not necessary. But in the future, we might support such a use case
	// (e.g. images stored in the cloud).
	if (thumb.isNull() && !stillLoading)
		std::tie(thumb, stillLoading) = fetchImage(filename, filename, tryDownload);

	if (thumb.isNull() && !stillLoading)
		qInfo() << "Error loading image" << filename << "[local:" << localFilename << "]";
	return { thumb, stillLoading };
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
			     dummyImage(renderIcon(":camera-icon", maxThumbnailSize()))
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

static QImage getThumbnailFromCache(const QString &picture_filename)
{
	QString filename = thumbnailFileName(picture_filename);
	if (filename.isEmpty())
		return QImage();
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
				return QImage();
			}
		}
	}

	if (!file.open(QIODevice::ReadOnly))
		return QImage();
	QDataStream stream(&file);

	// Each thumbnail file is composed of a media-type and an image file.
	// Currently, the type is ignored. This will be used to mark videos.
	quint32 type;
	QImage res;
	stream >> type;
	stream >> res;
	return res;
}

static void addThumbnailToCache(const QImage &thumbnail, const QString &picture_filename)
{
	if (thumbnail.isNull())
		return;

	QString filename = thumbnailFileName(picture_filename);
	QSaveFile file(filename);
	if (!file.open(QIODevice::WriteOnly))
		return;
	QDataStream stream(&file);

	// For format of the file, see comments in getThumnailForCache
	quint32 type = MEDIATYPE_PICTURE;
	stream << type;
	stream << thumbnail;
	file.commit();
}

void Thumbnailer::recalculate(QString filename)
{
	auto res = getHashedImage(filename, true);

	// If we couldn't load the image from disk -> leave old thumbnail.
	// The case "load from web" is a bit inconsistent: it will call into processItem() later
	// and therefore a "broken" image symbol may be shown.
	if (res.second || res.first.isNull())
		return;
	QImage thumbnail = res.first;
	addThumbnailToCache(thumbnail, filename);

	QMutexLocker l(&lock);
	emit thumbnailChanged(filename, thumbnail);
	workingOn.remove(filename);
}

void Thumbnailer::processItem(QString filename, bool tryDownload)
{
	QImage thumbnail = getThumbnailFromCache(filename);

	if (thumbnail.isNull()) {
		auto res = getHashedImage(filename, tryDownload);
		if (res.second)
			return;
		thumbnail = res.first;

		if (thumbnail.isNull()) {
			thumbnail = failImage;
		} else {
			int size = maxThumbnailSize();
			thumbnail = thumbnail.scaled(size, size, Qt::KeepAspectRatio);
			addThumbnailToCache(thumbnail, filename);
		}
	}

	QMutexLocker l(&lock);
	emit thumbnailChanged(filename, thumbnail);
	workingOn.remove(filename);
}

void Thumbnailer::imageDownloaded(QString filename)
{
	// Image was downloaded and the filename connected with a hash.
	// Try thumbnailing again.
	QMutexLocker l(&lock);
	workingOn[filename] = QtConcurrent::run(&pool, [this, filename]() { processItem(filename, false); });
}

void Thumbnailer::imageDownloadFailed(QString filename)
{
	emit thumbnailChanged(filename, failImage);
	QMutexLocker l(&lock);
	workingOn.remove(filename);
}

QImage Thumbnailer::fetchThumbnail(PictureEntry &entry)
{
	QMutexLocker l(&lock);

	// We are not currently fetching this thumbnail - add it to the list.
	const QString &filename = entry.filename;
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
