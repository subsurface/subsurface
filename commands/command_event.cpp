// SPDX-License-Identifier: GPL-2.0

#include "command_event.h"
#include "core/dive.h"
#include "core/event.h"
#include "core/selection.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/libdivecomputer.h"
#include "core/gettextfromc.h"
#include <QVector>

namespace Command {

EventBase::EventBase(struct dive *dIn, int dcNrIn) :
	d(dIn), dcNr(dcNrIn)
{
}

void EventBase::redo()
{
	redoit(); // Call actual function in base class
	updateDive();
}

void EventBase::undo()
{
	undoit(); // Call actual function in base class
	updateDive();
}

void EventBase::updateDive()
{
	invalidate_dive_cache(d);
	emit diveListNotifier.eventsChanged(d);
	dc_number = dcNr;
	setSelection({ d }, d);
}

AddEventBase::AddEventBase(struct dive *d, int dcNr, struct event *ev) : EventBase(d, dcNr),
	eventToAdd(ev)
{
}

bool AddEventBase::workToBeDone()
{
	return true;
}

void AddEventBase::redoit()
{
	struct divecomputer *dc = get_dive_dc(d, dcNr);
	eventToRemove = eventToAdd.get();
	add_event_to_dc(dc, eventToAdd.release()); // return ownership to backend
}

void AddEventBase::undoit()
{
	struct divecomputer *dc = get_dive_dc(d, dcNr);
	remove_event_from_dc(dc, eventToRemove);
	eventToAdd.reset(eventToRemove); // take ownership of event
	eventToRemove = nullptr;
}

AddEventBookmark::AddEventBookmark(struct dive *d, int dcNr, int seconds) :
	AddEventBase(d, dcNr, create_event(seconds, SAMPLE_EVENT_BOOKMARK, 0, 0, "bookmark"))
{
	setText(Command::Base::tr("Add bookmark"));
}

AddEventDivemodeSwitch::AddEventDivemodeSwitch(struct dive *d, int dcNr, int seconds, int divemode) :
	AddEventBase(d, dcNr, create_event(seconds, SAMPLE_EVENT_BOOKMARK, 0, divemode, QT_TRANSLATE_NOOP("gettextFromC", "modechange")))
{
	setText(Command::Base::tr("Add dive mode switch to %1").arg(gettextFromC::tr(divemode_text_ui[divemode])));
}

AddEventSetpointChange::AddEventSetpointChange(struct dive *d, int dcNr, int seconds, pressure_t pO2) :
	AddEventBase(d, dcNr, create_event(seconds, SAMPLE_EVENT_PO2, 0, pO2.mbar, QT_TRANSLATE_NOOP("gettextFromC", "SP change"))),
	divemode(CCR)
{
	setText(Command::Base::tr("Add set point change")); // TODO: format pO2 value in bar or psi.
}

void AddEventSetpointChange::undoit()
{
	AddEventBase::undoit();
	std::swap(get_dive_dc(d, dcNr)->divemode, divemode);
}

void AddEventSetpointChange::redoit()
{
	AddEventBase::redoit();
	std::swap(get_dive_dc(d, dcNr)->divemode, divemode);
}

RenameEvent::RenameEvent(struct dive *d, int dcNr, struct event *ev, const char *name) : EventBase(d, dcNr),
	eventToAdd(clone_event_rename(ev, name)),
	eventToRemove(ev)
{
	setText(Command::Base::tr("Rename bookmark to %1").arg(name));
}

bool RenameEvent::workToBeDone()
{
	return true;
}

void RenameEvent::redoit()
{
	struct divecomputer *dc = get_dive_dc(d, dcNr);
	swap_event(dc, eventToRemove, eventToAdd.get());
	event *tmp = eventToRemove;
	eventToRemove = eventToAdd.release();
	eventToAdd.reset(tmp);
}

void RenameEvent::undoit()
{
	// Undo and redo do the same thing - they simply swap events
	redoit();
}

RemoveEvent::RemoveEvent(struct dive *d, int dcNr, struct event *ev) : EventBase(d, dcNr),
	eventToRemove(ev),
	cylinder(ev->type == SAMPLE_EVENT_GASCHANGE2 || ev->type == SAMPLE_EVENT_GASCHANGE ?
		 ev->gas.index : -1)
{
	setText(Command::Base::tr("Remove %1 event").arg(ev->name));
}

bool RemoveEvent::workToBeDone()
{
	return true;
}

void RemoveEvent::redoit()
{
	struct divecomputer *dc = get_dive_dc(d, dcNr);
	remove_event_from_dc(dc, eventToRemove);
	eventToAdd.reset(eventToRemove); // take ownership of event
	eventToRemove = nullptr;
}

void RemoveEvent::undoit()
{
	struct divecomputer *dc = get_dive_dc(d, dcNr);
	eventToRemove = eventToAdd.get();
	add_event_to_dc(dc, eventToAdd.release()); // return ownership to backend
}

void RemoveEvent::post() const
{
	if (cylinder < 0)
		return;

	fixup_dive(d);
	emit diveListNotifier.cylinderEdited(d, cylinder);

	// TODO: This is silly we send a DURATION change event so that the statistics are recalculated.
	// We should instead define a proper DiveField that expresses the change caused by a gas switch.
	emit diveListNotifier.divesChanged(QVector<dive *>{ d }, DiveField::DURATION | DiveField::DEPTH);
}

AddGasSwitch::AddGasSwitch(struct dive *d, int dcNr, int seconds, int tank) : EventBase(d, dcNr)
{
	// If there is a gas change at this time stamp, remove it before adding the new one.
	// There shouldn't be more than one gas change per time stamp. Just in case we'll
	// support that anyway.
	struct divecomputer *dc = get_dive_dc(d, dcNr);
	struct event *gasChangeEvent = dc->events;
	while ((gasChangeEvent = get_next_event_mutable(gasChangeEvent, "gaschange")) != NULL) {
		if (gasChangeEvent->time.seconds == seconds) {
			eventsToRemove.push_back(gasChangeEvent);
			int idx = gasChangeEvent->gas.index;
			if (std::find(cylinders.begin(), cylinders.end(), idx) == cylinders.end())
				cylinders.push_back(idx); // cylinders might have changed their status
		}
		gasChangeEvent = gasChangeEvent->next;
	}

	eventsToAdd.emplace_back(create_gas_switch_event(d, dc, seconds, tank));
}

bool AddGasSwitch::workToBeDone()
{
	return true;
}

void AddGasSwitch::redoit()
{
	std::vector<OwningEventPtr> newEventsToAdd;
	std::vector<event *> newEventsToRemove;
	newEventsToAdd.reserve(eventsToRemove.size());
	newEventsToRemove.reserve(eventsToAdd.size());
	struct divecomputer *dc = get_dive_dc(d, dcNr);

	for (event *ev: eventsToRemove) {
		remove_event_from_dc(dc, ev);
		newEventsToAdd.emplace_back(ev); // take ownership of event
	}
	for (OwningEventPtr &ev: eventsToAdd) {
		newEventsToRemove.push_back(ev.get());
		add_event_to_dc(dc, ev.release()); // return ownership to backend
	}
	eventsToAdd = std::move(newEventsToAdd);
	eventsToRemove = std::move(newEventsToRemove);

	// this means we potentially have a new tank that is being used and needs to be shown
	fixup_dive(d);

	for (int idx: cylinders)
		emit diveListNotifier.cylinderEdited(d, idx);

	// TODO: This is silly we send a DURATION change event so that the statistics are recalculated.
	// We should instead define a proper DiveField that expresses the change caused by a gas switch.
	emit diveListNotifier.divesChanged(QVector<dive *>{ d }, DiveField::DURATION | DiveField::DEPTH);
}

void AddGasSwitch::undoit()
{
	// Undo and redo do the same thing, as the dives-to-be-added and dives-to-be-removed are exchanged.
	redoit();
}

} // namespace Command
