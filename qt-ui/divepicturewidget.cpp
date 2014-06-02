#include "divepicturewidget.h"
#include <dive.h>
#include <qtconcurrentmap.h>

void DivePictureDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyledItemDelegate::paint(painter, option, index);
}

DivePictureModel::DivePictureModel(QObject *parent): QAbstractTableModel(parent)
{
}

typedef QPair<QString, QPixmap> SPixmap;
typedef QList<SPixmap> SPixmapList;

SPixmap scaleImages(const QString& s) {
	QPixmap p = QPixmap(s).scaled(128,128, Qt::KeepAspectRatio);
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
		case Qt::DisplayRole : ret = key; break;
		case Qt::DecorationRole : ret = stringPixmapCache[key]; break;
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
