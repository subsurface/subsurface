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

DivePictureModel::DivePictureModel() : rowDDStart(0),
				       rowDDEnd(0),
				       zoomLevel(0.0),
				       defaultSize(Thumbnailer::defaultThumbnailSize())
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
		entry.image = Thumbnailer::instance()->fetchThumbnail(entry);
}

void DivePictureModel::updateDivePictures()
{
	beginResetModel();
	if (!pictures.isEmpty()) {
		pictures.clear();
		rowDDStart = rowDDEnd = 0;
		Thumbnailer::instance()->clearWorkQueue();
	}

	// if the dive_table is empty, quit
	if (dive_table.nr == 0)
		return;

	int i;
	struct dive *dive;
	for_each_dive (i, dive) {
		if (!dive->selected)
			continue;
		int first = pictures.count();
		FOR_EACH_PICTURE(dive)
			pictures.push_back({ dive->id, picture, picture->filename, {}, picture->offset.seconds });
		int last = pictures.count();

		// Sort pictures of this dive by offset.
		// Thus, the list will be sorted by (diveId, offset)
		std::sort(pictures.begin() + first, pictures.begin() + last,
			  [](const PictureEntry &a, const PictureEntry &b) { return a.offsetSeconds < b.offsetSeconds; });

		if (dive->id == displayed_dive.id) {
			rowDDStart = first;
			rowDDEnd = last;
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
		case Qt::UserRole:	// Used by profile widget to access bigger thumbnails
			ret = entry.image.scaled(defaultSize, defaultSize, Qt::KeepAspectRatio);
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
	// Find picture with given filename, but only if it belongs to the currently displayed dive.
	// This accounts for the facts that
	//	1) The same picture may be associated to different dives.
	//	2) It doesn't make sense to place a picture of a different dive to the shown profile.
	// If such a picture doesn't exist -> return early.
	int diveId = displayed_dive.id;
	auto it = std::find_if(pictures.begin(), pictures.end(),
			       [&filename, diveId](const PictureEntry &e)
			       { return e.filename == filename && e.diveId == diveId; });
	if (it == pictures.end())
		return;

	// Find range of pictures belonging to this picture's dive
	auto from = it;
	auto to = it;
	while (from != pictures.begin() && std::prev(from)->diveId == diveId)
		--from;
	while (to != pictures.end() && to->diveId == diveId)
		++to;

	// Find new position in range
	auto newPos = from;
	for ( ; newPos != to; ++newPos)
		if (newPos->offsetSeconds > offsetSeconds)
			break;

	// Update the offset here and in the C-part
	it->offsetSeconds = offsetSeconds;
	FOR_EACH_PICTURE(current_dive) {
		if (picture->filename == filename) {
			picture->offset.seconds = offsetSeconds;
			mark_divelist_changed(true);
			break;
		}
	}
	copy_dive(current_dive, &displayed_dive);

	// Henceforth we will work with indexes, thus turn the interators into indexes.
	int oldIndex = it - pictures.begin();
	int newIndex = newPos - pictures.begin();

	// Qt disallows moving rows to the same place. This is unfortunate, because this would
	// allow us to adjust the coordinates of the pictures in ProfileWidget2's movedPictures() slot.
	// Because of this, we have to guard Qt's functions by ifs and add a custom signal/slot pair
	// for adjusting coordinates.
	bool actualMove = oldIndex != newIndex && oldIndex + 1 != newIndex;
	if (actualMove)
		beginMoveRows(QModelIndex(), oldIndex, oldIndex, QModelIndex(), newIndex);
	int newRangeIndex = moveInVector(pictures, oldIndex, oldIndex + 1, newIndex);
	if (actualMove)
		endMoveRows();

	// Instruct the profile plot to recalculate the coordinates of the images.
	emit offsetChanged(newRangeIndex);
}
