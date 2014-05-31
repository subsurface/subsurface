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

SPixmap scaleImages(const QString& s){
	QPixmap p = QPixmap(s).scaled(128,128, Qt::KeepAspectRatio);
	SPixmap ret;
	ret.first = s;
	ret.second = p;
	return ret;
}

void DivePictureModel::updateDivePictures(int divenr)
{
	qDebug() << "Updating dive pictures.";
	beginRemoveRows(QModelIndex(), 0, numberOfPictures-1);
	numberOfPictures = 0;
	endRemoveRows();

	QStringList pictures;
	struct dive *d = get_dive(divenr);
	if (!d){
		qDebug() << "Got no dive, exiting.";
		return;
	}
	// All pictures are set in *all* divecomputers. ( waste of memory if > 100 pictures? )
	// so just get from the first one.
	struct event *ev = d->dc.events;
	while(ev){
		if(ev->type == 123){ // 123 means PICTURE.
			numberOfPictures++;
			pictures.push_back(QString(ev->name));
		}
		ev = ev->next;
	}

	SPixmapList retList = QtConcurrent::blockingMapped<SPixmapList>( pictures, scaleImages);
	Q_FOREACH(const SPixmap & pixmap, retList)
		stringPixmapCache[pixmap.first] = pixmap.second;

	if (numberOfPictures == 0){
		qDebug() << "Got no pictures, exiting.";
		return;
	}
	beginInsertRows(QModelIndex(), 0, numberOfPictures-1);
	endInsertRows();
	qDebug() << "Everything Ok.";
}

int DivePictureModel::columnCount(const QModelIndex &parent) const
{

}

QVariant DivePictureModel::data(const QModelIndex &index, int role) const
{

}

int DivePictureModel::rowCount(const QModelIndex &parent) const
{
	return numberOfPictures;
}

DivePictureWidget::DivePictureWidget(QWidget *parent): QListView(parent)
{

}
