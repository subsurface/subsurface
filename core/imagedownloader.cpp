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

std::pair<QImage,bool> getHashedImage(const QString &file)
{
	std::pair<QImage,bool> res { {}, false };
	QUrl url = QUrl::fromUserInput(localFilePath(file));
	if (url.isLocalFile())
		res = loadImage(url.toLocalFile());
	if (res.first.isNull() && !res.second) {
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
			if (!res.first.isNull() || res.second) {
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

Thumbnailer *Thumbnailer::instance()
{
	static Thumbnailer self;
	return &self;
}

void Thumbnailer::processItem(QString filename)
{
	auto res = getHashedImage(filename);
	QImage thumbnail = res.first;
	bool isVideo = res.second;
	int size = maxThumbnailSize();

	if (thumbnail.isNull() && !isVideo) {
		// TODO: Don't misuse filter close icon
		thumbnail = QImage(":filter-close").scaled(size, size, Qt::KeepAspectRatio);
	} else {
		thumbnail = isVideo ?
			QImage(":video-icon").scaled(size, size, Qt::KeepAspectRatio) :
			res.first.scaled(size, size, Qt::KeepAspectRatio);
		QMutexLocker l(&lock);
		if (isVideo)
			videoThumbnailCache.insert(filename, QImage());
		else
			thumbnailCache.insert(filename, thumbnail);
	}
	emit thumbnailChanged(filename, thumbnail, isVideo);
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
