// SPDX-License-Identifier: GPL-2.0

#include "command_edit.h"
#include "core/divelist.h"
#include "core/fulltext.h"
#include "core/qthelper.h" // for copy_qstring
#include "core/selection.h"
#include "core/subsurface-string.h"
#include "core/tag.h"
#include "qt-models/weightsysteminfomodel.h"
#include "qt-models/tankinfomodel.h"
#ifdef SUBSURFACE_MOBILE
#include "qt-models/divelocationmodel.h"
#endif

namespace Command {

template <typename T, DiveField::Flags ID>
DiveField EditTemplate<T, ID>::fieldId() const
{
	return ID;
}

template <DiveField::Flags ID>
DiveField EditTagsTemplate<ID>::fieldId() const
{
	return ID;
}

template <typename T, DiveField::Flags ID, T dive::*PTR>
void EditDefaultSetter<T, ID, PTR>::set(struct dive *d, T v) const
{
	d->*PTR = v;
}

template <typename T, DiveField::Flags ID, T dive::*PTR>
T EditDefaultSetter<T, ID, PTR>::data(struct dive *d) const
{
	return d->*PTR;
}

template <DiveField::Flags ID, char *dive::*PTR>
void EditStringSetter<ID, PTR>::set(struct dive *d, QString v) const
{
	free(d->*PTR);
	d->*PTR = copy_qstring(v);
}

template <DiveField::Flags ID, char *dive::*PTR>
QString EditStringSetter<ID, PTR>::data(struct dive *d) const
{
	return QString(d->*PTR);
}

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
	if (num_dives == 1)
		setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("Edit %1").arg(fieldName())).arg(diveNumberOrDate(dives[0])));
	else if (num_dives > 0)
		setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("Edit %1 (%n dive(s))", "", num_dives).arg(fieldName())).arg(getListOfDives(dives)));

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
		fulltext_register(d); // Update the fulltext cache
		invalidate_dive_cache(d); // Ensure that dive is written in git_save()
	}

	std::swap(old, value);

	// Send signals.
	DiveField id = fieldId();
	emit diveListNotifier.divesChanged(stdToQt<dive *>(dives), id);
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
EditBase<bool>::EditBase(bool newValue, bool currentDiveOnly);
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
QString EditNotes::fieldName() const
{
	return Command::Base::tr("notes");
}

// ***** Suit *****
QString EditSuit::fieldName() const
{
	return Command::Base::tr("suit");
}

// ***** Rating *****
QString EditRating::fieldName() const
{
	return Command::Base::tr("rating");
}

// ***** Visibility *****
QString EditVisibility::fieldName() const
{
	return Command::Base::tr("visibility");
}

// ***** WaveSize *****
QString EditWaveSize::fieldName() const
{
	return Command::Base::tr("wavesize");
}

// ***** Current *****
QString EditCurrent::fieldName() const
{
	return Command::Base::tr("current");
}

// ***** Surge *****
QString EditSurge::fieldName() const
{
	return Command::Base::tr("surge");
}

// ***** Chill *****
QString EditChill::fieldName() const
{
	return Command::Base::tr("chill");
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
	return Command::Base::tr("air temperature");
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
	return Command::Base::tr("water temperature");
}

// ***** Water Type *****
void EditWaterTypeUser::set(struct dive *d, int value) const
{
	d->user_salinity = value > 0 ? value : 0;
}

int EditWaterTypeUser::data(struct dive *d) const
{
	return d->user_salinity;
}

QString EditWaterTypeUser::fieldName() const
{
	return Command::Base::tr("salinity");
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
	return Command::Base::tr("Atm. pressure");
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
	return Command::Base::tr("duration");
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
	return Command::Base::tr("depth");
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
	return Command::Base::tr("dive site");
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
	: EditTemplate(newValue, currentDiveOnly), index(indexIn)
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
	return Command::Base::tr("dive mode");
}

// ***** Invalid *****
QString EditInvalid::fieldName() const
{
	return Command::Base::tr("invalid");
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
	if (num_dives == 1)
		setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("Edit %1").arg(fieldName())).arg(diveNumberOrDate(dives[0])));
	else if (num_dives > 0)
		setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("Edit %1 (%n dive(s))", "", num_dives).arg(fieldName())).arg(getListOfDives(dives)));

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
		set(d, tags);
		fulltext_register(d); // Update the fulltext cache
		invalidate_dive_cache(d); // Ensure that dive is written in git_save()
	}

	std::swap(tagsToAdd, tagsToRemove);

	// Send signals.
	DiveField id = fieldId();
	emit diveListNotifier.divesChanged(stdToQt<dive *>(dives), id);
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
	return Command::Base::tr("tags");
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
	return Command::Base::tr("buddies");
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
	return Command::Base::tr("dive master");
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
	if (what.wavesize)
		wavesize = data->wavesize;
	if (what.current)
		current = data->current;
	if (what.surge)
		surge = data->surge;
	if (what.chill)
		chill = data->chill;
	if (what.divesite)
		divesite = data->dive_site;
	if (what.tags)
		tags = taglist_copy(data->tag_list);
	if (what.cylinders) {
		copy_cylinders(&data->cylinders, &cylinders);
		// Paste cylinders is "special":
		// 1) For cylinders that exist in the destination dive we keep the gas-mix and pressures.
		// 2) For cylinders that do not yet exist in the destination dive, we set the pressures to 0, i.e. unset.
		//    Moreover, for these we set the manually_added flag, because they weren't downloaded from a DC.
		for (int i = 0; i < d->cylinders.nr && i < cylinders.nr; ++i) {
			const cylinder_t &src = *get_cylinder(d, i);
			cylinder_t &dst = cylinders.cylinders[i];
			dst.gasmix = src.gasmix;
			dst.start = src.start;
			dst.end = src.end;
			dst.sample_start = src.sample_start;
			dst.sample_end = src.sample_end;
			dst.depth = src.depth;
			dst.manually_added = src.manually_added;
			dst.gas_used = src.gas_used;
			dst.deco_gas_used = src.deco_gas_used;
			dst.cylinder_use = src.cylinder_use;
			dst.bestmix_o2 = src.bestmix_o2;
			dst.bestmix_he = src.bestmix_he;
		}
		for (int i = d->cylinders.nr; i < cylinders.nr; ++i) {
			cylinder_t &cyl = cylinders.cylinders[i];
			cyl.start.mbar = 0;
			cyl.end.mbar = 0;
			cyl.sample_start.mbar = 0;
			cyl.sample_end.mbar = 0;
			cyl.manually_added = true;
		}
	}
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
	if (what.wavesize)
		std::swap(wavesize, d->wavesize);
	if (what.current)
		std::swap(current, d->current);
	if (what.surge)
		std::swap(surge, d->surge);
	if (what.chill)
		std::swap(chill, d->chill);
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
PasteDives::PasteDives(const dive *data, dive_components whatIn) : what(whatIn)
{
	std::vector<dive *> selection = getDiveSelection();
	dives.reserve(selection.size());
	for (dive *d: selection)
		dives.emplace_back(d, data, what);

	setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("Paste onto %n dive(s)", "", dives.size())).arg(getListOfDives(selection)));
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
	fields.wavesize = what.wavesize;
	fields.current = what.current;
	fields.surge = what.surge;
	fields.chill = what.chill;
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

// ***** ReplanDive *****
ReplanDive::ReplanDive(dive *source, bool edit_profile) : d(current_dive),
	when(0),
	maxdepth({0}),
	meandepth({0}),
	dc({ 0 }),
	notes(nullptr),
	surface_pressure({0}),
	duration({0}),
	salinity(0)
{
	memset(&cylinders, 0, sizeof(cylinders));
	if (!d)
		return;

	// Fix source. Things might be inconsistent after modifying the profile.
	source->maxdepth.mm = source->dc.maxdepth.mm = 0;
	fixup_dive(source);

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

	setText((edit_profile ? Command::Base::tr("Replan dive") : Command::Base::tr("Edit profile")) + diveNumberOrDate(d));
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
	invalidate_dive_cache(d); // Ensure that dive is written in git_save()

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

// ***** Add Weight *****
AddWeight::AddWeight(bool currentDiveOnly) :
	EditDivesBase(currentDiveOnly)
{
	if (dives.size() == 1)
		setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("Add weight")).arg(diveNumberOrDate(dives[0])));
	else
		setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("Add weight (%n dive(s))", "", dives.size())).arg(getListOfDives(dives)));
}

bool AddWeight::workToBeDone()
{
	return true;
}

void AddWeight::undo()
{
	for (dive *d: dives) {
		if (d->weightsystems.nr <= 0)
			continue;
		remove_weightsystem(d, d->weightsystems.nr - 1);
		emit diveListNotifier.weightRemoved(d, d->weightsystems.nr);
		invalidate_dive_cache(d); // Ensure that dive is written in git_save()
	}
}

void AddWeight::redo()
{
	for (dive *d: dives) {
		add_cloned_weightsystem(&d->weightsystems, empty_weightsystem);
		emit diveListNotifier.weightAdded(d, d->weightsystems.nr - 1);
		invalidate_dive_cache(d); // Ensure that dive is written in git_save()
	}
}

static int find_weightsystem_index(const struct dive *d, weightsystem_t ws)
{
	for (int idx = 0; idx < d->weightsystems.nr; ++idx) {
		if (same_weightsystem(d->weightsystems.weightsystems[idx], ws))
			return idx;
	}
	return -1;
}

EditWeightBase::EditWeightBase(int index, bool currentDiveOnly) :
	EditDivesBase(currentDiveOnly),
	ws(empty_weightsystem)
{
	// Get the old weightsystem, bail if index is invalid
	if (!current || index < 0 || index >= current->weightsystems.nr) {
		dives.clear();
		return;
	}
	ws = clone_weightsystem(current->weightsystems.weightsystems[index]);

	// Deleting a weightsystem from multiple dives is semantically ill-defined.
	// What we will do is trying to delete the same weightsystem if it exists.
	// For that purpose, we will determine the indices of the same weightsystem.
	std::vector<dive *> divesNew;
	divesNew.reserve(dives.size());
	indices.reserve(dives.size());

	for (dive *d: dives) {
		if (d == current) {
			divesNew.push_back(d);
			indices.push_back(index);
			continue;
		}
		int idx = find_weightsystem_index(d, ws);
		if (idx >= 0) {
			divesNew.push_back(d);
			indices.push_back(idx);
		}
	}
	dives = std::move(divesNew);
}

EditWeightBase::~EditWeightBase()
{
	free_weightsystem(ws);
}

bool EditWeightBase::workToBeDone()
{
	return !dives.empty();
}

// ***** Remove Weight *****
RemoveWeight::RemoveWeight(int index, bool currentDiveOnly) :
	EditWeightBase(index, currentDiveOnly)
{
	size_t num_dives = dives.size();
	if (num_dives == 1)
		setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("Remove weight")).arg(diveNumberOrDate(dives[0])));
	else
		setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("Remove weight (%n dive(s))", "", num_dives)).arg(getListOfDives(dives)));
}

void RemoveWeight::undo()
{
	for (size_t i = 0; i < dives.size(); ++i) {
		add_to_weightsystem_table(&dives[i]->weightsystems, indices[i], clone_weightsystem(ws));
		emit diveListNotifier.weightAdded(dives[i], indices[i]);
		invalidate_dive_cache(dives[i]); // Ensure that dive is written in git_save()
	}
}

void RemoveWeight::redo()
{
	for (size_t i = 0; i < dives.size(); ++i) {
		remove_weightsystem(dives[i], indices[i]);
		emit diveListNotifier.weightRemoved(dives[i], indices[i]);
		invalidate_dive_cache(dives[i]); // Ensure that dive is written in git_save()
	}
}

// ***** Edit Weight *****
EditWeight::EditWeight(int index, weightsystem_t wsIn, bool currentDiveOnly) :
	EditWeightBase(index, currentDiveOnly),
	new_ws(empty_weightsystem)
{
	if (dives.empty())
		return;

	size_t num_dives = dives.size();
	if (num_dives == 1)
		setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("Edit weight")).arg(diveNumberOrDate(dives[0])));
	else
		setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("Edit weight (%n dive(s))", "", num_dives)).arg(getListOfDives(dives)));

	// Try to untranslate the weightsystem name
	new_ws = clone_weightsystem(wsIn);
	QString vString(new_ws.description);
	for (int i = 0; i < MAX_WS_INFO && ws_info[i].name; ++i) {
		if (gettextFromC::tr(ws_info[i].name) == vString) {
			free_weightsystem(new_ws);
			new_ws.description = copy_string(ws_info[i].name);
			break;
		}
	}

	// If that doesn't change anything, do nothing
	if (same_weightsystem(ws, new_ws)) {
		dives.clear();
		return;
	}

	WSInfoModel *wsim = WSInfoModel::instance();
	QModelIndexList matches = wsim->match(wsim->index(0, 0), Qt::DisplayRole, gettextFromC::tr(new_ws.description));
	if (!matches.isEmpty())
		wsim->setData(wsim->index(matches.first().row(), WSInfoModel::GR), new_ws.weight.grams);
}

EditWeight::~EditWeight()
{
	free_weightsystem(new_ws);
}

void EditWeight::redo()
{
	for (size_t i = 0; i < dives.size(); ++i) {
		set_weightsystem(dives[i], indices[i], new_ws);
		emit diveListNotifier.weightEdited(dives[i], indices[i]);
		invalidate_dive_cache(dives[i]); // Ensure that dive is written in git_save()
	}
	std::swap(ws, new_ws);
}

// Undo and redo do the same as just the stored value is exchanged
void EditWeight::undo()
{
	redo();
}

// ***** Add Cylinder *****
AddCylinder::AddCylinder(bool currentDiveOnly) :
	EditDivesBase(currentDiveOnly),
	cyl(empty_cylinder)
{
	if (dives.empty())
		return;
	else if (dives.size() == 1)
		setText(Command::Base::tr("Add cylinder"));
	else
		setText(Command::Base::tr("Add cylinder (%n dive(s))", "", dives.size()));
	cyl = create_new_cylinder(dives[0]);
}

AddCylinder::~AddCylinder()
{
	free_cylinder(cyl);
}

bool AddCylinder::workToBeDone()
{
	return true;
}

void AddCylinder::undo()
{
	for (dive *d: dives) {
		if (d->cylinders.nr <= 0)
			continue;
		remove_cylinder(d, d->cylinders.nr - 1);
		update_cylinder_related_info(d);
		emit diveListNotifier.cylinderRemoved(d, d->cylinders.nr);
		invalidate_dive_cache(d); // Ensure that dive is written in git_save()
	}
}

void AddCylinder::redo()
{
	for (dive *d: dives) {
		add_cloned_cylinder(&d->cylinders, cyl);
		update_cylinder_related_info(d);
		emit diveListNotifier.cylinderAdded(d, d->cylinders.nr - 1);
		invalidate_dive_cache(d); // Ensure that dive is written in git_save()
	}
}

static bool same_cylinder_type(const cylinder_t &cyl1, const cylinder_t &cyl2)
{
	return same_string(cyl1.type.description, cyl2.type.description) &&
	       cyl1.cylinder_use == cyl2.cylinder_use;
}

static bool same_cylinder_size(const cylinder_t &cyl1, const cylinder_t &cyl2)
{
	return cyl1.type.size.mliter == cyl2.type.size.mliter &&
	       cyl1.type.workingpressure.mbar == cyl2.type.workingpressure.mbar;
}

static bool same_cylinder_pressure(const cylinder_t &cyl1, const cylinder_t &cyl2)
{
	return cyl1.start.mbar == cyl2.start.mbar &&
	       cyl1.end.mbar == cyl2.end.mbar;
}

// Flags for comparing cylinders
static const constexpr int SAME_TYPE = 1 << 0;
static const constexpr int SAME_SIZE = 1 << 1;
static const constexpr int SAME_PRESS = 1 << 2;
static const constexpr int SAME_GAS = 1 << 3;

static bool same_cylinder_with_flags(const cylinder_t &cyl1, const cylinder_t &cyl2, int sameCylinderFlags)
{
	return (((sameCylinderFlags & SAME_TYPE)  == 0 || same_cylinder_type(cyl1, cyl2)) &&
		((sameCylinderFlags & SAME_SIZE)  == 0 || same_cylinder_size(cyl1, cyl2)) &&
		((sameCylinderFlags & SAME_PRESS) == 0 || same_cylinder_pressure(cyl1, cyl2)) &&
		((sameCylinderFlags & SAME_GAS)   == 0 || same_gasmix(cyl1.gasmix, cyl2.gasmix)));
}

static int find_cylinder_index(const struct dive *d, const cylinder_t &cyl, int sameCylinderFlags)
{
	for (int idx = 0; idx < d->cylinders.nr; ++idx) {
		if (same_cylinder_with_flags(cyl, *get_cylinder(d, idx), sameCylinderFlags))
			return idx;
	}
	return -1;
}

EditCylinderBase::EditCylinderBase(int index, bool currentDiveOnly, bool nonProtectedOnly, int sameCylinderFlags) :
	EditDivesBase(currentDiveOnly)
{
	// Get the old cylinder, bail if index is invalid
	if (!current || index < 0 || index >= current->cylinders.nr) {
		dives.clear();
		return;
	}
	const cylinder_t &orig = *get_cylinder(current, index);

	std::vector<dive *> divesNew;
	divesNew.reserve(dives.size());
	indexes.reserve(dives.size());
	cyl.reserve(dives.size());

	for (dive *d: dives) {
		int idx = d == current ? index : find_cylinder_index(d, orig, sameCylinderFlags);
		if (idx < 0 || (nonProtectedOnly && is_cylinder_prot(d, idx)))
			continue;
		divesNew.push_back(d);
		indexes.push_back(idx);
		cyl.push_back(clone_cylinder(*get_cylinder(d, idx)));
	}
	dives = std::move(divesNew);
}

EditCylinderBase::~EditCylinderBase()
{
	for (cylinder_t c: cyl)
		free_cylinder(c);
}

bool EditCylinderBase::workToBeDone()
{
	return !dives.empty();
}

// ***** Remove Cylinder *****
RemoveCylinder::RemoveCylinder(int index, bool currentDiveOnly) :
	EditCylinderBase(index, currentDiveOnly, true, SAME_TYPE | SAME_PRESS | SAME_GAS)
{
	if (dives.size() == 1)
		setText(Command::Base::tr("Remove cylinder"));
	else
		setText(Command::Base::tr("Remove cylinder (%n dive(s))", "", dives.size()));
}

void RemoveCylinder::undo()
{
	for (size_t i = 0; i < dives.size(); ++i) {
		std::vector<int> mapping = get_cylinder_map_for_add(dives[i]->cylinders.nr, indexes[i]);
		add_cylinder(&dives[i]->cylinders, indexes[i], clone_cylinder(cyl[i]));
		update_cylinder_related_info(dives[i]);
		emit diveListNotifier.cylinderAdded(dives[i], indexes[i]);
		invalidate_dive_cache(dives[i]); // Ensure that dive is written in git_save()
	}
}

void RemoveCylinder::redo()
{
	for (size_t i = 0; i < dives.size(); ++i) {
		std::vector<int> mapping = get_cylinder_map_for_remove(dives[i]->cylinders.nr, indexes[i]);
		remove_cylinder(dives[i], indexes[i]);
		cylinder_renumber(dives[i], &mapping[0]);
		update_cylinder_related_info(dives[i]);
		emit diveListNotifier.cylinderRemoved(dives[i], indexes[i]);
		invalidate_dive_cache(dives[i]); // Ensure that dive is written in git_save()
	}
}

static int editCylinderTypeToFlags(EditCylinderType type)
{
	switch (type) {
	default:
	case EditCylinderType::TYPE:
		return SAME_TYPE | SAME_SIZE;
	case EditCylinderType::PRESSURE:
		return SAME_PRESS;
	case EditCylinderType::GASMIX:
		return SAME_GAS;
	}
}

// ***** Edit Cylinder *****
EditCylinder::EditCylinder(int index, cylinder_t cylIn, EditCylinderType typeIn, bool currentDiveOnly) :
	EditCylinderBase(index, currentDiveOnly, false, editCylinderTypeToFlags(typeIn)),
	type(typeIn)
{
	if (dives.empty())
		return;

	if (dives.size() == 1)
		setText(Command::Base::tr("Edit cylinder"));
	else
		setText(Command::Base::tr("Edit cylinder (%n dive(s))", "", dives.size()));

	// Try to untranslate the cylinder type
	QString description = cylIn.type.description;
	for (int i = 0; i < tank_info_table.nr; ++i) {
		if (gettextFromC::tr(tank_info_table.infos[i].name) == description) {
			description = tank_info_table.infos[i].name;
			break;
		}
	}

	// Update the tank info model
	TankInfoModel *tim = TankInfoModel::instance();
	QModelIndexList matches = tim->match(tim->index(0, 0), Qt::DisplayRole, gettextFromC::tr(cylIn.type.description));
	if (!matches.isEmpty()) {
		if (cylIn.type.size.mliter != cyl[0].type.size.mliter)
			tim->setData(tim->index(matches.first().row(), TankInfoModel::ML), cylIn.type.size.mliter);
		if (cylIn.type.workingpressure.mbar != cyl[0].type.workingpressure.mbar)
			tim->setData(tim->index(matches.first().row(), TankInfoModel::BAR), cylIn.type.workingpressure.mbar / 1000.0);
	}

	// The base class copied the cylinders for us, let's edit them
	for (int i = 0; i < (int)indexes.size(); ++i) {
		switch (type) {
		case EditCylinderType::TYPE:
			free((void *)cyl[i].type.description);
			cyl[i].type = cylIn.type;
			cyl[i].type.description = copy_qstring(description);
			cyl[i].cylinder_use = cylIn.cylinder_use;
			break;
		case EditCylinderType::PRESSURE:
			cyl[i].start.mbar = cylIn.start.mbar;
			cyl[i].end.mbar = cylIn.end.mbar;
			break;
		case EditCylinderType::GASMIX:
			cyl[i].gasmix = cylIn.gasmix;
			cyl[i].bestmix_o2 = cylIn.bestmix_o2;
			cyl[i].bestmix_he = cylIn.bestmix_he;
			sanitize_gasmix(&cyl[i].gasmix);
			break;
		}
	}
}

void EditCylinder::redo()
{
	for (size_t i = 0; i < dives.size(); ++i) {
		std::swap(*get_cylinder(dives[i], indexes[i]), cyl[i]);
		update_cylinder_related_info(dives[i]);
		emit diveListNotifier.cylinderEdited(dives[i], indexes[i]);
		invalidate_dive_cache(dives[i]); // Ensure that dive is written in git_save()
	}
}

// Undo and redo do the same as just the stored value is exchanged
void EditCylinder::undo()
{
	redo();
}

#ifdef SUBSURFACE_MOBILE

EditDive::EditDive(dive *oldDiveIn, dive *newDiveIn, dive_site *createDs, dive_site *editDs, location_t dsLocationIn)
	: oldDive(oldDiveIn)
	, newDive(newDiveIn)
	, newDiveSite(newDiveIn->dive_site)
	, changedFields(DiveField::NONE)
	, siteToRemove(nullptr)
	, siteToAdd(createDs)
	, siteToEdit(editDs)
	, dsLocation(dsLocationIn)
{
	if (!oldDive || !newDive)
		return;

	setText(Command::Base::tr("Edit dive [%1]").arg(diveNumberOrDate(oldDive)));

	// Calculate the fields that changed.
	// Note: Probably not needed, as on mobile we don't have that granularity.
	// However, for future-proofeness let's just do it.
	changedFields = DiveField::NONE;
	if (oldDive->number != newDive->number)
		changedFields |= DiveField::NR;
	if (oldDive->when != newDive->when)
		changedFields |= DiveField::DATETIME;
	if (oldDive->maxdepth.mm != newDive->maxdepth.mm)
		changedFields |= DiveField::DEPTH;
	if (oldDive->duration.seconds != newDive->duration.seconds)
		changedFields |= DiveField::DURATION;
	if (oldDive->airtemp.mkelvin != newDive->airtemp.mkelvin)
		changedFields |= DiveField::AIR_TEMP;
	if (oldDive->watertemp.mkelvin != newDive->watertemp.mkelvin)
		changedFields |= DiveField::WATER_TEMP;
	if (oldDive->surface_pressure.mbar != newDive->surface_pressure.mbar)
		changedFields |= DiveField::ATM_PRESS;
	if (oldDive->dive_site != newDive->dive_site)
		changedFields |= DiveField::DIVESITE;
	if (!same_string(oldDive->divemaster, newDive->divemaster))
		changedFields |= DiveField::DIVEMASTER;
	if (!same_string(oldDive->buddy, newDive->buddy))
		changedFields |= DiveField::BUDDY;
	if (oldDive->rating != newDive->rating)
		changedFields |= DiveField::RATING;
	if (oldDive->visibility != newDive->visibility)
		changedFields |= DiveField::VISIBILITY;
	if (oldDive->wavesize != newDive->wavesize)
		changedFields |= DiveField::WAVESIZE;
	if (oldDive->current != newDive->current)
		changedFields |= DiveField::CURRENT;
	if (oldDive->surge != newDive->surge)
		changedFields |= DiveField::SURGE;
	if (oldDive->chill != newDive->chill)
		changedFields |= DiveField::CHILL;
	if (!same_string(oldDive->suit, newDive->suit))
		changedFields |= DiveField::SUIT;
	if (get_taglist_string(oldDive->tag_list) != get_taglist_string(newDive->tag_list)) // This is cheating. Do we have a taglist comparison function?
		changedFields |= DiveField::TAGS;
	if (oldDive->dc.divemode != newDive->dc.divemode)
		changedFields |= DiveField::MODE;
	if (!same_string(oldDive->notes, newDive->notes))
		changedFields |= DiveField::NOTES;
	if (oldDive->salinity != newDive->salinity)
		changedFields |= DiveField::SALINITY;

	newDive->dive_site = nullptr; // We will add the dive to the site manually and therefore saved the dive site.
}

void EditDive::undo()
{
	if (siteToRemove) {
		int idx = unregister_dive_site(siteToRemove);
		siteToAdd.reset(siteToRemove);
		emit diveListNotifier.diveSiteDeleted(siteToRemove, idx); // Inform frontend of removed dive site.
	}

	exchangeDives();
	editDs();
}

void EditDive::redo()
{
	if (siteToAdd) {
		siteToRemove = siteToAdd.get();
		int idx = register_dive_site(siteToAdd.release()); // Return ownership to backend.
		emit diveListNotifier.diveSiteAdded(siteToRemove, idx); // Inform frontend of new dive site.
	}

	exchangeDives();
	editDs();
}

void EditDive::exchangeDives()
{
	// Set the filter flag of the new dive to the old dive.
	// Reason: When we send the dive-changed signal, the model will
	// track the *change* of the filter flag, so we must not overwrite
	// it by swapping the dive data.
	newDive->hidden_by_filter = oldDive->hidden_by_filter;

	// Bluntly exchange dive data by shallow copy.
	// Don't forget to unregister the old and register the new dive!
	// Likewise take care to add/remove the dive from the dive site.
	fulltext_unregister(oldDive);
	dive_site *oldDiveSite = oldDive->dive_site;
	if (oldDiveSite)
		unregister_dive_from_dive_site(oldDive); // the dive-site pointer in the dive is now NULL
	std::swap(*newDive, *oldDive);
	fulltext_register(oldDive);
	if (newDiveSite)
		add_dive_to_dive_site(oldDive, newDiveSite);
	newDiveSite = oldDiveSite; // remember the previous dive site
	invalidate_dive_cache(oldDive);

	// Changing times may have unsorted the dive and trip tables
	QVector<dive *> dives = { oldDive };
	timestamp_t delta = oldDive->when - newDive->when;
	if (delta != 0) {
		sort_dive_table(&dive_table);
		sort_trip_table(&trip_table);
		if (newDive->divetrip != oldDive->divetrip)
			qWarning("Command::EditDive::redo(): This command does not support moving between trips!");
		if (oldDive->divetrip)
			sort_dive_table(&newDive->divetrip->dives); // Keep the trip-table in order
		emit diveListNotifier.divesTimeChanged(delta, dives);
	}

	// Send signals
	emit diveListNotifier.divesChanged(dives, changedFields);

	// Select the changed dives
	setSelection( { oldDive }, oldDive);
}

void EditDive::editDs()
{
	if (siteToEdit) {
		std::swap(siteToEdit->location, dsLocation);
		emit diveListNotifier.diveSiteChanged(siteToEdit, LocationInformationModel::LOCATION); // Inform frontend of changed dive site.
	}
}

bool EditDive::workToBeDone()
{
	// We trust the frontend that an EditDive command is only created if there are changes.
	return true;
}

#endif // SUBSURFACE_MOBILE

} // namespace Command
