// SPDX-License-Identifier: GPL-2.0
// Note: this header file is used by the undo-machinery and should not be included elsewhere.

#ifndef COMMAND_EVENT_H
#define COMMAND_EVENT_H

#include "command_base.h"
#include "core/divemode.h"
#include "core/event.h"

// We put everything in a namespace, so that we can shorten names without polluting the global namespace
namespace Command {

// Pointers to events are not stable, so we always store indexes.

class EventBase : public Base {
protected:
	EventBase(struct dive *d, int dcNr);
	void undo() override;
	void redo() override;
	virtual void redoit() = 0;
	virtual void undoit() = 0;

	// Note: we store dive and the divecomputer-number instead of a pointer to the divecomputer.
	// Pointers to divecomputers are not stable.
	struct dive *d;
	int dcNr;
private:
	void updateDive();
};

class AddEventBase : public EventBase {
public:
	AddEventBase(struct dive *d, int dcNr, struct event ev); // Takes ownership of event!
protected:
	void undoit() override;
	void redoit() override;
private:
	bool workToBeDone() override;

	struct event ev;			// for redo
	int idx;				// for undo
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
	RenameEvent(struct dive *d, int dcNr, int idx, const std::string name);
private:
	bool workToBeDone() override;
	void undoit() override;
	void redoit() override;

	int idx;		// for undo and redo
	std::string name;	// for undo and redo
};

class RemoveEvent : public EventBase {
public:
	RemoveEvent(struct dive *d, int dcNr, int idx);
private:
	bool workToBeDone() override;
	void undoit() override;
	void redoit() override;
	void post() const; // Called to fix up dives should a gas-change have happened.

	event ev;	// for undo
	int idx;	// for redo
	int cylinder;	// affected cylinder (if removing gas switch). <0: not a gas switch.
};

class AddGasSwitch : public EventBase {
public:
	AddGasSwitch(struct dive *d, int dcNr, int seconds, int tank);
private:
	bool workToBeDone() override;
	void undoit() override;
	void redoit() override;

	std::vector<int> cylinders; // cylinders that are modified
	std::vector<event> eventsToAdd;
	std::vector<int> eventsToRemove;
};

} // namespace Command

#endif // COMMAND_EVENT_H
