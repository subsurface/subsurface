// SPDX-License-Identifier: GPL-2.0
#ifndef IMAGEDOWNLOADER_H
#define IMAGEDOWNLOADER_H

#include <QImage>
#include <QFuture>
#include <QNetworkReply>

class ImageDownloader : public QObject {
	Q_OBJECT;
public:
	ImageDownloader(struct picture *picture);
	~ImageDownloader();
	void load(bool fromHash);

private:
	struct picture *picture;
	QNetworkAccessManager manager;
	bool loadFromHash;

private slots:
	void saveImage(QNetworkReply *reply);
};

class SHashedImage : public QImage {
public:
	SHashedImage(struct picture *picture);
};

#endif // IMAGEDOWNLOADER_H
