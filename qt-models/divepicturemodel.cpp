// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divepicturemodel.h"
#include "core/dive.h"
#include "core/metrics.h"
#include "core/divelist.h"
#include "core/imagedownloader.h"
#include "core/qthelper.h"

#include <QFileInfo>

DivePictureModel *DivePictureModel::instance()
{
	static DivePictureModel *self = new DivePictureModel();
	return self;
}

DivePictureModel::DivePictureModel() : zoomLevel(0.0)
{
	connect(Thumbnailer::instance(), &Thumbnailer::thumbnailChanged,
		this, &DivePictureModel::updateThumbnail, Qt::QueuedConnection);
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
	updateZoom();
	layoutChanged();
}

void DivePictureModel::updateZoom()
{
	size = Thumbnailer::thumbnailSize(zoomLevel);
}

void DivePictureModel::updateThumbnails()
{
	updateZoom();
	for (PictureEntry &entry: pictures)
		entry.image = Thumbnailer::instance()->fetchThumbnail(entry.filename);
}

void DivePictureModel::updateDivePictures()
{
	beginResetModel();
	if (!pictures.isEmpty()) {
		pictures.clear();
		Thumbnailer::instance()->clearWorkQueue();
	}

	// if the dive_table is empty, quit
	if (dive_table.nr == 0)
		return;

	int i;
	struct dive *dive;
	for_each_dive (i, dive) {
		if (dive->selected) {
			FOR_EACH_PICTURE(dive)
				pictures.push_back({picture, picture->filename, {}, picture->offset.seconds});
		}
	}

	updateThumbnails();
	endResetModel();
}

int DivePictureModel::columnCount(const QModelIndex&) const
{
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
			ret = entry.image.scaled(size, size, Qt::KeepAspectRatio);
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

// Return true if we actually removed a picture
static bool removePictureFromSelectedDive(const char *fileUrl)
{
	int i;
	struct dive *dive;
	for_each_dive (i, dive) {
		if (dive->selected && dive_remove_picture(dive, fileUrl))
			return true;
	}
	return false;
}

void DivePictureModel::removePictures(const QVector<QString> &fileUrls)
{
	bool removed = false;
	for (const QString &fileUrl: fileUrls)
		removed |= removePictureFromSelectedDive(qPrintable(fileUrl));
	if (!removed)
		return;
	copy_dive(current_dive, &displayed_dive);
	mark_divelist_changed(true);

	for (int i = 0; i < pictures.size(); ++i) {
		// Find range [i j) of pictures to remove
		if (std::find(fileUrls.begin(), fileUrls.end(), pictures[i].filename) == fileUrls.end())
			continue;
		int j;
		for (j = i + 1; j < pictures.size(); ++j) {
			if (std::find(fileUrls.begin(), fileUrls.end(), pictures[j].filename) == fileUrls.end())
				break;
		}

		// Qt's model-interface is surprisingly idiosyncratic: you don't pass [first last), but [first last] ranges.
		// For example, an empty list would be [0 -1].
		beginRemoveRows(QModelIndex(), i, j - 1);
		pictures.erase(pictures.begin() + i, pictures.begin() + j);
		endRemoveRows();
	}
	emit picturesRemoved(fileUrls);
}

int DivePictureModel::rowCount(const QModelIndex&) const
{
	return pictures.count();
}

int DivePictureModel::findPictureId(const QString &filename)
{
	for (int i = 0; i < pictures.size(); ++i)
		if (pictures[i].filename == filename)
			return i;
	return -1;
}

void DivePictureModel::updateThumbnail(QString filename, QImage thumbnail)
{
	int i = findPictureId(filename);
	if (i >= 0) {
		pictures[i].image = thumbnail;
		emit dataChanged(createIndex(i, 0), createIndex(i, 1));
	}
}

void DivePictureModel::updateDivePictureOffset(const QString &filename, int offsetSeconds)
{
	int i = findPictureId(filename);
	if (i >= 0) {
		pictures[i].offsetSeconds = offsetSeconds;
		emit dataChanged(createIndex(i, 0), createIndex(i, 1));
	}
}
