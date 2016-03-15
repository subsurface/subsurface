#ifndef IMAGEDOWNLOADER_H
#define IMAGEDOWNLOADER_H

#include <QImage>
#include <QFuture>
#include <QNetworkReply>

typedef QPair<QString, QByteArray> SHashedFilename;

extern QUrl cloudImageURL(const char *hash);


class ImageDownloader : public QObject {
	Q_OBJECT;
public:
	ImageDownloader(struct picture *picture);
	~ImageDownloader();
	void load(bool fromHash);

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
