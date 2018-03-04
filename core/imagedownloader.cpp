// SPDX-License-Identifier: GPL-2.0
#include "dive.h"
#include "metrics.h"
#include "divelist.h"
#include "qthelper.h"
#include "imagedownloader.h"
#include <unistd.h>
#include <QString>
#include <QImageReader>

#include <QtConcurrent>

static QUrl cloudImageURL(const char *filename)
{
	QString hash = hashString(filename);
	return QUrl::fromUserInput(QString("https://cloud.subsurface-divelog.org/images/").append(hash));
}

ImageDownloader::ImageDownloader(struct picture *pic)
{
	picture = pic;
}

ImageDownloader::~ImageDownloader()
{
	picture_free(picture);
}

void ImageDownloader::load(bool fromHash)
{
	if (fromHash && loadFromUrl(cloudImageURL(picture->filename)))
		return;

	// If loading from hash failed, try to load from filename
	loadFromUrl(QUrl::fromUserInput(QString(picture->filename)));
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
		learnHash(QString(picture->filename), imageFile.fileName(), hash.result());
	}
	// This should be called to make the picture actually show.
	// Problem is DivePictureModel is not in core.
	// Nevertheless, the image shows when the dive is selected the next time.
	// DivePictureModel::instance()->updateDivePictures();

}

static void loadPicture(struct picture *picture, bool fromHash)
{
	static QSet<QString> queuedPictures;
	static QMutex pictureQueueMutex;

	if (!picture)
		return;
	QMutexLocker locker(&pictureQueueMutex);
	if (queuedPictures.contains(QString(picture->filename))) {
		picture_free(picture);
		return;
	}
	queuedPictures.insert(QString(picture->filename));
	locker.unlock();

	ImageDownloader download(picture);
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

std::pair<QImage,bool> getHashedImage(struct picture *picture)
{
	std::pair<QImage,bool> res { {}, false };
	QUrl url = QUrl::fromUserInput(localFilePath(QString(picture->filename)));
	if (url.isLocalFile())
		res = loadImage(url.toLocalFile());
	if (res.first.isNull() && !res.second) {
		// This did not load anything. Let's try to get the image from other sources
		// Let's try to load it locally via its hash
		QString filename = localFilePath(picture->filename);
		qDebug() << QStringLiteral("Translated filename: %1 -> %2").arg(picture->filename, filename);
		if (filename.isNull()) {
			// That didn't produce a local filename.
			// Try the cloud server
			// TODO: This is dead code at the moment.
			QtConcurrent::run(loadPicture, clone_picture(picture), true);
		} else {
			// Load locally from translated file name
			res = loadImage(url.toLocalFile());
			if (!res.first.isNull() || res.second) {
				// Make sure the hash still matches the image file
				qDebug() << "Loaded picture from translated filename" << filename;
				QtConcurrent::run(hashPicture, clone_picture(picture));
			} else {
				// Interpret filename as URL
				qInfo() << "Failed loading picture from translated filename" << filename;
				QtConcurrent::run(loadPicture, clone_picture(picture), false);
			}
		}
	} else {
		// We loaded successfully. Now, make sure hash is up to date.
		QtConcurrent::run(hashPicture, clone_picture(picture));
	}
	return res;
}
