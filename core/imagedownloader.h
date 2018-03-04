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
	bool loadFromUrl(const QUrl &);	// return true on success
	void saveImage(QNetworkReply *reply, bool &success);
	struct picture *picture;
};

QImage getHashedImage(struct picture *picture);

#endif // IMAGEDOWNLOADER_H
