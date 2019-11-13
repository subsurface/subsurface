// SPDX-License-Identifier: GPL-2.0

#include "command_edit.h"
#include "command_private.h"
#include "core/divelist.h"
#include "core/qthelper.h" // for copy_qstring
#include "core/subsurface-string.h"
#include "core/tag.h"

namespace Command {

static std::vector<dive *> getDives(bool currentDiveOnly)
{
	if (currentDiveOnly)
		return current_dive ? std::vector<dive *> { current_dive }
				    : std::vector<dive *> { };

	std::vector<dive *> res;
	struct dive *d;
	int i;
	for_each_dive (i, d) {
		if (d->selected)
			res.push_back(d);
	}
	return res;
}

EditDivesBase::EditDivesBase(bool currentDiveOnly) :
	dives(getDives(currentDiveOnly)),
	selectedDives(getDiveSelection()),
	current(current_dive)
{
}

EditDivesBase::EditDivesBase(dive *d) :
	dives({ d }),
	selectedDives(getDiveSelection()),
	current(current_dive)
{
}

int EditDivesBase::numDives() const
{
	return dives.size();
}

template<typename T>
EditBase<T>::EditBase(T newValue, bool currentDiveOnly) :
	EditDivesBase(currentDiveOnly),
	value(std::move(newValue))
{
}

template<typename T>
EditBase<T>::EditBase(T newValue, dive *d) :
	EditDivesBase(d),
	value(std::move(newValue))
{
}

// This is quite hackish: we can't use virtual functions in the constructor and
// therefore can't initialize the list of dives [the values of the dives are
// accessed by virtual functions]. Therefore, we (mis)use the fact that workToBeDone()
// is called exactly once before adding the Command to the system and perform this here.
// To be more explicit about this, we might think about renaming workToBeDone() to init().
template<typename T>
bool EditBase<T>::workToBeDone()
{
	// First, let's fetch the old value, i.e. the value of the current dive.
	// If no current dive exists, bail.
	if (!current)
		return false;
	old = data(current);

	// If there is no change - do nothing.
	if (old == value)
		return false;

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
		//: remove the part in parentheses for %n = 1
		setText(tr("Edit %1 (%n dive(s))", "", num_dives).arg(fieldName()));

	return num_dives > 0;
}

template<typename T>
void EditBase<T>::undo()
{
	if (dives.empty()) {
		qWarning("Edit command called with empty dives list (shouldn't happen)");
		return;
	}

	for (dive *d: dives) {
		set(d, value);
		invalidate_dive_cache(d); // Ensure that dive is written in git_save()
	}

	std::swap(old, value);

	// Send signals.
	DiveField id = fieldId();
	emit diveListNotifier.divesChanged(QVector<dive *>::fromStdVector(dives), id);

	setSelection(selectedDives, current);
}

// We have to manually instantiate the constructors of the EditBase class,
// because the base class is never constructed and the derived classes
// don't have their own constructor. They simply delegate to the base
// class by virtue of a "using" declaration.
template
EditBase<QString>::EditBase(QString newValue, bool currentDiveOnly);
template
EditBase<int>::EditBase(int newValue, bool currentDiveOnly);
template
EditBase<struct dive_site *>::EditBase(struct dive_site *newValue, bool currentDiveOnly);

// Undo and redo do the same as just the stored value is exchanged
template<typename T>
void EditBase<T>::redo()
{
	// Note: here, we explicitly call the undo function of EditBase<T> and don't do
	// virtual dispatch. Thus, derived classes can call this redo function without
	// having their own undo() function called.
	EditBase<T>::undo();
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

// ***** Suit *****
void EditSuit::set(struct dive *d, QString s) const
{
	free(d->suit);
	d->suit = strdup(qPrintable(s));
}

QString EditSuit::data(struct dive *d) const
{
	return QString(d->suit);
}

QString EditSuit::fieldName() const
{
	return tr("suit");
}

DiveField EditSuit::fieldId() const
{
	return DiveField::SUIT;
}

// ***** Rating *****
void EditRating::set(struct dive *d, int value) const
{
	d->rating = value;
}

int EditRating::data(struct dive *d) const
{
	return d->rating;
}

QString EditRating::fieldName() const
{
	return tr("rating");
}

DiveField EditRating::fieldId() const
{
	return DiveField::RATING;
}

// ***** Visibility *****
void EditVisibility::set(struct dive *d, int value) const
{
	d->visibility = value;
}

int EditVisibility::data(struct dive *d) const
{
	return d->visibility;
}

QString EditVisibility::fieldName() const
{
	return tr("visibility");
}

DiveField EditVisibility::fieldId() const
{
	return DiveField::VISIBILITY;
}

// ***** Air Temperature *****
void EditAirTemp::set(struct dive *d, int value) const
{
	d->airtemp.mkelvin = value > 0 ? (uint32_t)value : 0u;
}

int EditAirTemp::data(struct dive *d) const
{
	return (int)d->airtemp.mkelvin;
}

QString EditAirTemp::fieldName() const
{
	return tr("air temperature");
}

DiveField EditAirTemp::fieldId() const
{
	return DiveField::AIR_TEMP;
}

// ***** Water Temperature *****
void EditWaterTemp::set(struct dive *d, int value) const
{
	d->watertemp.mkelvin = value > 0 ? (uint32_t)value : 0u;

	// re-populate the temperatures - easiest way to do this is by calling fixup_dive
	fixup_dive(d);
}

int EditWaterTemp::data(struct dive *d) const
{
	return (int)d->watertemp.mkelvin;
}

QString EditWaterTemp::fieldName() const
{
	return tr("water temperature");
}

DiveField EditWaterTemp::fieldId() const
{
	return DiveField::WATER_TEMP;
}

// ***** Atmospheric pressure *****
void EditAtmPress::set(struct dive *d, int value) const
{
	d->surface_pressure.mbar = value > 0 ? (uint32_t)value : 0u;
}

int EditAtmPress::data(struct dive *d) const
{
	return (int)d->surface_pressure.mbar;
}

QString EditAtmPress::fieldName() const
{
	return tr("Atm. pressure");
}

DiveField EditAtmPress::fieldId() const
{
	return DiveField::ATM_PRESS;
}

// ***** Duration *****
void EditDuration::set(struct dive *d, int value) const
{
	d->dc.duration.seconds = value;
	d->duration = d->dc.duration;
	d->dc.meandepth.mm = 0;
	d->dc.samples = 0;
}

int EditDuration::data(struct dive *d) const
{
	return d->duration.seconds;
}

QString EditDuration::fieldName() const
{
	return tr("duration");
}

DiveField EditDuration::fieldId() const
{
	return DiveField::DURATION;
}

// ***** Depth *****
void EditDepth::set(struct dive *d, int value) const
{
	d->dc.maxdepth.mm = value;
	d->maxdepth = d->dc.maxdepth;
	d->dc.meandepth.mm = 0;
	d->dc.samples = 0;
}

int EditDepth::data(struct dive *d) const
{
	return d->maxdepth.mm;
}

QString EditDepth::fieldName() const
{
	return tr("depth");
}

DiveField EditDepth::fieldId() const
{
	return DiveField::DEPTH;
}

// ***** DiveSite *****
void EditDiveSite::set(struct dive *d, struct dive_site *dive_site) const
{
	unregister_dive_from_dive_site(d);
	add_dive_to_dive_site(d, dive_site);
}

struct dive_site *EditDiveSite::data(struct dive *d) const
{
	return d->dive_site;
}

QString EditDiveSite::fieldName() const
{
	return tr("dive site");
}

DiveField EditDiveSite::fieldId() const
{
	return DiveField::DIVESITE;
}

void EditDiveSite::undo()
{
	// Do the normal undo thing, then send dive site changed signals
	EditBase<dive_site *>::undo();
	if (value)
		emit diveListNotifier.diveSiteDivesChanged(value);
	if (old)
		emit diveListNotifier.diveSiteDivesChanged(old);
}

void EditDiveSite::redo()
{
	EditDiveSite::undo(); // Undo and redo do the same
}

static struct dive_site *createDiveSite(const QString &name)
{
	struct dive_site *ds = alloc_dive_site();
	struct dive_site *old = current_dive ? current_dive->dive_site : nullptr;
	if (old) {
		copy_dive_site(old, ds);
		free(ds->name); // Free name, as we will overwrite it with our own version
	}

	// If the current dive has a location, use that as location for the new dive site
	if (current_dive) {
		location_t loc = dive_get_gps_location(current_dive);
		if (has_location(&loc))
			ds->location = loc;
	}

	ds->name = copy_qstring(name);
	return ds;
}

EditDiveSiteNew::EditDiveSiteNew(const QString &newName, bool currentDiveOnly) :
	EditDiveSite(createDiveSite(newName), currentDiveOnly),
	diveSiteToAdd(value),
	diveSiteToRemove(nullptr)
{
}

void EditDiveSiteNew::undo()
{
	EditDiveSite::undo();
	int idx = unregister_dive_site(diveSiteToRemove);
	diveSiteToAdd.reset(diveSiteToRemove);
	emit diveListNotifier.diveSiteDeleted(diveSiteToRemove, idx); // Inform frontend of removed dive site.
	diveSiteToRemove = nullptr;
}

void EditDiveSiteNew::redo()
{
	diveSiteToRemove = diveSiteToAdd.get();
	int idx = register_dive_site(diveSiteToAdd.release()); // Return ownership to backend.
	emit diveListNotifier.diveSiteAdded(diveSiteToRemove, idx); // Inform frontend of new dive site.
	EditDiveSite::redo();
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
EditMode::EditMode(int indexIn, int newValue, bool currentDiveOnly)
	: EditBase(newValue, currentDiveOnly), index(indexIn)
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

// ***** Number *****
void EditNumber::set(struct dive *d, int number) const
{
	d->number = number;
}

int EditNumber::data(struct dive *d) const
{
	return d->number;
}

QString EditNumber::fieldName() const
{
	return tr("number");
}

DiveField EditNumber::fieldId() const
{
	return DiveField::NR;
}


// ***** Tag based commands *****
EditTagsBase::EditTagsBase(const QStringList &newListIn, bool currentDiveOnly) :
	EditDivesBase(currentDiveOnly),
	newList(newListIn)
{
}

// Two helper functions: returns true if first list contains any tag or
// misses any tag of second list.
static bool containsAny(const QStringList &tags1, const QStringList &tags2)
{
	return std::any_of(tags2.begin(), tags2.end(), [&tags1](const QString &tag)
			   { return tags1.contains(tag); });
}

static bool missesAny(const QStringList &tags1, const QStringList &tags2)
{
	return std::any_of(tags2.begin(), tags2.end(), [&tags1](const QString &tag)
			   { return !tags1.contains(tag); });
}

// This is quite hackish: we can't use virtual functions in the constructor and
// therefore can't initialize the list of dives [the values of the dives are
// accessed by virtual functions]. Therefore, we (mis)use the fact that workToBeDone()
// is called exactly once before adding the Command to the system and perform this here.
// To be more explicit about this, we might think about renaming workToBeDone() to init().
bool EditTagsBase::workToBeDone()
{
	// changing the tags on multiple dives is semantically strange - what's the right thing to do?
	// here's what I think... add the tags that were added to the displayed dive and remove the tags
	// that were removed from it

	// If there is no current dive, bail.
	if (!current)
		return false;

	// Calculate tags to add and tags to remove
	QStringList oldList = data(current);
	for (const QString &s: newList) {
		if (!oldList.contains(s))
			tagsToAdd.push_back(s);
	}
	for (const QString &s: oldList) {
		if (!newList.contains(s))
			tagsToRemove.push_back(s);
	}

	// Now search for all dives that either
	//	- miss a tag to be added
	//	- have a tag to be removed
	std::vector<dive *> divesNew;
	divesNew.reserve(dives.size());
	for (dive *d: dives) {
		QStringList tags = data(d);
		if (missesAny(tags, tagsToAdd) || containsAny(tags, tagsToRemove))
			divesNew.push_back(d);
	}
	dives = std::move(divesNew);

	// Create a text for the menu entry. In the case of multiple dives add the number
	size_t num_dives = dives.size();
	if (num_dives > 0)
		//: remove the part in parentheses for %n = 1
		setText(tr("Edit %1 (%n dive(s))", "", num_dives).arg(fieldName()));

	return num_dives != 0;
}

void EditTagsBase::undo()
{
	if (dives.empty()) {
		qWarning("Edit command called with empty dives list (shouldn't happen)");
		return;
	}

	for (dive *d: dives) {
		QStringList tags = data(d);
		for (const QString &tag: tagsToRemove)
			tags.removeAll(tag);
		for (const QString &tag: tagsToAdd) {
			if (!tags.contains(tag))
				tags.push_back(tag);
		}
		invalidate_dive_cache(d); // Ensure that dive is written in git_save()
		set(d, tags);
	}

	std::swap(tagsToAdd, tagsToRemove);

	// Send signals.
	DiveField id = fieldId();
	emit diveListNotifier.divesChanged(QVector<dive *>::fromStdVector(dives), id);

	setSelection(selectedDives, current);
}

// Undo and redo do the same as just the stored value is exchanged
void EditTagsBase::redo()
{
	undo();
}

// ***** Tags *****
QStringList EditTags::data(struct dive *d) const
{
	QStringList res;
	for (const struct tag_entry *tag = d->tag_list; tag; tag = tag->next)
		res.push_back(tag->tag->name);
	return res;
}

void EditTags::set(struct dive *d, const QStringList &v) const
{
	taglist_free(d->tag_list);
	d->tag_list = NULL;
	for (const QString &tag: v)
		taglist_add_tag(&d->tag_list, qPrintable(tag));
	taglist_cleanup(&d->tag_list);
}

QString EditTags::fieldName() const
{
	return tr("tags");
}

DiveField EditTags::fieldId() const
{
	return DiveField::TAGS;
}

// String list helper
static QStringList stringToList(const QString &s)
{
	QStringList res = s.split(",", QString::SkipEmptyParts);
	for (QString &str: res)
		str = str.trimmed();
	return res;
}

// ***** Buddies *****
QStringList EditBuddies::data(struct dive *d) const
{
	return stringToList(d->buddy);
}

void EditBuddies::set(struct dive *d, const QStringList &v) const
{
	QString text = v.join(", ");
	free(d->buddy);
	d->buddy = copy_qstring(text);
}

QString EditBuddies::fieldName() const
{
	return tr("buddies");
}

DiveField EditBuddies::fieldId() const
{
	return DiveField::BUDDY;
}

// ***** DiveMaster *****
QStringList EditDiveMaster::data(struct dive *d) const
{
	return stringToList(d->divemaster);
}

void EditDiveMaster::set(struct dive *d, const QStringList &v) const
{
	QString text = v.join(", ");
	free(d->divemaster);
	d->divemaster = copy_qstring(text);
}

QString EditDiveMaster::fieldName() const
{
	return tr("dive master");
}

DiveField EditDiveMaster::fieldId() const
{
	return DiveField::DIVEMASTER;
}

static void swapCandQString(QString &q, char *&c)
{
	QString tmp(c);
	free(c);
	c = copy_qstring(q);
	q = std::move(tmp);
}

PasteState::PasteState(dive *dIn, const dive *data, dive_components what) : d(dIn),
	tags(nullptr)
{
	memset(&cylinders, 0, sizeof(cylinders));
	memset(&weightsystems, 0, sizeof(weightsystems));
	if (what.notes)
		notes = data->notes;
	if (what.divemaster)
		divemaster = data->divemaster;
	if (what.buddy)
		buddy = data->buddy;
	if (what.suit)
		suit = data->suit;
	if (what.rating)
		rating = data->rating;
	if (what.visibility)
		visibility = data->visibility;
	if (what.divesite)
		divesite = data->dive_site;
	if (what.tags)
		tags = taglist_copy(data->tag_list);
	if (what.cylinders)
		copy_cylinders(&data->cylinders, &cylinders);
	if (what.weights)
		copy_weights(&data->weightsystems, &weightsystems);
}

PasteState::~PasteState()
{
	taglist_free(tags);
	clear_cylinder_table(&cylinders);
	clear_weightsystem_table(&weightsystems);
	free(weightsystems.weightsystems);
}

void PasteState::swap(dive_components what)
{
	if (what.notes)
		swapCandQString(notes, d->notes);
	if (what.divemaster)
		swapCandQString(divemaster, d->divemaster);
	if (what.buddy)
		swapCandQString(buddy, d->buddy);
	if (what.suit)
		swapCandQString(suit, d->suit);
	if (what.rating)
		std::swap(rating, d->rating);
	if (what.visibility)
		std::swap(visibility, d->visibility);
	if (what.divesite)
		std::swap(divesite, d->dive_site);
	if (what.tags)
		std::swap(tags, d->tag_list);
	if (what.cylinders)
		std::swap(cylinders, d->cylinders);
	if (what.weights)
		std::swap(weightsystems, d->weightsystems);
}

// ***** Paste *****
PasteDives::PasteDives(const dive *data, dive_components whatIn) : what(whatIn),
	current(current_dive)
{
	std::vector<dive *> selection = getDiveSelection();
	dives.reserve(selection.size());
	for (dive *d: selection)
		dives.emplace_back(d, data, what);

	setText(tr("Paste onto %n dive(s)", "", dives.size()));
}

bool PasteDives::workToBeDone()
{
	return !dives.empty();
}

void PasteDives::undo()
{
	QVector<dive *> divesToNotify; // Remember dives so that we can send signals later
	divesToNotify.reserve(dives.size());
	for (PasteState &state: dives) {
		divesToNotify.push_back(state.d);
		state.swap(what);
		invalidate_dive_cache(state.d); // Ensure that dive is written in git_save()
	}

	// Send signals.
	DiveField fields(DiveField::NONE);
	fields.notes = what.notes;
	fields.divemaster = what.divemaster;
	fields.buddy = what.buddy;
	fields.suit = what.suit;
	fields.rating = what.rating;
	fields.visibility = what.visibility;
	fields.divesite = what.divesite;
	fields.tags = what.tags;
	emit diveListNotifier.divesChanged(divesToNotify, fields);
	if (what.cylinders)
		emit diveListNotifier.cylindersReset(divesToNotify);
	if (what.weights)
		emit diveListNotifier.weightsystemsReset(divesToNotify);
}

// Redo and undo do the same
void PasteDives::redo()
{
	undo();
}

// ***** Paste *****
ReplanDive::ReplanDive(dive *source) : d(current_dive),
	dc({ 0 }),
	notes(nullptr)
{
	memset(&cylinders, 0, sizeof(cylinders));
	if (!d)
		return;

	when = source->when;
	maxdepth = source->maxdepth;
	meandepth = source->meandepth;
	notes = copy_string(source->notes);
	duration = source->duration;
	salinity = source->salinity;
	surface_pressure = source->surface_pressure;

	// This resets the dive computers and cylinders of the source dive, avoiding deep copies.
	std::swap(source->cylinders, cylinders);
	std::swap(source->dc, dc);

	setText(tr("Replan dive"));
}

ReplanDive::~ReplanDive()
{
	clear_cylinder_table(&cylinders);
	free_dive_dcs(&dc);
	free(notes);
}

bool ReplanDive::workToBeDone()
{
	return !!d;
}

void ReplanDive::undo()
{
	std::swap(d->when, when);
	std::swap(d->maxdepth, maxdepth);
	std::swap(d->meandepth, meandepth);
	std::swap(d->cylinders, cylinders);
	std::swap(d->dc, dc);
	std::swap(d->notes, notes);
	std::swap(d->surface_pressure, surface_pressure);
	std::swap(d->duration, duration);
	std::swap(d->salinity, salinity);
	fixup_dive(d);

	QVector<dive *> divesToNotify = { d };
	// Note that we have to emit cylindersReset before divesChanged, because the divesChanged
	// updates the DivePlotDataModel, which is out-of-sync and gets confused.
	emit diveListNotifier.cylindersReset(divesToNotify);
	emit diveListNotifier.divesChanged(divesToNotify, DiveField::DATETIME | DiveField::DURATION | DiveField::DEPTH | DiveField::MODE |
							  DiveField::NOTES | DiveField::SALINITY | DiveField::ATM_PRESS);
}

// Redo and undo do the same
void ReplanDive::redo()
{
	undo();
}

} // namespace Command
