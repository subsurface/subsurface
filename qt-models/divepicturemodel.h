#ifndef DIVEPICTUREMODEL_H
#define DIVEPICTUREMODEL_H

#include <QAbstractTableModel>
#include <QImage>
#include <QFuture>

struct PhotoHelper {
	QImage image;
	int offsetSeconds;
};

typedef QList<struct picture *> SPictureList;
typedef struct picture *picturepointer;
typedef QPair<picturepointer, QImage> SPixmap;

// function that will scale the pixmap, used inside the QtConcurrent thread.
SPixmap scaleImages(picturepointer picture);

class DivePictureModel : public QAbstractTableModel {
	Q_OBJECT
public:
	static DivePictureModel *instance();
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual void updateDivePictures();
	void updateDivePicturesWhenDone(QList<QFuture<void> >);
	void removePicture(const QString& fileUrl, bool last);

protected:
	DivePictureModel();
	int numberOfPictures;
	// Currently, load the images on the fly
	// Later, use a thread to load the images
	// Later, save the thumbnails so we don't need to reopen every time.
	QHash<QString, PhotoHelper> stringPixmapCache;
};

#endif
