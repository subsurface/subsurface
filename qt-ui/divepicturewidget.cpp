#include "divepicturewidget.h"

void DivePictureDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyledItemDelegate::paint(painter, option, index);
}

int DivePictureModel::columnCount(const QModelIndex &parent) const
{

}

QVariant DivePictureModel::data(const QModelIndex &index, int role) const
{

}

int DivePictureModel::rowCount(const QModelIndex &parent) const
{

}

DivePictureView::DivePictureView(QWidget *parent): QListView(parent)
{

}
