// SPDX-License-Identifier: GPL-2.0

#include "command_event.h"
#include "core/dive.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/libdivecomputer.h"

namespace Command {

AddEventBase::AddEventBase(struct dive *dIn, int dcNrIn, struct event *ev) :
	d(dIn), dcNr(dcNrIn), eventToAdd(ev)
{
}

bool AddEventBase::workToBeDone()
{
	return true;
}

void AddEventBase::redo()
{
	struct divecomputer *dc = get_dive_dc(d, dcNr);
	eventToRemove = eventToAdd.get();
	add_event_to_dc(dc, eventToAdd.release()); // return ownership to backend
	invalidate_dive_cache(d);
}

void AddEventBase::undo()
{
	struct divecomputer *dc = get_dive_dc(d, dcNr);
	remove_event_from_dc(dc, eventToRemove);
	eventToAdd.reset(eventToRemove); // take ownership of event
	eventToRemove = nullptr;
	invalidate_dive_cache(d);
}

AddEventBookmark::AddEventBookmark(struct dive *d, int dcNr, int seconds) :
	AddEventBase(d, dcNr, create_event(seconds, SAMPLE_EVENT_BOOKMARK, 0, 0, "bookmark"))
{
	setText(tr("Add bookmark"));
}

} // namespace Command
