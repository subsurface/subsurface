#include "divepicturewidget.h"
#include <dive.h>

void DivePictureDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyledItemDelegate::paint(painter, option, index);
}

DivePictureModel::DivePictureModel(QObject *parent): QAbstractTableModel(parent)
{

}

void DivePictureModel::updateDivePictures(int divenr)
{
	beginRemoveRows(QModelIndex(), 0, numberOfPictures-1);
	numberOfPictures = 0;
	endRemoveRows();

	struct dive *d = get_dive(divenr);
	if (!d)
		return;
	// All pictures are set in *all* divecomputers. ( waste of memory if > 100 pictures? )
	// so just get from the first one.
	struct event *ev = d->dc.events;
	while(ev){
		if(ev->type == 123){ // 123 means PICTURE.
			numberOfPictures++;
		}
		ev = ev->next;
	}

	if (numberOfPictures == 0)
		return;
	beginInsertRows(QModelIndex(), 0, numberOfPictures-1);
	endInsertRows();
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
