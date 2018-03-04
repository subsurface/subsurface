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
	QImage imageProfile;	// For the profile widget keep a copy of a constant sized image
	int offsetSeconds;
	bool isVideo;
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
	void removePicture(const QString& fileUrl, bool last);
	int rowDDStart, rowDDEnd;
public slots:
	void setZoomLevel(int level);
private:
	DivePictureModel();
	QList<PictureEntry> pictures;
	double zoomLevel;	// -1.0: minimum, 0.0: standard, 1.0: maximum
	void updateThumbnails();
};

#endif
