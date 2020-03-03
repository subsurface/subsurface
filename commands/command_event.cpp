// SPDX-License-Identifier: GPL-2.0

#include "command_event.h"
#include "core/dive.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/libdivecomputer.h"
#include "core/gettextfromc.h"

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
	emit diveListNotifier.eventsChanged(d);
}

void AddEventBase::undo()
{
	struct divecomputer *dc = get_dive_dc(d, dcNr);
	remove_event_from_dc(dc, eventToRemove);
	eventToAdd.reset(eventToRemove); // take ownership of event
	eventToRemove = nullptr;
	invalidate_dive_cache(d);
	emit diveListNotifier.eventsChanged(d);
}

AddEventBookmark::AddEventBookmark(struct dive *d, int dcNr, int seconds) :
	AddEventBase(d, dcNr, create_event(seconds, SAMPLE_EVENT_BOOKMARK, 0, 0, "bookmark"))
{
	setText(tr("Add bookmark"));
}

AddEventDivemodeSwitch::AddEventDivemodeSwitch(struct dive *d, int dcNr, int seconds, int divemode) :
	AddEventBase(d, dcNr, create_event(seconds, SAMPLE_EVENT_BOOKMARK, 0, divemode, QT_TRANSLATE_NOOP("gettextFromC", "modechange")))
{
	setText(tr("Add dive mode switch to %1").arg(gettextFromC::tr(divemode_text_ui[divemode])));
}

} // namespace Command
