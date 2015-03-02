#ifndef DIVEPICTUREWIDGET_H
#define DIVEPICTUREWIDGET_H

#include <QAbstractTableModel>
#include <QListView>
#include <QThread>
#include <QFuture>
#include <QNetworkReply>

typedef QPair<QString, QByteArray> SHashedFilename;

struct PhotoHelper {
	QImage image;
	int offsetSeconds;
};

class SHashedImage : public QImage {
public:
	SHashedImage(struct picture *picture);
};

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

class DivePictureModel : public QAbstractTableModel {
	Q_OBJECT
public:
	static DivePictureModel *instance();
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	void updateDivePictures();
	void updateDivePicturesWhenDone(QList<QFuture<void> >);
	void removePicture(const QString& fileUrl);

private:
	DivePictureModel();
	int numberOfPictures;
	// Currently, load the images on the fly
	// Later, use a thread to load the images
	// Later, save the thumbnails so we don't need to reopen every time.
	QHash<QString, PhotoHelper> stringPixmapCache;
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
