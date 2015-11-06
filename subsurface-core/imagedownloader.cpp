#include "dive.h"
#include "metrics.h"
#include "divelist.h"
#include "qthelper.h"
#include "imagedownloader.h"
#include <unistd.h>

#include <QtConcurrent>

ImageDownloader::ImageDownloader(struct picture *pic)
{
	picture = pic;
}

void ImageDownloader::load(){
	QUrl url = QUrl::fromUserInput(QString(picture->filename));
	if (url.isValid()) {
		QEventLoop loop;
		QNetworkRequest request(url);
		connect(&manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(saveImage(QNetworkReply *)));
		QNetworkReply *reply = manager.get(request);
		while (reply->isRunning()) {
			loop.processEvents();
			sleep(1);
		}
	}
}

void ImageDownloader::saveImage(QNetworkReply *reply)
{
	QByteArray imageData = reply->readAll();
	QImage image = QImage();
	image.loadFromData(imageData);
	if (image.isNull())
		return;
	QCryptographicHash hash(QCryptographicHash::Sha1);
	hash.addData(imageData);
	QString path = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first();
	QDir dir(path);
	if (!dir.exists())
		dir.mkpath(path);
	QFile imageFile(path.append("/").append(hash.result().toHex()));
	if (imageFile.open(QIODevice::WriteOnly)) {
		QDataStream stream(&imageFile);
		stream.writeRawData(imageData.data(), imageData.length());
		imageFile.waitForBytesWritten(-1);
		imageFile.close();
		add_hash(imageFile.fileName(), hash.result());
		learnHash(picture, hash.result());
	}
	reply->manager()->deleteLater();
	reply->deleteLater();
}

void loadPicture(struct picture *picture)
{
	ImageDownloader download(picture);
	download.load();
}

SHashedImage::SHashedImage(struct picture *picture) : QImage()
{
	QUrl url = QUrl::fromUserInput(QString(picture->filename));
	if(url.isLocalFile())
		load(url.toLocalFile());
	if (isNull()) {
		// Hash lookup.
		load(fileFromHash(picture->hash));
		if (!isNull()) {
			QtConcurrent::run(updateHash, picture);
		} else {
			QtConcurrent::run(loadPicture, picture);
		}
	} else {
		QByteArray hash = hashFile(url.toLocalFile());
		free(picture->hash);
		picture->hash = strdup(hash.toHex().data());
	}
}
