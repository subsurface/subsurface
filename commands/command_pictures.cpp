// SPDX-License-Identifier: GPL-2.0

#include "command_pictures.h"
#include "core/subsurface-qt/divelistnotifier.h"

namespace Command {

static picture *dive_get_picture(const dive *d, const QString &fn)
{
	int idx = get_picture_idx(&d->pictures, qPrintable(fn));
	return idx < 0 ? nullptr : &d->pictures.pictures[idx];
}

SetPictureOffset::SetPictureOffset(dive *dIn, const QString &filenameIn, offset_t offsetIn) :
	d(dIn), filename(filenameIn), offset(offsetIn)
{
	if (!dive_get_picture(d, filename))
		d = nullptr;
	setText(Command::Base::tr("Change media time"));
}

void SetPictureOffset::redo()
{
	picture *pic = dive_get_picture(d, filename);
	if (!pic) {
		fprintf(stderr, "SetPictureOffset::redo(): picture disappeared!");
		return;
	}
	std::swap(pic->offset, offset);
	offset_t newOffset = pic->offset;

	// Instead of trying to be smart, let's simply resort the picture table.
	// If someone complains about speed, do our usual "smart" thing.
	sort_picture_table(&d->pictures);
	emit diveListNotifier.pictureOffsetChanged(d, filename, newOffset);
	invalidate_dive_cache(d);
}

// Undo and redo do the same thing
void SetPictureOffset::undo()
{
	redo();
}

bool SetPictureOffset::workToBeDone()
{
	return !!d;
}

// Filter out pictures that don't exist and sort them according to the backend
static PictureListForDeletion filterPictureListForDeletion(const PictureListForDeletion &p)
{
	PictureListForDeletion res;
	res.d = p.d;
	res.filenames.reserve(p.filenames.size());
	for (int i = 0; i < p.d->pictures.nr; ++i) {
		std::string fn = p.d->pictures.pictures[i].filename;
		if (std::find(p.filenames.begin(), p.filenames.end(), fn) != p.filenames.end())
			res.filenames.push_back(fn);
	}
	return res;
}

// Helper function to remove pictures. Clears the passed-in vector.
static std::vector<PictureListForAddition> removePictures(std::vector<PictureListForDeletion> &picturesToRemove)
{
	std::vector<PictureListForAddition> res;
	for (const PictureListForDeletion &list: picturesToRemove) {
		QVector<QString> filenames;
		PictureListForAddition toAdd;
		toAdd.d = list.d;
		for (const std::string &fn: list.filenames) {
			int idx = get_picture_idx(&list.d->pictures, fn.c_str());
			if (idx < 0) {
				fprintf(stderr, "removePictures(): picture disappeared!");
				continue; // Huh? We made sure that this can't happen by filtering out non-existant pictures.
			}
			filenames.push_back(QString::fromStdString(fn));
			toAdd.pics.emplace_back(list.d->pictures.pictures[idx]);
			remove_from_picture_table(&list.d->pictures, idx);
		}
		if (!toAdd.pics.empty())
			res.push_back(toAdd);
		invalidate_dive_cache(list.d);
		emit diveListNotifier.picturesRemoved(list.d, filenames);
	}
	picturesToRemove.clear();
	return res;
}

// Helper function to add pictures. Clears the passed-in vector.
static std::vector<PictureListForDeletion> addPictures(std::vector<PictureListForAddition> &picturesToAdd)
{
	// We play it extra safe here and again filter out those pictures that
	// are already added to the dive. There should be no way for this to
	// happen, as we checked that before.
	std::vector<PictureListForDeletion> res;
	for (const PictureListForAddition &list: picturesToAdd) {
		QVector<PictureObj> picsForSignal;
		PictureListForDeletion toRemove;
		toRemove.d = list.d;
		for (const PictureObj &pic: list.pics) {
			int idx = get_picture_idx(&list.d->pictures, pic.filename.c_str()); // This should *not* already exist!
			if (idx >= 0) {
				fprintf(stderr, "addPictures(): picture disappeared!");
				continue; // Huh? We made sure that this can't happen by filtering out existing pictures.
			}
			picsForSignal.push_back(pic);
			add_picture(&list.d->pictures, pic.toCore());
			toRemove.filenames.push_back(pic.filename);
		}
		if (!toRemove.filenames.empty())
			res.push_back(toRemove);
		invalidate_dive_cache(list.d);
		emit diveListNotifier.picturesAdded(list.d, picsForSignal);
	}
	picturesToAdd.clear();
	return res;
}

RemovePictures::RemovePictures(const std::vector<PictureListForDeletion> &pictures) : picturesToRemove(pictures)
{
	// Filter out the pictures that don't actually exist. In principle this shouldn't be necessary.
	// Nevertheless, let's play it save. This has the additional benefit of sorting the pictures
	// according to the backend if, for some reason, the caller passed the pictures in in random order.
	picturesToRemove.reserve(pictures.size());
	size_t count = 0;
	for (const PictureListForDeletion &p: pictures) {
		PictureListForDeletion filtered = filterPictureListForDeletion(p);
		if (filtered.filenames.size())
			picturesToRemove.push_back(p);
		count += filtered.filenames.size();
	}
	if (count == 0) {
		picturesToRemove.clear(); // This signals that nothing is to be done
		return;
	}
	setText(Command::Base::tr("remove %n pictures(s)", "", count));
}

bool RemovePictures::workToBeDone()
{
	return !picturesToRemove.empty();
}

void RemovePictures::undo()
{
	picturesToRemove = addPictures(picturesToAdd);
}

void RemovePictures::redo()
{
	picturesToAdd = removePictures(picturesToRemove);
}

} // namespace Command
