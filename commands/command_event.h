// SPDX-License-Identifier: GPL-2.0
// Note: this header file is used by the undo-machinery and should not be included elsewhere.

#ifndef COMMAND_EVENT_H
#define COMMAND_EVENT_H

#include "command_base.h"
#include "core/divemode.h"

// We put everything in a namespace, so that we can shorten names without polluting the global namespace
namespace Command {

// Events are a strange thing: they contain there own description which means
// that on changing the description a new object must be allocated. Moreover,
// it means that these objects can't be collected in a table.
// Therefore, the undo commands work on events as they do with dives: using
// owning pointers. See comments in command_base.h

class EventBase : public Base {
protected:
	EventBase(struct dive *d, int dcNr);
	void undo() override;
	void redo() override;
	virtual void redoit() = 0;
	virtual void undoit() = 0;

	// Note: we store dive and the divecomputer-number instead of a pointer to the divecomputer.
	// Since one divecomputer is integrated into the dive structure, pointers to divecomputers
	// are probably not stable.
	struct dive *d;
	int dcNr;
private:
	void updateDive();
};

class AddEventBase : public EventBase {
public:
	AddEventBase(struct dive *d, int dcNr, struct event *ev); // Takes ownership of event!
protected:
	void undoit() override;
	void redoit() override;
private:
	bool workToBeDone() override;

	OwningEventPtr eventToAdd;	// for redo
	event *eventToRemove;		// for undo
};

class AddEventBookmark : public AddEventBase {
public:
	AddEventBookmark(struct dive *d, int dcNr, int seconds);
};

class AddEventDivemodeSwitch : public AddEventBase {
public:
	AddEventDivemodeSwitch(struct dive *d, int dcNr, int seconds, int divemode);
};

class AddEventSetpointChange : public AddEventBase {
public:
	AddEventSetpointChange(struct dive *d, int dcNr, int seconds, pressure_t pO2);
private:
	divemode_t divemode;	// Wonderful: this may change the divemode of the dive to CCR
	void undoit() override;
	void redoit() override;
};

class RenameEvent : public EventBase {
public:
	RenameEvent(struct dive *d, int dcNr, struct event *ev, const char *name);
private:
	bool workToBeDone() override;
	void undoit() override;
	void redoit() override;

	OwningEventPtr eventToAdd;	// for undo and redo
	event *eventToRemove;		// for undo and redo
};

class RemoveEvent : public EventBase {
public:
	RemoveEvent(struct dive *d, int dcNr, struct event *ev);
private:
	bool workToBeDone() override;
	void undoit() override;
	void redoit() override;
	void post() const; // Called to fix up dives should a gas-change have happened.

	OwningEventPtr eventToAdd;	// for undo
	event *eventToRemove;		// for redo
	int cylinder;			// affected cylinder (if removing gas switch). <0: not a gas switch.
};

class AddGasSwitch : public EventBase {
public:
	AddGasSwitch(struct dive *d, int dcNr, int seconds, int tank);
private:
	bool workToBeDone() override;
	void undoit() override;
	void redoit() override;

	std::vector<int> cylinders; // cylinders that are modified
	std::vector<OwningEventPtr> eventsToAdd;
	std::vector<event *> eventsToRemove;
};

} // namespace Command

#endif // COMMAND_EVENT_H
