// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divepicturemodel.h"
#include "core/dive.h"
#include "core/metrics.h"
#include "core/divelist.h"
#include "core/imagedownloader.h"
#include "core/qthelper.h"

#include <QFileInfo>

// To be able to pass the mediatype_t enum through Qt's signal/slot system,
// we have to first declare it as a metatype using a macro and then call
// qRegisterMetaType(). We must do this before connecting the first signal
// with tis signature. Not a friendly interface!
Q_DECLARE_METATYPE(mediatype_t)

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
	qRegisterMetaType<mediatype_t>();

	connect(Thumbnailer::instance(), &Thumbnailer::thumbnailChanged,
		this, &DivePictureModel::updateThumbnail, Qt::QueuedConnection);
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

	int i;
	struct dive *dive;
	for_each_dive (i, dive) {
		if (dive->selected) {
			if (dive->id == displayed_dive.id)
				rowDDStart = pictures.count();
			FOR_EACH_PICTURE(dive)
				pictures.push_back({picture, picture->filename, {}, picture->offset.seconds, MEDIATYPE_UNKNOWN});
			if (dive->id == displayed_dive.id)
				rowDDEnd = pictures.count();
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
		case Qt::UserRole + 1:
			ret = (quint32)entry.type;
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

// Calculate how many items of a range are before the given index
static int rangeBefore(int rangeFrom, int rangeTo, int index)
{
	if (rangeTo <= rangeFrom)
		return 0;
	return std::min(rangeTo, index) - std::min(rangeFrom, index);
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

		// After removing pictures, we have to adjust rowDDStart and rowDDEnd.
		// Calculate the part of the range that is before rowDDStart and rowDDEnd,
		// respectively and subtract accordingly.
		rowDDStart -= rangeBefore(i, j, rowDDStart);
		rowDDEnd -= rangeBefore(i, j, rowDDEnd);
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

void DivePictureModel::updateThumbnail(QString filename, QImage thumbnail, mediatype_t type)
{
	int i = findPictureId(filename);
	if (i >= 0) {
		pictures[i].type = type;
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
