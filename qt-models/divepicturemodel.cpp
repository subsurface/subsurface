// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divepicturemodel.h"
#include "core/dive.h"
#include "core/metrics.h"
#include "core/divelist.h"
#include "core/imagedownloader.h"

#include <QtConcurrent>

extern QHash <QString, QImage> thumbnailCache;
extern QHash <QString, QImage> videoThumbnailCache;
static QMutex thumbnailMutex;
static const int maxZoom = 3;	// Maximum zoom: thrice of standard size

static QImage getThumbnailFromCache(PictureEntry &entry, int maxSize)
{
	QMutexLocker l(&thumbnailMutex);

	// Currently we only save a null picture in the videoThumbnailCache
	// as a marker that this was identified as a video. In the future,
	// use the videoThumbnail cache to save an actual still image.
	entry.isVideo = videoThumbnailCache.contains(entry.filename);
	if (entry.isVideo)
		return QImage(":video-icon").scaled(maxSize, maxSize, Qt::KeepAspectRatio);

	return thumbnailCache.value(entry.filename);
}

static void scaleImages(PictureEntry &entry, int size, int maxSize)
{
	QImage thumbnail = getThumbnailFromCache(entry, maxSize);
	// If thumbnails were written by an earlier version, they might be smaller than needed.
	// Rescale in such a case to avoid resizing artifacts.
	if (thumbnail.isNull() || (thumbnail.size().width() < maxSize && thumbnail.size().height() < maxSize)) {
		qDebug() << "No thumbnail in cache for" << entry.filename;
		auto res = getHashedImage(entry.picture);
		thumbnail = res.first.scaled(maxSize, maxSize, Qt::KeepAspectRatio);
		entry.isVideo = res.second;
		QMutexLocker l(&thumbnailMutex);
		if (entry.isVideo)
			videoThumbnailCache.insert(entry.filename, thumbnail);
		else
			thumbnailCache.insert(entry.filename, thumbnail);
	}

	entry.imageProfile = thumbnail.scaled(maxSize / maxZoom, maxSize / maxZoom, Qt::KeepAspectRatio);
	entry.image = size == maxSize ? thumbnail
				      : thumbnail.scaled(size, size, Qt::KeepAspectRatio);
}

DivePictureModel *DivePictureModel::instance()
{
	static DivePictureModel *self = new DivePictureModel();
	return self;
}

DivePictureModel::DivePictureModel() : rowDDStart(0),
				       rowDDEnd(0),
				       zoomLevel(0.0)
{
}

void DivePictureModel::updateDivePicturesWhenDone(QList<QFuture<void>> futures)
{
	Q_FOREACH (QFuture<void> f, futures) {
		f.waitForFinished();
	}
	updateDivePictures();
}

void DivePictureModel::setZoomLevel(int level)
{
	zoomLevel = level / 10.0;
	// zoomLevel is bound by [-1.0 1.0], see comment below.
	if (zoomLevel < -1.0)
		zoomLevel = -1.0;
	if (zoomLevel > 1.0)
		zoomLevel = 1.0;
	updateThumbnails();
	layoutChanged();
}

void DivePictureModel::updateThumbnails()
{
	// Calculate size of thumbnails. The standard size is defaultIconMetrics().sz_pic.
	// We use exponential scaling so that the central point is the standard
	// size and the minimum and maximum extreme points are a third respectively
	// three times the standard size.
	// Naturally, these three zoom levels are then represented by
	// -1.0 (minimum), 0 (standard) and 1.0 (maximum). The actual size is
	// calculated as standard_size*3.0^zoomLevel.
	int defaultSize = defaultIconMetrics().sz_pic;
	int maxSize = defaultSize * maxZoom;
	int size = static_cast<int>(round(defaultSize * pow(maxZoom, zoomLevel)));
	QtConcurrent::blockingMap(pictures, [size, maxSize](PictureEntry &entry){scaleImages(entry, size, maxSize);});
}

void DivePictureModel::updateDivePictures()
{
	if (!pictures.isEmpty()) {
		beginRemoveRows(QModelIndex(), 0, pictures.count() - 1);
		pictures.clear();
		endRemoveRows();
		rowDDStart = rowDDEnd = 0;
	}

	// if the dive_table is empty, quit
	if (dive_table.nr == 0)
		return;

	int i;
	struct dive *dive;
	for_each_dive (i, dive) {
		if (dive->selected) {
			if (dive->id == displayed_dive.id)
				rowDDStart = pictures.count();
			FOR_EACH_PICTURE(dive)
				pictures.push_back({picture, picture->filename, {}, {}, picture->offset.seconds, false});
			if (dive->id == displayed_dive.id)
				rowDDEnd = pictures.count();
		}
	}

	updateThumbnails();

	if (!pictures.isEmpty()) {
		beginInsertRows(QModelIndex(), 0, pictures.count() - 1);
		endInsertRows();
	}
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
		case Qt::UserRole:	// Used by profile widget to access bigger thumbnails
			ret = entry.imageProfile;
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
	int i;
	struct dive *dive;
	for_each_dive (i, dive) {
		if (dive->selected && dive_remove_picture(dive, qPrintable(fileUrl)))
			break;
	}
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
