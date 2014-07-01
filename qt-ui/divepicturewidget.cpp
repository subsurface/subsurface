#include "divepicturewidget.h"
#include <dive.h>
#include <QtConcurrentMap>
#include <QDir>
#include <QHash>

DivePictureModel *DivePictureModel::instance()
{
	static DivePictureModel *self = new DivePictureModel();
	return self;
}

DivePictureModel::DivePictureModel() : numberOfPictures(0)
{
}

typedef QPair<QString, QImage> SPixmap;
typedef QList<SPixmap> SPixmapList;

SPixmap scaleImages(const QString &s)
{
	static QHash <QString, QImage > cache;
	SPixmap ret;
	ret.first = s;
	if (cache.contains(s)) {
		ret.second = cache.value(s);
	} else {
		QImage p = QImage(s).scaled(128, 128, Qt::KeepAspectRatio);
		cache.insert(s, p);
		ret.second = p;
	}
	return ret;
}

void DivePictureModel::updateDivePictures(int divenr)
{
	if (numberOfPictures != 0) {
		beginRemoveRows(QModelIndex(), 0, numberOfPictures - 1);
		numberOfPictures = 0;
		endRemoveRows();
	}

	struct dive *d = get_dive(divenr);
	numberOfPictures = dive_get_picture_count(d);
	if (!d || numberOfPictures == 0) {
		return;
	}

	stringPixmapCache.clear();
	QStringList pictures;
	FOR_EACH_PICTURE (d) {
		stringPixmapCache[QString(picture->filename)].picture = picture;
		pictures.push_back(QString(picture->filename));
	}


	SPixmapList retList = QtConcurrent::blockingMapped<SPixmapList>(pictures, scaleImages);
	Q_FOREACH (const SPixmap &pixmap, retList)
		stringPixmapCache[pixmap.first].image = pixmap.second;

	beginInsertRows(QModelIndex(), 0, numberOfPictures - 1);
	endInsertRows();
}

int DivePictureModel::columnCount(const QModelIndex &parent) const
{
	return 2;
}

QVariant DivePictureModel::data(const QModelIndex &index, int role) const
{
	QVariant ret;
	if (!index.isValid())
		return ret;

	QString key = stringPixmapCache.keys().at(index.row());
	if (index.column() == 0) {
		switch (role) {
		case Qt::ToolTipRole:
			ret = key;
			break;
		case Qt::DecorationRole:
			ret = stringPixmapCache[key].image;
			break;
		case Qt::DisplayRole:
			ret = QFileInfo(key).fileName();
			break;
		case Qt::DisplayPropertyRole:
			ret = QFileInfo(key).filePath();
		}
	} else if (index.column() == 1) {
		switch (role) {
		case Qt::UserRole:
			ret = QVariant::fromValue((void *)stringPixmapCache[key].picture);
		}
	}
	return ret;
}

int DivePictureModel::rowCount(const QModelIndex &parent) const
{
	return numberOfPictures;
}

DivePictureWidget::DivePictureWidget(QWidget *parent) : QListView(parent)
{
	connect(this, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(doubleClicked(const QModelIndex &)));
}

void DivePictureWidget::doubleClicked(const QModelIndex &index)
{
	QString filePath = model()->data(index, Qt::DisplayPropertyRole).toString();
	emit photoDoubleClicked(filePath);
}
