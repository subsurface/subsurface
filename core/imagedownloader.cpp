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

static QUrl cloudImageURL(const char *filename)
{
	QString hash = hashString(filename);
	return QUrl::fromUserInput(QString("https://cloud.subsurface-divelog.org/images/").append(hash));
}

ImageDownloader::ImageDownloader(const QString &filename_in) : filename(filename_in)
{
}

void ImageDownloader::load(bool fromHash)
{
	if (fromHash && loadFromUrl(cloudImageURL(qPrintable(filename))))
		return;

	// If loading from hash failed, try to load from filename
	loadFromUrl(QUrl::fromUserInput(filename));
}

bool ImageDownloader::loadFromUrl(const QUrl &url)
{
	bool success = false;
	if (url.isValid()) {
		QEventLoop loop;
		QNetworkAccessManager manager;
		QNetworkRequest request(url);
		connect(&manager, &QNetworkAccessManager::finished, this,
			[this,&success] (QNetworkReply *reply) { saveImage(reply, success); });
		connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
		qDebug() << "Downloading image from" << url;
		QNetworkReply *reply = manager.get(request);
		loop.exec();
		delete reply;
	}
	return success;
}

void ImageDownloader::saveImage(QNetworkReply *reply, bool &success)
{
	success = false;
	QByteArray imageData = reply->readAll();
	QImage image;
	image.loadFromData(imageData);
	if (image.isNull())
		return;
	success = true;
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
	// This should be called to make the picture actually show.
	// Problem is DivePictureModel is not in core.
	// Nevertheless, the image shows when the dive is selected the next time.
	// DivePictureModel::instance()->updateDivePictures();

}

static void loadPicture(QString filename, bool fromHash)
{
	static QSet<QString> queuedPictures;
	static QMutex pictureQueueMutex;

	QMutexLocker locker(&pictureQueueMutex);
	if (queuedPictures.contains(filename))
		return;
	queuedPictures.insert(filename);
	locker.unlock();

	ImageDownloader download(filename);
	download.load(fromHash);
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

QImage getHashedImage(const QString &file)
{
	QImage res;
	QUrl url = QUrl::fromUserInput(localFilePath(file));
	if (url.isLocalFile())
		res = loadImage(url.toLocalFile());
	if (res.isNull()) {
		// This did not load anything. Let's try to get the image from other sources
		// Let's try to load it locally via its hash
		QString filenameLocal = localFilePath(qPrintable(file));
		qDebug() << QStringLiteral("Translated filename: %1 -> %2").arg(file, filenameLocal);
		if (filenameLocal.isNull()) {
			// That didn't produce a local filename.
			// Try the cloud server
			// TODO: This is dead code at the moment.
			loadPicture(file, true);
		} else {
			// Load locally from translated file name
			res = loadImage(filenameLocal);
			if (!res.isNull()) {
				// Make sure the hash still matches the image file
				hashPicture(filenameLocal);
			} else {
				// Interpret filename as URL
				loadPicture(filenameLocal, false);
			}
		}
	} else {
		// We loaded successfully. Now, make sure hash is up to date.
		hashPicture(file);
	}
	return res;
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
}

Thumbnailer *Thumbnailer::instance()
{
	static Thumbnailer self;
	return &self;
}

static QImage getThumbnailFromCache(const QString &picture_filename)
{
	// First, check if we know a hash for this filename
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

	// If we got a thumbnail, we are guaranteed to have its hash and therefore
	// thumbnailFileName() should return a filename.
	if (filename.isEmpty()) {
		qWarning() << "Internal error: can't get filename of recently created thumbnail";
		return;
	}

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

void Thumbnailer::processItem(QString filename)
{
	QImage thumbnail = getThumbnailFromCache(filename);

	if (thumbnail.isNull()) {
		thumbnail = getHashedImage(filename);
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

QImage Thumbnailer::fetchThumbnail(PictureEntry &entry)
{
	QMutexLocker l(&lock);

	// We are not currently fetching this thumbnail - add it to the list.
	const QString &filename = entry.filename;
	if (!workingOn.contains(filename)) {
		workingOn.insert(filename,
				 QtConcurrent::run(&pool, [this, filename]() { processItem(filename); }));
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
