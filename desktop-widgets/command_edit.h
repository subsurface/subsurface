// SPDX-License-Identifier: GPL-2.0
// Note: this header file is used by the undo-machinery and should not be included elsewhere.

#ifndef COMMAND_EDIT_H
#define COMMAND_EDIT_H

#include "command_base.h"

#include <QVector>

// These are commands that edit individual fields of a set of dives.
// The implementation is very OO-style. Out-of-fashion and certainly
// not elegant, but in line with Qt's OO-based design.
// The actual code is in a common base class "Command::EditBase". To
// read and set the fields, the base class calls virtual functions of
// the derived classes.
//
// To deal with different data types, the base class is implemented
// as a template. The template parameter is the type to be read or
// set. Thus, switch-cascades and union trickery can be avoided.

// We put everything in a namespace, so that we can shorten names without polluting the global namespace
namespace Command {

template <typename T>
class EditBase : public Base {
	T value; // Value to be set
	T old; // Previous value

	void undo() override;
	void redo() override;
	bool workToBeDone() override;

	// Dives to be edited. For historical reasons, the *last* entry was
	// the active dive when the user initialized the action. This dive
	// will be made the current dive on redo / undo.
	std::vector<dive *> dives;
public:
	EditBase(const QVector<dive *> &dives, T newValue, T oldValue);

protected:
	// Get and set functions to be overriden by sub-classes.
	virtual void set(struct dive *d, T) const = 0;
	virtual T data(struct dive *d) const = 0;
	virtual QString fieldName() const = 0;	// Name of the field, used to create the undo menu-entry
};

class EditNotes : public EditBase<QString> {
public:
	using EditBase<QString>::EditBase;	// Use constructor of base class.
	void set(struct dive *d, QString s) const override;
	QString data(struct dive *d) const override;
	QString fieldName() const override;
};

} // namespace Command

#endif
