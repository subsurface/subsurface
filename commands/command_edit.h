// SPDX-License-Identifier: GPL-2.0
// Note: this header file is used by the undo-machinery and should not be included elsewhere.

#ifndef COMMAND_EDIT_H
#define COMMAND_EDIT_H

#include "command_base.h"
#include "command.h" // for EditCylinderType
#include "core/divecomputer.h"
#include "core/subsurface-qt/divelistnotifier.h"

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

// The individual Edit-commands define a virtual function that return the field-id.
// For reasons, which I don't fully understand, the C++ makers are strictly opposed
// to "virtual member constants" so we have to define these functions. To make
// things a bit more compact we do this automatically with the following template.
// Of course, we could directly encode the value in the EditBase-template, but
// that would lead to a multiplication of the created code.
template <typename T, DiveField::Flags ID>
class EditTemplate : public EditBase<T> {
private:
	using EditBase<T>::EditBase;	// Use constructor of base class.
	DiveField fieldId() const override final;	// final prevents further overriding - then just don't use this template
};

// Automatically generate getter and setter in the case of simple assignments.
// The third parameter is a pointer to a member of the dive structure.
template <typename T, DiveField::Flags ID, T dive::*PTR>
class EditDefaultSetter : public EditTemplate<T, ID> {
private:
	using EditTemplate<T, ID>::EditTemplate;
	void set(struct dive *d, T) const override final;	// final prevents further overriding - then just don't use this template
	T data(struct dive *d) const override final;	// final prevents further overriding - then just don't use this template
};

// Automatically generate getter and setter in the case for string assignments.
// The third parameter is a pointer to a C-style string in the dive structure.
template <DiveField::Flags ID, std::string dive::*PTR>
class EditStringSetter : public EditTemplate<QString, ID> {
private:
	using EditTemplate<QString, ID>::EditTemplate;
	void set(struct dive *d, QString) const override final;	// final prevents further overriding - then just don't use this template
	QString data(struct dive *d) const override final;	// final prevents further overriding - then just don't use this template
};

class EditNotes : public EditStringSetter<DiveField::NOTES, &dive::notes> {
public:
	using EditStringSetter::EditStringSetter;	// Use constructor of base class.
	QString fieldName() const override;
};

class EditSuit : public EditStringSetter<DiveField::SUIT, &dive::suit> {
public:
	using EditStringSetter::EditStringSetter;	// Use constructor of base class.
	QString fieldName() const override;
};

class EditRating : public EditDefaultSetter<int, DiveField::RATING, &dive::rating> {
public:
	using EditDefaultSetter::EditDefaultSetter;	// Use constructor of base class.
	QString fieldName() const override;
};

class EditVisibility : public EditDefaultSetter<int, DiveField::VISIBILITY, &dive::visibility> {
public:
	using EditDefaultSetter::EditDefaultSetter;	// Use constructor of base class.
	QString fieldName() const override;
};

class EditWaveSize : public EditDefaultSetter<int, DiveField::WAVESIZE, &dive::wavesize> {
public:
	using EditDefaultSetter::EditDefaultSetter;	// Use constructor of base class.
	QString fieldName() const override;
};

class EditCurrent : public EditDefaultSetter<int, DiveField::CURRENT, &dive::current> {
public:
	using EditDefaultSetter::EditDefaultSetter;	// Use constructor of base class.
	QString fieldName() const override;
};

class EditSurge : public EditDefaultSetter<int, DiveField::SURGE, &dive::surge> {
public:
	using EditDefaultSetter::EditDefaultSetter;	// Use constructor of base class.
	QString fieldName() const override;
};

class EditChill : public EditDefaultSetter<int, DiveField::CHILL, &dive::chill> {
public:
	using EditDefaultSetter::EditDefaultSetter;	// Use constructor of base class.
	QString fieldName() const override;
};

class EditAirTemp : public EditTemplate<int, DiveField::AIR_TEMP> {
public:
	using EditTemplate::EditTemplate;	// Use constructor of base class.
	void set(struct dive *d, int value) const override;
	int data(struct dive *d) const override;
	QString fieldName() const override;
};

class EditWaterTemp : public EditTemplate<int, DiveField::WATER_TEMP> {
public:
	using EditTemplate::EditTemplate;	// Use constructor of base class.
	void set(struct dive *d, int value) const override;
	int data(struct dive *d) const override;
	QString fieldName() const override;
};

class EditAtmPress : public EditTemplate<int, DiveField::ATM_PRESS> {
public:
	using EditTemplate::EditTemplate;	// Use constructor of base class.
	void set(struct dive *d, int value) const override;
	int data(struct dive *d) const override;
	QString fieldName() const override;
};

class EditWaterTypeUser : public EditTemplate<int, DiveField::SALINITY> {
public:
	using EditTemplate::EditTemplate;	// Use constructor of base class.
	void set(struct dive *d, int value) const override;
	int data(struct dive *d) const override;
	QString fieldName() const override;
};

class EditDuration : public EditTemplate<int, DiveField::DURATION> {
public:
	using EditTemplate::EditTemplate;	// Use constructor of base class.
	void set(struct dive *d, int value) const override;
	int data(struct dive *d) const override;
	QString fieldName() const override;
};

class EditDepth : public EditTemplate<int, DiveField::DEPTH> {
public:
	using EditTemplate::EditTemplate;	// Use constructor of base class.
	void set(struct dive *d, int value) const override;
	int data(struct dive *d) const override;
	QString fieldName() const override;
};

class EditDiveSite : public EditTemplate<struct dive_site *, DiveField::DIVESITE> {
public:
	using EditTemplate::EditTemplate;	// Use constructor of base class.
	void set(struct dive *d, struct dive_site *value) const override;
	struct dive_site *data(struct dive *d) const override;
	QString fieldName() const override;

	// We specialize these so that we can send dive-site changed signals.
	void undo() override;
	void redo() override;
};

// Edit dive site, but add a new dive site first. Reuses the code of EditDiveSite by
// deriving from it and hooks into undo() and redo() to add / remove the dive site.
class EditDiveSiteNew : public EditDiveSite {
public:
	std::unique_ptr<dive_site> diveSiteToAdd;
	struct dive_site *diveSiteToRemove;
	EditDiveSiteNew(const QString &newName, bool currentDiveOnly);
	void undo() override;
	void redo() override;
};

class EditMode : public EditTemplate<int, DiveField::MODE> {
	int index;
public:
	EditMode(int indexIn, int newValue, bool currentDiveOnly);
	void set(struct dive *d, int i) const override;
	int data(struct dive *d) const override;
	QString fieldName() const override;
};

class EditInvalid : public EditDefaultSetter<bool, DiveField::INVALID, &dive::invalid> {
public:
	using EditDefaultSetter::EditDefaultSetter;	// Use constructor of base class.
	QString fieldName() const override;
};

// Fields that work with tag-lists (tags, buddies, dive guides) work differently and therefore
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

// See comments for EditTemplate
template <DiveField::Flags ID>
class EditTagsTemplate : public EditTagsBase {
private:
	using EditTagsBase::EditTagsBase;		// Use constructor of base class.
	DiveField fieldId() const override final;	// final prevents further overriding - then just don't use this template
};

class EditTags : public EditTagsTemplate<DiveField::TAGS> {
public:
	using EditTagsTemplate::EditTagsTemplate;	// Use constructor of base class.
	QStringList data(struct dive *d) const override;
	void set(struct dive *d, const QStringList &v) const override;
	QString fieldName() const override;
};

class EditBuddies : public EditTagsTemplate<DiveField::BUDDY> {
public:
	using EditTagsTemplate::EditTagsTemplate;	// Use constructor of base class.
	QStringList data(struct dive *d) const override;
	void set(struct dive *d, const QStringList &v) const override;
	QString fieldName() const override;
};

class EditDiveGuide : public EditTagsTemplate<DiveField::DIVEGUIDE> {
public:
	using EditTagsTemplate::EditTagsTemplate;	// Use constructor of base class.
	QStringList data(struct dive *d) const override;
	void set(struct dive *d, const QStringList &v) const override;
	QString fieldName() const override;
};

// Fields we have to remember to undo paste
struct PasteState {
	dive &d;
	std::optional<dive_site *> divesite;
	std::optional<std::string> notes;
	std::optional<std::string> diveguide;
	std::optional<std::string> buddy;
	std::optional<std::string> suit;
	std::optional<int> rating;
	std::optional<int> wavesize;
	std::optional<int> visibility;
	std::optional<int> current;
	std::optional<int> surge;
	std::optional<int> chill;
	std::optional<tag_list> tags;
	std::optional<cylinder_table> cylinders;
	std::optional<weightsystem_table> weightsystems;
	std::optional<int> number;
	std::optional<timestamp_t> when;

	PasteState(dive &d, const dive_paste_data &data, std::vector<dive_site *> &changed_dive_sites);
	~PasteState();
	void swap(); // Exchange values here and in dive
};

class PasteDives : public Base {
	dive_paste_data data;
	std::vector<PasteState> dives;
	std::vector<dive_site *> dive_sites_changed;
public:
	PasteDives(const dive_paste_data &data);
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
	std::string notes;
	pressure_t surface_pressure;
	duration_t duration;
	int salinity;
public:
	// Dive computer(s) and cylinders(s) of the source dive will be reset!
	ReplanDive(dive *source);
	~ReplanDive();
private:
	void undo() override;
	void redo() override;
	bool workToBeDone() override;
};

class EditProfile : public Base {
	dive *d;
	int dcNr;

	depth_t maxdepth, meandepth, dcmaxdepth;
	duration_t duration;
	struct divecomputer dc;
public:
	// Note: source must be clean (i.e. fixup_dive must have been called on it).
	EditProfile(const dive *source, int dcNr, EditProfileType type, int count);
	~EditProfile();
private:
	void undo() override;
	void redo() override;
	bool workToBeDone() override;
};

class AddWeight : public EditDivesBase {
public:
	AddWeight(bool currentDiveOnly);
private:
	void undo() override;
	void redo() override;
	bool workToBeDone() override;
};

class EditWeightBase : public EditDivesBase {
protected:
	EditWeightBase(int index, bool currentDiveOnly);
	~EditWeightBase();

	weightsystem_t ws;
	std::vector<int> indices; // An index for each dive in the dives vector.
	bool workToBeDone() override;
};

class RemoveWeight : public EditWeightBase {
public:
	RemoveWeight(int index, bool currentDiveOnly);
private:
	void undo() override;
	void redo() override;
};

class EditWeight : public EditWeightBase {
public:
	EditWeight(int index, weightsystem_t ws, bool currentDiveOnly); // Clones ws
	~EditWeight();
private:
	weightsystem_t new_ws;
	void undo() override;
	void redo() override;
};

class AddCylinder : public EditDivesBase {
public:
	AddCylinder(bool currentDiveOnly);
	~AddCylinder();
private:
	cylinder_t cyl;
	std::vector<int> indexes; // An index for each dive in the dives vector.
	void undo() override;
	void redo() override;
	bool workToBeDone() override;
};

class EditCylinderBase : public EditDivesBase {
protected:
	EditCylinderBase(int index, bool currentDiveOnly, bool nonProtectedOnly, int sameCylinderFlags);
	~EditCylinderBase();

	std::vector<cylinder_t> cyl;
	std::vector<int> indexes; // An index for each dive in the dives vector.
	bool workToBeDone() override;
};

class RemoveCylinder : public EditCylinderBase {
public:
	RemoveCylinder(int index, bool currentDiveOnly);
private:
	void undo() override;
	void redo() override;
};

// Instead of implementing an undo command for every single field in a cylinder,
// we only have one and pass an edit "type". We either edit the type, pressure
// or gasmix fields. This has mostly historical reasons rooted in the way the
// CylindersModel code works. The model works for undo and also in the planner
// without undo. Having a single undo-command simplifies the code there.
class EditCylinder : public EditCylinderBase {
public:
	EditCylinder(int index, cylinder_t cyl, EditCylinderType type, bool currentDiveOnly); // Clones cylinder
private:
	EditCylinderType type;
	void undo() override;
	void redo() override;
};

class EditSensors : public Base
{
public:
	EditSensors(unsigned int cylIndex, int16_t fromCylinder, int dcNr);

private:
	struct dive *d;
	struct divecomputer *dc;
	unsigned int cylinderIndex;
	int16_t	sensorId;
	std::vector<struct tank_sensor_mapping> oldTankSensorMappings;
	void undo() override;
	void redo() override;
	bool workToBeDone() override;
};

#ifdef SUBSURFACE_MOBILE
// Edit a full dive. This is used on mobile where we don't have per-field granularity.
// It may add or edit a dive site.
class EditDive : public Base {
public:
	EditDive(dive *oldDive, dive *newDive, dive_site *createDs, dive_site *editDs, location_t dsLocation); // Takes ownership of newDive
private:
	dive *oldDive; // Dive that is going to be overwritten
	std::unique_ptr<dive> newDive; // New data
	dive_site *newDiveSite;
	int changedFields;

	dive_site *siteToRemove;
	std::unique_ptr<dive_site> siteToAdd;

	dive_site *siteToEdit;
	location_t dsLocation;

	void undo() override;
	void redo() override;
	bool workToBeDone() override;

	void exchangeDives();
	void editDs();
};

#endif // SUBSURFACE_MOBILE

} // namespace Command
#endif
