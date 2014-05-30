#ifndef DIVEPICTUREWIDGET_H
#define DIVEPICTUREWIDGET_H

#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QListView>
#include <QThread>

class DivePictureModel : QAbstractTableModel {
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
};

class DivePictureDelegate : QStyledItemDelegate {
	virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class DivePictureView : public QListView{
	Q_OBJECT
public:
	DivePictureView(QWidget *parent);
};

class DivePictureThumbnailThread : public QThread {

};

#endif