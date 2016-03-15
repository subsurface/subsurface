#include "divepicturewidget.h"
#include "divepicturemodel.h"
#include "metrics.h"
#include "dive.h"
#include "divelist.h"
#include <unistd.h>
#include <QtConcurrentMap>
#include <QtConcurrentRun>
#include <QFuture>
#include <QDir>
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <mainwindow.h>
#include <qthelper.h>
#include <QStandardPaths>

void loadPicture(struct picture *picture)
{
	ImageDownloader download(picture);
	download.load();
}

SHashedImage::SHashedImage(struct picture *picture) : QImage()
{
	QUrl url = QUrl::fromUserInput(localFilePath(QString(picture->filename)));

	if(url.isLocalFile())
		load(url.toLocalFile());
	if (isNull()) {
		// Hash lookup.
		load(fileFromHash(picture->hash));
		if (!isNull()) {
			QtConcurrent::run(updateHash, clone_picture(picture));
		} else {
			QtConcurrent::run(loadPicture, clone_picture(picture));
		}
	} else {
		QByteArray hash = hashFile(url.toLocalFile());
		free(picture->hash);
		picture->hash = strdup(hash.toHex().data());
	}
}

ImageDownloader::ImageDownloader(struct picture *pic)
{
	picture = pic;
}

ImageDownloader::~ImageDownloader()
{
	picture_free(picture);
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
	qDebug() << "downloaded ";
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
		DivePictureModel::instance()->updateDivePictures();
	}
	reply->manager()->deleteLater();
	reply->deleteLater();
}

DivePictureWidget::DivePictureWidget(QWidget *parent) : QListView(parent)
{
	connect(this, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(doubleClicked(const QModelIndex &)));
}

void DivePictureWidget::doubleClicked(const QModelIndex &index)
{
	QString filePath = model()->data(index, Qt::DisplayPropertyRole).toString();
	emit photoDoubleClicked(localFilePath(filePath));
}
