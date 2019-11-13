// SPDX-License-Identifier: GPL-2.0
// Note: this header file is used by the undo-machinery and should not be included elsewhere.

#ifndef COMMAND_EDIT_TRIP_H
#define COMMAND_EDIT_TRIP_H

#include "command_base.h"
#include "core/subsurface-qt/DiveListNotifier.h"

#include <QVector>

// These are commands that edit individual fields of a a trip.
// The implementation follows the (rather verbose) OO-style of the dive-edit commands.
// But here, no template is used as the two fields are both of string type.

// We put everything in a namespace, so that we can shorten names without polluting the global namespace
namespace Command {

class EditTripBase : public Base {
	bool workToBeDone() override;

	dive_trip *trip; // Trip to be edited.
public:
	EditTripBase(dive_trip *trip, const QString &newValue);

protected:
	QString value; // Value to be set
	void undo() override;
	void redo() override;

	// Get and set functions to be overriden by sub-classes.
	virtual void set(struct dive_trip *t, const QString &) const = 0;
	virtual QString data(struct dive_trip *t) const = 0;
	virtual QString fieldName() const = 0;	// Name of the field, used to create the undo menu-entry
	virtual TripField fieldId() const = 0;
};

class EditTripLocation : public EditTripBase {
public:
	using EditTripBase::EditTripBase;	// Use constructor of base class.
	void set(dive_trip *t, const QString &s) const override;
	QString data(dive_trip *t) const override;
	QString fieldName() const override;
	TripField fieldId() const override;
};

class EditTripNotes : public EditTripBase {
public:
	using EditTripBase::EditTripBase;	// Use constructor of base class.
	void set(dive_trip *t, const QString &s) const override;
	QString data(dive_trip *t) const override;
	QString fieldName() const override;
	TripField fieldId() const override;
};

} // namespace Command

#endif
