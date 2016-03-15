#ifndef DIVEPICTUREWIDGET_H
#define DIVEPICTUREWIDGET_H

#include <QAbstractTableModel>
#include <QListView>
#include <QThread>
#include <QFuture>
#include <QNetworkReply>

class ImageDownloader : public QObject {
	Q_OBJECT;
public:
	ImageDownloader(struct picture *picture);
	~ImageDownloader();
	void load();
private:
	struct picture *picture;
	QNetworkAccessManager manager;
private slots:
	void saveImage(QNetworkReply *reply);
};

class DivePictureWidget : public QListView {
	Q_OBJECT
public:
	DivePictureWidget(QWidget *parent);
signals:
	void photoDoubleClicked(const QString filePath);
private
slots:
	void doubleClicked(const QModelIndex &index);
};

class DivePictureThumbnailThread : public QThread {
};

#endif
