// SPDX-License-Identifier: GPL-2.0

#include "command_edit.h"
#include "core/divelist.h"

namespace Command {

template<typename T>
EditBase<T>::EditBase(const QVector<dive *> &divesIn, T newValue, T oldValue) :
	value(std::move(newValue)),
	old(std::move(oldValue)),
	dives(divesIn.toStdVector())
{
	// If there is nothing to do, clear the dives vector.
	// This signals that no action has to be taken.
	if (old == value)
		dives.clear();
}

// This is quite hackish: we can't use virtual functions in the constructor and
// therefore can't initialize the list of dives [the values of the dives are
// accessed by virtual functions]. Therefore, we (mis)use the fact that workToBeDone()
// is called exactly once before adding the Command to the system and perform this here.
// To be more explicit about this, we might think about renaming workToBeDone() to init().
template<typename T>
bool EditBase<T>::workToBeDone()
{
	std::vector<dive *> divesNew;
	divesNew.reserve(dives.size());
	for (dive *d: dives) {
		if (data(d) == old)
			divesNew.push_back(d);
	}
	dives = std::move(divesNew);

	// Create a text for the menu entry. In the case of multiple dives add the number
	size_t num_dives = dives.size();
	if (num_dives > 0)
		//: remove the part in parantheses for %n = 1
		setText(tr("Edit %1 (%n dive(s))", "", num_dives).arg(fieldName()));

	return num_dives;
}

template<typename T>
void EditBase<T>::undo()
{
	if (dives.empty()) {
		qWarning("Edit command called with empty dives list (shouldn't happen)");
		return;
	}

	for (dive *d: dives)
		set(d, value);

	std::swap(old, value);

	emit diveListNotifier.divesEdited(QVector<dive *>::fromStdVector(dives), fieldId());

	mark_divelist_changed(true);
}

// We have to manually instantiate the constructors of the EditBase class,
// because the base class is never constructed and the derived classes
// don't have their own constructor. They simply delegate to the base
// class by virtue of a "using" declaration.
template
EditBase<QString>::EditBase(const QVector<dive *> &dives, QString oldValue, QString newValue);

// Undo and redo do the same as just the stored value is exchanged
template<typename T>
void EditBase<T>::redo()
{
	undo();
}

// Implementation of virtual functions

// ***** Notes *****
void EditNotes::set(struct dive *d, QString s) const
{
	free(d->notes);
	d->notes = strdup(qPrintable(s));
}

QString EditNotes::data(struct dive *d) const
{
	return QString(d->notes);
}

QString EditNotes::fieldName() const
{
	return tr("notes");
}

DiveField EditNotes::fieldId() const
{
	return DiveField::NOTES;
}

// ***** Mode *****
// Editing the dive mode has very peculiar semantics for historic reasons:
// Since the dive-mode depends on the dive computer, the i-th dive computer
// of each dive will be edited. If the dive has less than i dive computers,
// the default dive computer will be edited.
// The index "i" will be stored as an additional payload with the command.
// Thus, we need an explicit constructor. Since the actual handling is done
// by the base class, which knows nothing about this index, it will not be
// sent via signals. Currently this is not needed. Should it turn out to
// become necessary, then we might either
//	- Not derive EditMode from EditBase.
//	- Change the semantics of the mode-editing.
// The future will tell.
EditMode::EditMode(const QVector<dive *> &dives, int indexIn, int newValue, int oldValue)
	: EditBase(dives, newValue, oldValue), index(indexIn)
{
}

void EditMode::set(struct dive *d, int i) const
{
	get_dive_dc(d, index)->divemode = (enum divemode_t)i;
	update_setpoint_events(d, get_dive_dc(d, index));
}

int EditMode::data(struct dive *d) const
{
	return get_dive_dc(d, index)->divemode;
}

QString EditMode::fieldName() const
{
	return tr("dive mode");
}

DiveField EditMode::fieldId() const
{
	return DiveField::MODE;
}

} // namespace Command
