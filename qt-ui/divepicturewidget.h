#ifndef DIVEPICTUREWIDGET_H
#define DIVEPICTUREWIDGET_H

#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QListView>
#include <QThread>

class DivePictureModel : public QAbstractTableModel {
Q_OBJECT
public:
	DivePictureModel(QObject *parent);
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	void updateDivePictures(int divenr);
private:
	int numberOfPictures;
	// Currently, load the images on the fly
	// Later, use a thread to load the images
	// Later, save the thumbnails so we don't need to reopen every time.
	QHash<QString, QImage> stringPixmapCache;
};

class DivePictureDelegate : QStyledItemDelegate {
	virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class DivePictureWidget : public QListView{
	Q_OBJECT
public:
	DivePictureWidget(QWidget *parent);
};

class DivePictureThumbnailThread : public QThread {

};

#endif