// SPDX-License-Identifier: GPL-2.0

#include "command_event.h"
#include "core/dive.h"
#include "core/divelist.h"
#include "core/divelog.h"
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
	d->invalidate_cache();
	emit diveListNotifier.eventsChanged(d);
	setSelection({ d }, d, dcNr);
}

AddEventBase::AddEventBase(struct dive *d, int dcNr, struct event ev) : EventBase(d, dcNr),
	ev(std::move(ev))
{
}

bool AddEventBase::workToBeDone()
{
	return true;
}

void AddEventBase::redoit()
{
	struct divecomputer *dc = d->get_dc(dcNr);
	idx = add_event_to_dc(dc, ev); // return ownership to backend
}

void AddEventBase::undoit()
{
	struct divecomputer *dc = d->get_dc(dcNr);
	ev = remove_event_from_dc(dc, idx);
}

AddEventBookmark::AddEventBookmark(struct dive *d, int dcNr, int seconds) :
	AddEventBase(d, dcNr, event(seconds, SAMPLE_EVENT_BOOKMARK, 0, 0, "bookmark"))
{
	setText(Command::Base::tr("Add bookmark"));
}

AddEventDivemodeSwitch::AddEventDivemodeSwitch(struct dive *d, int dcNr, int seconds, int divemode) :
	AddEventBase(d, dcNr, event(seconds, SAMPLE_EVENT_BOOKMARK, 0, divemode, QT_TRANSLATE_NOOP("gettextFromC", "modechange")))
{
	setText(Command::Base::tr("Add dive mode switch to %1").arg(gettextFromC::tr(divemode_text_ui[divemode])));
}

AddEventSetpointChange::AddEventSetpointChange(struct dive *d, int dcNr, int seconds, pressure_t pO2) :
	AddEventBase(d, dcNr, event(seconds, SAMPLE_EVENT_PO2, 0, pO2.mbar, QT_TRANSLATE_NOOP("gettextFromC", "SP change"))),
	divemode(CCR)
{
	setText(Command::Base::tr("Add set point change")); // TODO: format pO2 value in bar or psi.
}

void AddEventSetpointChange::undoit()
{
	AddEventBase::undoit();
	std::swap(d->get_dc(dcNr)->divemode, divemode);
}

void AddEventSetpointChange::redoit()
{
	AddEventBase::redoit();
	std::swap(d->get_dc(dcNr)->divemode, divemode);
}

RenameEvent::RenameEvent(struct dive *d, int dcNr, int idx, const std::string nameIn) : EventBase(d, dcNr),
	idx(idx),
	name(std::move(nameIn))
{
	setText(Command::Base::tr("Rename bookmark to %1").arg(name.c_str()));
}

bool RenameEvent::workToBeDone()
{
	return true;
}

void RenameEvent::redoit()
{
	struct divecomputer *dc = d->get_dc(dcNr);
	event *ev = get_event(dc, idx);
	if (ev)
		std::swap(ev->name, name);
}

void RenameEvent::undoit()
{
	// Undo and redo do the same thing - they simply swap names
	redoit();
}

RemoveEvent::RemoveEvent(struct dive *d, int dcNr, int idxIn) : EventBase(d, dcNr),
	idx(idxIn), cylinder(-1)
{
	struct divecomputer *dc = d->get_dc(dcNr);
	event *ev = get_event(dc, idx);
	if (!ev) {
		idx = -1;
		return;
	}
	if (ev->type == SAMPLE_EVENT_GASCHANGE2 || ev->type == SAMPLE_EVENT_GASCHANGE)
		cylinder = ev->gas.index;
	setText(Command::Base::tr("Remove %1 event").arg(ev->name.c_str()));
}

bool RemoveEvent::workToBeDone()
{
	return idx >= 0;
}

void RemoveEvent::redoit()
{
	struct divecomputer *dc = d->get_dc(dcNr);
	ev = remove_event_from_dc(dc, idx);
}

void RemoveEvent::undoit()
{
	struct divecomputer *dc = d->get_dc(dcNr);
	idx = add_event_to_dc(dc, std::move(ev));
}

void RemoveEvent::post() const
{
	if (cylinder < 0)
		return;

	divelog.dives.fixup_dive(*d);
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
	struct divecomputer *dc = d->get_dc(dcNr);

	// Note that we remove events in reverse order so that the indexes don't change
	// meaning while removing. This should be an extremely rare case anyway.
	for (int idx = static_cast<int>(dc->events.size()) - 1; idx > 0; --idx) {
		const event &ev = dc->events[idx];
		if (ev.time.seconds == seconds && ev.name == "gaschange")
			eventsToRemove.push_back(idx);
		if (std::find(cylinders.begin(), cylinders.end(), ev.gas.index) == cylinders.end())
			cylinders.push_back(ev.gas.index); // cylinders might have changed their status
	}

	eventsToAdd.push_back(create_gas_switch_event(d, seconds, tank));
}

bool AddGasSwitch::workToBeDone()
{
	return true;
}

void AddGasSwitch::redoit()
{
	std::vector<event> newEventsToAdd;
	std::vector<int> newEventsToRemove;
	newEventsToAdd.reserve(eventsToRemove.size());
	newEventsToRemove.reserve(eventsToAdd.size());
	struct divecomputer *dc = d->get_dc(dcNr);

	for (int idx: eventsToRemove)
		newEventsToAdd.push_back(remove_event_from_dc(dc, idx));

	for (auto &ev: eventsToAdd)
		newEventsToRemove.push_back(add_event_to_dc(dc, std::move(ev)));

	// Make sure that events are removed in reverse order
	std::sort(newEventsToRemove.begin(), newEventsToRemove.end(), std::greater<int>());

	eventsToAdd = std::move(newEventsToAdd);
	eventsToRemove = std::move(newEventsToRemove);

	// this means we potentially have a new tank that is being used and needs to be shown
	divelog.dives.fixup_dive(*d);

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
