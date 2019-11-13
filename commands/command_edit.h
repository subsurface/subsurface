// SPDX-License-Identifier: GPL-2.0
// Note: this header file is used by the undo-machinery and should not be included elsewhere.

#ifndef COMMAND_EDIT_H
#define COMMAND_EDIT_H

#include "command_base.h"
#include "core/subsurface-qt/DiveListNotifier.h"

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

// Base class for commands that have a list of dives.
// This is used for extracting the number of dives and show a
// warning message when multiple dives are edited.
class EditDivesBase : public Base {
protected:
	EditDivesBase(bool currentDiveOnly);
	EditDivesBase(dive *d);
	std::vector<dive *> dives; // Dives to be edited.

	// On undo, we set the selection and current dive at the time of the operation.
	std::vector<dive *> selectedDives;
	struct dive *current;
public:
	int numDives() const;
};

template <typename T>
class EditBase : public EditDivesBase {
protected:
	T value; // Value to be set
	T old; // Previous value

	void undo() override;
	void redo() override;
	bool workToBeDone() override;

public:
	EditBase(T newValue, bool currentDiveOnly);
	EditBase(T newValue, dive *d);

protected:
	// Get and set functions to be overriden by sub-classes.
	virtual void set(struct dive *d, T) const = 0;
	virtual T data(struct dive *d) const = 0;
	virtual QString fieldName() const = 0;	// Name of the field, used to create the undo menu-entry
	virtual DiveField fieldId() const = 0;
};

class EditNotes : public EditBase<QString> {
public:
	using EditBase<QString>::EditBase;	// Use constructor of base class.
	void set(struct dive *d, QString s) const override;
	QString data(struct dive *d) const override;
	QString fieldName() const override;
	DiveField fieldId() const override;
};

class EditSuit : public EditBase<QString> {
public:
	using EditBase<QString>::EditBase;	// Use constructor of base class.
	void set(struct dive *d, QString s) const override;
	QString data(struct dive *d) const override;
	QString fieldName() const override;
	DiveField fieldId() const override;
};

class EditRating : public EditBase<int> {
public:
	using EditBase<int>::EditBase;	// Use constructor of base class.
	void set(struct dive *d, int value) const override;
	int data(struct dive *d) const override;
	QString fieldName() const override;
	DiveField fieldId() const override;
};

class EditVisibility : public EditBase<int> {
public:
	using EditBase<int>::EditBase;	// Use constructor of base class.
	void set(struct dive *d, int value) const override;
	int data(struct dive *d) const override;
	QString fieldName() const override;
	DiveField fieldId() const override;
};

class EditAirTemp : public EditBase<int> {
public:
	using EditBase<int>::EditBase;	// Use constructor of base class.
	void set(struct dive *d, int value) const override;
	int data(struct dive *d) const override;
	QString fieldName() const override;
	DiveField fieldId() const override;
};

class EditWaterTemp : public EditBase<int> {
public:
	using EditBase<int>::EditBase;	// Use constructor of base class.
	void set(struct dive *d, int value) const override;
	int data(struct dive *d) const override;
	QString fieldName() const override;
	DiveField fieldId() const override;
};

class EditAtmPress : public EditBase<int> {
public:
	using EditBase<int>::EditBase;	// Use constructor of base class.
	void set(struct dive *d, int value) const override;
	int data(struct dive *d) const override;
	QString fieldName() const override;
	DiveField fieldId() const override;
};

class EditDuration : public EditBase<int> {
public:
	using EditBase<int>::EditBase;	// Use constructor of base class.
	void set(struct dive *d, int value) const override;
	int data(struct dive *d) const override;
	QString fieldName() const override;
	DiveField fieldId() const override;
};

class EditDepth : public EditBase<int> {
public:
	using EditBase<int>::EditBase;	// Use constructor of base class.
	void set(struct dive *d, int value) const override;
	int data(struct dive *d) const override;
	QString fieldName() const override;
	DiveField fieldId() const override;
};

class EditDiveSite : public EditBase<struct dive_site *> {
public:
	using EditBase<struct dive_site *>::EditBase;	// Use constructor of base class.
	void set(struct dive *d, struct dive_site *value) const override;
	struct dive_site *data(struct dive *d) const override;
	QString fieldName() const override;
	DiveField fieldId() const override;

	// We specialize these so that we can send dive-site changed signals.
	void undo() override;
	void redo() override;
};

// Edit dive site, but add a new dive site first. Reuses the code of EditDiveSite by
// deriving from it and hooks into undo() and redo() to add / remove the dive site.
class EditDiveSiteNew : public EditDiveSite {
public:
	OwningDiveSitePtr diveSiteToAdd;
	struct dive_site *diveSiteToRemove;
	EditDiveSiteNew(const QString &newName, bool currentDiveOnly);
	void undo() override;
	void redo() override;
};

class EditMode : public EditBase<int> {
	int index;
public:
	EditMode(int indexIn, int newValue, bool currentDiveOnly);
	void set(struct dive *d, int i) const override;
	int data(struct dive *d) const override;
	QString fieldName() const override;
	DiveField fieldId() const override;
};

class EditNumber : public EditBase<int> {
public:
	using EditBase<int>::EditBase;	// Use constructor of base class.
	void set(struct dive *d, int number) const override;
	int data(struct dive *d) const override;
	QString fieldName() const override;
	DiveField fieldId() const override;
};

// Fields that work with tag-lists (tags, buddies, divemasters) work differently and therefore
// have their own base class. In this case, it's not a template, as all these lists are base
// on strings.
class EditTagsBase : public EditDivesBase {
	bool workToBeDone() override;

	QStringList newList;	// Temporary until initialized
public:
	EditTagsBase(const QStringList &newList, bool currentDiveOnly);

protected:
	QStringList tagsToAdd;
	QStringList tagsToRemove;
	void undo() override;
	void redo() override;

	// Getters, setters and parsers to be overriden by sub-classes.
	virtual QStringList data(struct dive *d) const = 0;
	virtual void set(struct dive *d, const QStringList &v) const = 0;
	virtual QString fieldName() const = 0;	// Name of the field, used to create the undo menu-entry
	virtual DiveField fieldId() const = 0;
};

class EditTags : public EditTagsBase {
public:
	using EditTagsBase::EditTagsBase;	// Use constructor of base class.
	QStringList data(struct dive *d) const override;
	void set(struct dive *d, const QStringList &v) const override;
	QString fieldName() const override;
	DiveField fieldId() const override;
};

class EditBuddies : public EditTagsBase {
public:
	using EditTagsBase::EditTagsBase;	// Use constructor of base class.
	QStringList data(struct dive *d) const override;
	void set(struct dive *d, const QStringList &v) const override;
	QString fieldName() const override;
	DiveField fieldId() const override;
};

class EditDiveMaster : public EditTagsBase {
public:
	using EditTagsBase::EditTagsBase;	// Use constructor of base class.
	QStringList data(struct dive *d) const override;
	void set(struct dive *d, const QStringList &v) const override;
	QString fieldName() const override;
	DiveField fieldId() const override;
};

// Fields we have to remember to undo paste
struct PasteState {
	dive *d;
	dive_site *divesite;
	QString notes;
	QString divemaster;
	QString buddy;
	QString suit;
	int rating;
	int visibility;
	tag_entry *tags;
	struct cylinder_table cylinders;
	struct weightsystem_table weightsystems;

	PasteState(dive *d, const dive *data, dive_components what); // Read data from dive data for dive d
	~PasteState();
	void swap(dive_components what); // Exchange values here and in dive
};

class PasteDives : public Base {
	dive_components what;
	std::vector<PasteState> dives;
	dive *current;
public:
	PasteDives(const dive *d, dive_components what);
private:
	void undo() override;
	void redo() override;
	bool workToBeDone() override;
};

class ReplanDive : public Base {
	dive *d;

	// Exchange these data with current dive
	timestamp_t when;
	depth_t maxdepth, meandepth;
	struct cylinder_table cylinders;
	struct divecomputer dc;
	char *notes;
	pressure_t surface_pressure;
	duration_t duration;
	int salinity;
public:
	ReplanDive(dive *source); // Dive computer(s) and cylinders(s) of the source dive will be reset!
	~ReplanDive();
private:
	void undo() override;
	void redo() override;
	bool workToBeDone() override;
};

} // namespace Command

#endif
