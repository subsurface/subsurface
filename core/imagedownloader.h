// SPDX-License-Identifier: GPL-2.0
#ifndef IMAGEDOWNLOADER_H
#define IMAGEDOWNLOADER_H

#include <QImage>
#include <QFuture>
#include <QNetworkReply>

class ImageDownloader : public QObject {
	Q_OBJECT
public:
	ImageDownloader(const QString &filename);
	void load(bool fromHash);

private:
	bool loadFromUrl(const QUrl &);	// return true on success
	void saveImage(QNetworkReply *reply, bool &success);
	QString filename;
};

QImage getHashedImage(const QString &filename);

#endif // IMAGEDOWNLOADER_H
