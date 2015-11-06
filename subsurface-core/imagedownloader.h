#ifndef IMAGEDOWNLOADER_H
#define IMAGEDOWNLOADER_H

#include <QImage>
#include <QFuture>
#include <QNetworkReply>

typedef QPair<QString, QByteArray> SHashedFilename;

class ImageDownloader : public QObject {
	Q_OBJECT;
public:
	ImageDownloader(struct picture *picture);
	void load();

private:
	struct picture *picture;
	QNetworkAccessManager manager;

private slots:
	void saveImage(QNetworkReply *reply);
};

class SHashedImage : public QImage {
public:
	SHashedImage(struct picture *picture);
};

#endif // IMAGEDOWNLOADER_H
