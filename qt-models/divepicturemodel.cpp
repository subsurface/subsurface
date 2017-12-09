// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divepicturemodel.h"
#include "core/dive.h"
#include "core/metrics.h"
#include "core/divelist.h"
#include "core/imagedownloader.h"

#include <QtConcurrent>

extern QHash <QString, QImage> thumbnailCache;

void scaleImages(PictureEntry &entry)
{
	if (thumbnailCache.contains(entry.filename) && !thumbnailCache.value(entry.filename).isNull()) {
		entry.image = thumbnailCache.value(entry.filename);
	} else {
		int dim = defaultIconMetrics().sz_pic;
		QImage p = SHashedImage(entry.picture);
		if(!p.isNull()) {
			p = p.scaled(dim, dim, Qt::KeepAspectRatio);
			thumbnailCache.insert(entry.filename, p);
		}
		entry.image = p;
	}
}

DivePictureModel *DivePictureModel::instance()
{
	static DivePictureModel *self = new DivePictureModel();
	return self;
}

DivePictureModel::DivePictureModel()
{
}

void DivePictureModel::updateDivePicturesWhenDone(QList<QFuture<void>> futures)
{
	Q_FOREACH (QFuture<void> f, futures) {
		f.waitForFinished();
	}
	updateDivePictures();
}

void DivePictureModel::updateDivePictures()
{
	if (!pictures.isEmpty()) {
		beginRemoveRows(QModelIndex(), 0, pictures.count() - 1);
		pictures.clear();
		endRemoveRows();
	}

	// if the dive_table is empty, ignore the displayed_dive
	if (dive_table.nr == 0 || dive_get_picture_count(&displayed_dive) == 0)
		return;

	FOR_EACH_PICTURE_NON_PTR(displayed_dive)
		pictures.push_back({picture, picture->filename, QImage(), picture->offset.seconds});

	QtConcurrent::blockingMap(pictures, scaleImages);

	beginInsertRows(QModelIndex(), 0, pictures.count() - 1);
	endInsertRows();
}

int DivePictureModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return 2;
}

QVariant DivePictureModel::data(const QModelIndex &index, int role) const
{
	QVariant ret;
	if (!index.isValid())
		return ret;

	const PictureEntry &entry = pictures.at(index.row());
	if (index.column() == 0) {
		switch (role) {
		case Qt::ToolTipRole:
			ret = entry.filename;
			break;
		case Qt::DecorationRole:
			ret = entry.image;
			break;
		case Qt::DisplayRole:
			ret = QFileInfo(entry.filename).fileName();
			break;
		case Qt::DisplayPropertyRole:
			ret = QFileInfo(entry.filename).filePath();
		}
	} else if (index.column() == 1) {
		switch (role) {
		case Qt::UserRole:
			ret = QVariant::fromValue(entry.offsetSeconds);
			break;
		case Qt::DisplayRole:
			ret = entry.filename;
		}
	}
	return ret;
}

void DivePictureModel::removePicture(const QString &fileUrl, bool last)
{
	dive_remove_picture(fileUrl.toUtf8().data());
	if (last) {
		copy_dive(current_dive, &displayed_dive);
		updateDivePictures();
		mark_divelist_changed(true);
	}
}

int DivePictureModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return pictures.count();
}
