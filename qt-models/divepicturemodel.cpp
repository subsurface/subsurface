// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divepicturemodel.h"
#include "core/divelist.h" // for comp_dives
#include "core/metrics.h"
#include "core/imagedownloader.h"
#include "core/picture.h"
#include "core/qthelper.h"
#include "core/range.h"
#include "core/selection.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "commands/command.h"

#include <QFileInfo>
#include <QPainter>

PictureEntry::PictureEntry(dive *dIn, const PictureObj &p) : d(dIn),
	filename(p.filename),
	offsetSeconds(p.offset.seconds),
	length({ 0 })
{
}

PictureEntry::PictureEntry(dive *dIn, const picture &p) : d(dIn),
	filename(p.filename),
	offsetSeconds(p.offset.seconds),
	length({ 0 })
{
}

// Note: it is crucial that this uses the same sorting as the core.
// Therefore, we use the C strcmp functions [std::string::operator<()
// should give the same result].
bool PictureEntry::operator<(const PictureEntry &p2) const
{
	if (int cmp = comp_dives(d, p2.d))
		return cmp < 0;
	if (offsetSeconds != p2.offsetSeconds)
		return offsetSeconds < p2.offsetSeconds;
	return strcmp(filename.c_str(), p2.filename.c_str()) < 0;
}

DivePictureModel *DivePictureModel::instance()
{
	static DivePictureModel *self = new DivePictureModel();
	return self;
}

DivePictureModel::DivePictureModel() : zoomLevel(0.0)
{
	connect(Thumbnailer::instance(), &Thumbnailer::thumbnailChanged,
		this, &DivePictureModel::updateThumbnail, Qt::QueuedConnection);
	connect(&diveListNotifier, &DiveListNotifier::pictureOffsetChanged,
		this, &DivePictureModel::pictureOffsetChanged);
	connect(&diveListNotifier, &DiveListNotifier::picturesRemoved,
		this, &DivePictureModel::picturesRemoved);
	connect(&diveListNotifier, &DiveListNotifier::picturesAdded,
		this, &DivePictureModel::picturesAdded);
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
		entry.image = Thumbnailer::instance()->fetchThumbnail(QString::fromStdString(entry.filename), false);
}

void DivePictureModel::updateDivePictures()
{
	beginResetModel();
	if (!pictures.empty()) {
		pictures.clear();
		Thumbnailer::instance()->clearWorkQueue();
	}

	for (struct dive *dive: getDiveSelection()) {
		size_t first = pictures.size();
		FOR_EACH_PICTURE(dive)
			pictures.push_back(PictureEntry(dive, *picture));

		// Sort pictures of this dive by offset.
		// Thus, the list will be sorted by (dive, offset).
		std::sort(pictures.begin() + first, pictures.end(),
			  [](const PictureEntry &a, const PictureEntry &b) { return a.offsetSeconds < b.offsetSeconds; });
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
	if (!index.isValid())
		return QVariant();

	const PictureEntry &entry = pictures.at(index.row());
	if (index.column() == 0) {
		switch (role) {
		case Qt::ToolTipRole:
			return QString::fromStdString(entry.filename);
		case Qt::DecorationRole:
			return entry.image.scaled(size, size, Qt::KeepAspectRatio);
		case Qt::DisplayRole:
			return QFileInfo(QString::fromStdString(entry.filename)).fileName();
		case Qt::DisplayPropertyRole:
			return QFileInfo(QString::fromStdString(entry.filename)).filePath();
		case Qt::UserRole + 1:
			return entry.offsetSeconds;
		case Qt::UserRole + 2:
			return entry.length.seconds;
		}
	} else if (index.column() == 1) {
		switch (role) {
		case Qt::DisplayRole:
			return QString::fromStdString(entry.filename);
		}
	}
	return QVariant();
}

void DivePictureModel::removePictures(const QModelIndexList &indices)
{
	// Collect pictures to remove by dive
	std::vector<Command::PictureListForDeletion> pics;
	for (const QModelIndex &idx: indices) {
		if (!idx.isValid())
			continue;
		const PictureEntry &item = pictures[idx.row()];
		// Check if we already have pictures for that dive.
		auto it = find_if(pics.begin(), pics.end(),
				  [&item](const Command::PictureListForDeletion &list)
				  { return list.d == item.d; });
		// If not found, add a new list
		if (it == pics.end())
			pics.push_back({ item.d, { item.filename }});
		else
			it->filenames.push_back(item.filename);
	}
	Command::removePictures(pics);
}

void DivePictureModel::picturesRemoved(dive *d, QVector<QString> filenamesIn)
{
	// Transform vector of QStrings into vector of std::strings
	std::vector<std::string> filenames;
	filenames.reserve(filenamesIn.size());
	std::transform(filenamesIn.begin(), filenamesIn.end(), std::back_inserter(filenames),
		       [] (const QString &s) { return s.toStdString(); });

	// Get range of pictures of the given dive.
	// Note: we could be more efficient by either using a binary search or a two-level data structure.
	auto from = std::find_if(pictures.begin(), pictures.end(), [d](const PictureEntry &e) { return e.d == d; });
	auto to = std::find_if(from, pictures.end(), [d](const PictureEntry &e) { return e.d != d; });
	if (from == pictures.end())
		return;

	size_t fromIdx = from - pictures.begin();
	size_t toIdx = to - pictures.begin();
	for (size_t i = fromIdx; i < toIdx; ++i) {
		// Find range [i j) of pictures to remove
		if (std::find(filenames.begin(), filenames.end(), pictures[i].filename) == filenames.end())
			continue;
		size_t j;
		for (j = i + 1; j < toIdx; ++j) {
			if (std::find(filenames.begin(), filenames.end(), pictures[j].filename) == filenames.end())
				break;
		}

		// Qt's model-interface is surprisingly idiosyncratic: you don't pass [first last), but [first last] ranges.
		// For example, an empty list would be [0 -1].
		beginRemoveRows(QModelIndex(), i, j - 1);
		pictures.erase(pictures.begin() + i, pictures.begin() + j);
		endRemoveRows();
		toIdx -= j - i;
	}
}

// Assumes that pics is sorted!
void DivePictureModel::picturesAdded(dive *d, QVector<PictureObj> picsIn)
{
	// We only display pictures of selected dives
	if (!d->selected || picsIn.empty())
		return;

	// Convert the picture-data into our own format
	std::vector<PictureEntry> pics;
	pics.reserve(picsIn.size());
	for (const PictureObj &pic: picsIn)
		pics.push_back(PictureEntry(d, pic));

	// Insert batch-wise to avoid too many reloads
	pictures.reserve(pictures.size() + pics.size());
	auto from = pics.begin();
	int dest = 0;
	while (from != pics.end()) {
		// Search for the insertion index. This supposes a lexicographical sort for the [dive, offset, filename] triple.
		// TODO: currently this works, because all undo commands that manipulate the dive list also reset the selection
		// and thus the model is rebuilt. However, we might catch the respective signals here and not rely on being
		// called by the tab-widgets.
		auto dest_it = std::lower_bound(pictures.begin() + dest, pictures.end(), *from);
		int dest = dest_it - pictures.begin();
		auto to = dest_it == pictures.end() ? pics.end() : from + 1; // If at the end - just add the rest
		while (to != pics.end() && *to < *dest_it)
			++to;
		int batch_size = to - from;
		beginInsertRows(QModelIndex(), dest, dest + batch_size - 1);
		pictures.insert(pictures.begin() + dest, from, to);
		// Get thumbnails of inserted pictures
		for (auto it = pictures.begin() + dest; it < pictures.begin() + dest + batch_size; ++it)
			it->image = Thumbnailer::instance()->fetchThumbnail(QString::fromStdString(it->filename), false);
		endInsertRows();
		from = to;
		dest += batch_size;
	}
}

int DivePictureModel::rowCount(const QModelIndex&) const
{
	return (int)pictures.size();
}

int DivePictureModel::findPictureId(const std::string &filename)
{
	return index_of_if(pictures, [&filename](const PictureEntry &p)
				     { return p.filename == filename; });
}

static void addDurationToThumbnail(QImage &img, duration_t duration)
{
	int seconds = duration.seconds;
	if (seconds < 0)
		return;
	QString s = seconds >= 3600 ?
		QStringLiteral("%1:%2:%3").arg(seconds / 3600, 2, 10, QChar('0'))
					  .arg((seconds % 3600) / 60, 2, 10, QChar('0'))
					  .arg(seconds % 60, 2, 10, QChar('0')) :
		QStringLiteral("%1:%2").arg(seconds / 60, 2, 10, QChar('0'))
				       .arg(seconds % 60, 2, 10, QChar('0'));

	QFont font(system_divelist_default_font, 30);
	QFontMetrics metrics(font);
	QSize size = metrics.size(Qt::TextSingleLine, s);
	QSize imgSize = img.size();
	int x = imgSize.width() - size.width();
	int y = imgSize.height() - size.height() + metrics.descent();
	QPainter painter(&img);
	painter.setBrush(Qt::white);
	painter.setPen(Qt::NoPen);
	painter.drawRect(x, y, size.width(), size.height() - metrics.descent());
	painter.setFont(font);
	painter.setPen(Qt::black);
	painter.drawText(x, imgSize.height(), s);
}

void DivePictureModel::updateThumbnail(QString filename, QImage thumbnail, duration_t duration)
{
	int i = findPictureId(filename.toStdString());
	if (i >= 0) {
		if (duration.seconds > 0) {
			addDurationToThumbnail(thumbnail, duration);	// If we know the duration paint it on top of the thumbnail
			pictures[i].length = duration;
		}
		pictures[i].image = std::move(thumbnail);
		emit dataChanged(createIndex(i, 0), createIndex(i, 1));
	}
}

void DivePictureModel::pictureOffsetChanged(dive *d, const QString filenameIn, offset_t offset)
{
	std::string filename = filenameIn.toStdString();

	// Find the pictures of the given dive.
	auto from = std::find_if(pictures.begin(), pictures.end(), [d](const PictureEntry &e) { return e.d == d; });
	auto to = std::find_if(from, pictures.end(), [d](const PictureEntry &e) { return e.d != d; });

	// Find picture with the given filename
	auto oldPos = std::find_if(from, to, [&filename](const PictureEntry &e) { return e.filename == filename; });
	if (oldPos == to)
		return;

	// Find new position
	auto newPos = std::find_if(from, to, [offset](const PictureEntry &e) { return e.offsetSeconds > offset.seconds; });

	// Update the offset here and in the backend
	oldPos->offsetSeconds = offset.seconds;

	// Henceforth we will work with indices instead of iterators
	int oldIndex = oldPos - pictures.begin();
	int newIndex = newPos - pictures.begin();
	if (oldIndex == newIndex || oldIndex + 1 == newIndex)
		return;
	beginMoveRows(QModelIndex(), oldIndex, oldIndex, QModelIndex(), newIndex);
	move_in_range(pictures, oldIndex, oldIndex + 1, newIndex);
	endMoveRows();
}
