// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEPICTUREMODEL_H
#define DIVEPICTUREMODEL_H

#include "core/picture.h"

#include <QAbstractTableModel>
#include <QImage>

// We use std::string instead of QString to use the same character-encoding
// as in the C core (UTF-8). This is crucial to guarantee the same sort-order.
struct dive;
struct PictureEntry {
	dive *d;
	std::string filename;
	QImage image;
	int offsetSeconds;
	duration_t length;
	PictureEntry(dive *, const picture &);
	bool operator<(const PictureEntry &) const;
};

class DivePictureModel : public QAbstractTableModel {
	Q_OBJECT
public:
	static DivePictureModel *instance();
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	void updateDivePictures();
	void removePictures(const QModelIndexList &);
public slots:
	void setZoomLevel(int level);
	void updateThumbnail(QString filename, QImage thumbnail, duration_t duration);
	void pictureOffsetChanged(dive *d, const QString filename, offset_t offset);
	void picturesRemoved(dive *d, QVector<QString> filenames);
	void picturesAdded(dive *d, QVector<picture> pics);
private:
	DivePictureModel();
	std::vector<PictureEntry> pictures;
	int findPictureId(const std::string &filename);	// Return -1 if not found
	double zoomLevel;	// -1.0: minimum, 0.0: standard, 1.0: maximum
	int size;
	void updateThumbnails();
	void updateZoom();
};

#endif
