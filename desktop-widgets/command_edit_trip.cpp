// SPDX-License-Identifier: GPL-2.0

#include "command_edit_trip.h"
#include "command_private.h"
#include "core/qthelper.h"

namespace Command {

EditTripBase::EditTripBase(dive_trip *tripIn, const QString &newValue) : trip(tripIn),
	value(newValue)
{
}

// Note: Virtual functions cannot be called in the constructor.
// Therefore, setting of the title is done here.
bool EditTripBase::workToBeDone()
{
	setText(tr("Edit %1").arg(fieldName()));
	return data(trip) != value;
}

void EditTripBase::undo()
{
	QString old = data(trip);
	set(trip, value);
	value = old;

	emit diveListNotifier.tripChanged(trip, fieldId());
}

// Undo and redo do the same as just the stored value is exchanged
void EditTripBase::redo()
{
	undo();
}

// Implementation of virtual functions

// ***** Location *****
void EditTripLocation::set(dive_trip *t, const QString &s) const
{
	free(t->location);
	t->location = copy_qstring(s);
}

QString EditTripLocation::data(dive_trip *t) const
{
	return QString(t->location);
}

QString EditTripLocation::fieldName() const
{
	return tr("trip location");
}

TripField EditTripLocation::fieldId() const
{
	return TripField::LOCATION;
}

// ***** Notes *****
void EditTripNotes::set(dive_trip *t, const QString &s) const
{
	free(t->notes);
	t->notes = copy_qstring(s);
}

QString EditTripNotes::data(dive_trip *t) const
{
	return QString(t->notes);
}

QString EditTripNotes::fieldName() const
{
	return tr("trip notes");
}

TripField EditTripNotes::fieldId() const
{
	return TripField::NOTES;
}

} // namespace Command
