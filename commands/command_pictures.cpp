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

} // namespace Command
