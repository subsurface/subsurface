#include "divepicturewidget.h"
#include <dive.h>
#include <qtconcurrentmap.h>
#include <qdir.h>

void DivePictureDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyledItemDelegate::paint(painter, option, index);
}

DivePictureModel *DivePictureModel::instance()
{
	static DivePictureModel* self = new DivePictureModel();
	return self;
}

DivePictureModel::DivePictureModel()
{
}

typedef QPair<QString, QImage> SPixmap;
typedef QList<SPixmap> SPixmapList;

SPixmap scaleImages(const QString& s) {
	QImage p = QImage(s).scaled(128,128, Qt::KeepAspectRatio);
	SPixmap ret;
	ret.first = s;
	ret.second = p;
	return ret;
}

void DivePictureModel::updateDivePictures(int divenr)
{
	if (numberOfPictures != 0) {
		beginRemoveRows(QModelIndex(), 0, numberOfPictures-1);
		numberOfPictures = 0;
		endRemoveRows();
	}

	struct dive *d = get_dive(divenr);
	numberOfPictures = dive_get_picture_count(d);
	if (!d || numberOfPictures == 0) {
		return;
	}

	QStringList pictures;
	FOR_EACH_PICTURE( d ) {
		pictures.push_back(QString(picture->filename));
	}

	stringPixmapCache.clear();
	SPixmapList retList = QtConcurrent::blockingMapped<SPixmapList>( pictures, scaleImages);
	Q_FOREACH(const SPixmap & pixmap, retList)
		stringPixmapCache[pixmap.first] = pixmap.second;

	beginInsertRows(QModelIndex(), 0, numberOfPictures-1);
	endInsertRows();
}

int DivePictureModel::columnCount(const QModelIndex &parent) const
{
	return 1;
}

QVariant DivePictureModel::data(const QModelIndex &index, int role) const
{
	QVariant ret;
	if (!index.isValid())
		return ret;

	QString key = stringPixmapCache.keys().at(index.row());
	switch(role){
		case Qt::ToolTipRole : ret = key; break;
		case Qt::DecorationRole : ret = stringPixmapCache[key]; break;
		case Qt::DisplayRole : ret = QFileInfo(key).fileName();
	}
	return ret;
}

int DivePictureModel::rowCount(const QModelIndex &parent) const
{
	return numberOfPictures;
}

DivePictureWidget::DivePictureWidget(QWidget *parent): QListView(parent)
{
}
