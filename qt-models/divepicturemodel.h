// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEPICTUREMODEL_H
#define DIVEPICTUREMODEL_H

#include <QAbstractTableModel>
#include <QImage>
#include <QFuture>

struct PictureEntry {
	struct picture *picture;
	QString filename;
	QImage image;
	int offsetSeconds;
};

class DivePictureModel : public QAbstractTableModel {
	Q_OBJECT
public:
	static DivePictureModel *instance();
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual void updateDivePictures();
	void updateDivePicturesWhenDone(QList<QFuture<void>>);
	void removePictures(const QVector<QString> &fileUrls);
	int rowDDStart, rowDDEnd;
	void updateDivePictureOffset(const QString &filename, int offsetSeconds);
public slots:
	void setZoomLevel(int level);
	void updateThumbnail(QString filename, QImage thumbnail);
private:
	DivePictureModel();
	QList<PictureEntry> pictures;
	int findPictureId(const QString &filename);	// Return -1 if not found
	double zoomLevel;	// -1.0: minimum, 0.0: standard, 1.0: maximum
	int size;
	int defaultSize;
	void updateThumbnails();
	void updateZoom();
};

#endif
