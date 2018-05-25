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

void ImageDownloader::load(QString filename)
{
	QUrl url = QUrl::fromUserInput(filename);

	// If this is a file, we tried previously -> don't bother trying it again
	if (url.scheme() == "file" || !url.isValid()) {
		emit failed(filename);
		return;
	}

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

static void loadPicture(QString filename)
{
	// This has to be done in UI main thread, because QNetworkManager refuses
	// to treat requests from other threads.
	QMetaObject::invokeMethod(ImageDownloader::instance(), "load", Qt::AutoConnection, Q_ARG(QString, filename));
}

// Overwrite QImage::load() so that we can perform better error reporting.
static QImage loadImage(const QString &fileName, const char *format = nullptr)
{
	QImageReader reader(fileName, format);
	QImage res = reader.read();
	if (res.isNull())
		qInfo() << "Error loading image" << fileName << (int)reader.error() << reader.errorString();
	return res;
}

// Returns: thumbnail, still loading
static std::pair<QImage,bool> getHashedImage(const QString &file_in, bool tryDownload)
{
	QString file = file_in.startsWith("file://", Qt::CaseInsensitive) ? file_in.mid(7) : file_in;
	QImage thumb;
	bool stillLoading = false;
	QUrl url = QUrl::fromUserInput(localFilePath(file));
	if (url.isLocalFile())
		thumb = loadImage(url.toLocalFile());
	if (!thumb.isNull()) {
		// We loaded successfully. Now, make sure hash is up to date.
		hashPicture(file);
	} else if (tryDownload) {
		// This did not load anything. Let's try to get the image from other sources
		QString filenameLocal = localFilePath(qPrintable(file));
		qDebug() << QStringLiteral("Translated filename: %1 -> %2").arg(file, filenameLocal);
		// Load locally from translated file name if it is different
		if (filenameLocal != file)
			thumb = loadImage(filenameLocal);
		if (!thumb.isNull()) {
			// Make sure the hash still matches the image file
			hashPicture(filenameLocal);
		} else {
			// Interpret filename as URL
			loadPicture(filenameLocal);
			stillLoading = true;
		}
	}
	return { thumb, stillLoading };
}

static QImage renderIcon(const char *id, int size)
{
	QImage res(size, size, QImage::Format_ARGB32);
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
