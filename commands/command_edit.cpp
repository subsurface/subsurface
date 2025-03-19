// SPDX-License-Identifier: GPL-2.0

#include "command_edit.h"
#include "core/divelist.h"
#include "core/divelog.h"
#include "core/event.h"
#include "core/fulltext.h"
#include "core/qthelper.h"
#include "core/range.h"
#include "core/sample.h"
#include "core/selection.h"
#include "core/subsurface-string.h"
#include "core/tag.h"
#include "core/tanksensormapping.h"
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

template <DiveField::Flags ID, std::string dive::*PTR>
void EditStringSetter<ID, PTR>::set(struct dive *d, QString v) const
{
	d->*PTR = v.toStdString();
}

template <DiveField::Flags ID, std::string dive::*PTR>
QString EditStringSetter<ID, PTR>::data(struct dive *d) const
{
	return QString::fromStdString(d->*PTR);
}

static std::vector<dive *> getDives(bool currentDiveOnly)
{
	if (currentDiveOnly)
		return current_dive ? std::vector<dive *> { current_dive }
				    : std::vector<dive *> { };

	std::vector<dive *> res;
	for (auto &d: divelog.dives) {
		if (d->selected)
			res.push_back(d.get());
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
		d->invalidate_cache(); // Ensure that dive is written in git_save()
	}

	std::swap(old, value);

	// Send signals.
	DiveField id = fieldId();
	emit diveListNotifier.divesChanged(stdToQt<dive *>(dives), id);
	if (!placingCommand())
		setSelection(selectedDives, current, -1);
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
	divelog.dives.fixup_dive(*d);
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
	d->dcs[0].duration.seconds = value;
	d->duration = d->dcs[0].duration;
	d->dcs[0].meandepth = 0_m;
	d->dcs[0].samples.clear();
	fake_dc(&d->dcs[0]);
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
	d->dcs[0].maxdepth.mm = value;
	d->maxdepth = d->dcs[0].maxdepth;
	d->dcs[0].meandepth = 0_m;
	d->dcs[0].samples.clear();
	fake_dc(&d->dcs[0]);
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
	dive_site->add_dive(d);
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

static struct dive_site *createDiveSite(const std::string &name)
{
	struct dive_site *ds = new dive_site;
	if (current_dive && current_dive->dive_site)
		*ds = *current_dive->dive_site;

	// If the current dive has a location, use that as location for the new dive site
	if (current_dive) {
		location_t loc = current_dive->get_gps_location();
		if (has_location(&loc))
			ds->location = loc;
	}

	ds->name = name;
	return ds;
}

EditDiveSiteNew::EditDiveSiteNew(const QString &newName, bool currentDiveOnly) :
	EditDiveSite(createDiveSite(newName.toStdString()), currentDiveOnly),
	diveSiteToAdd(value),
	diveSiteToRemove(nullptr)
{
}

void EditDiveSiteNew::undo()
{
	EditDiveSite::undo();
	auto res = divelog.sites.pull(diveSiteToRemove);
	diveSiteToAdd = std::move(res.ptr);
	emit diveListNotifier.diveSiteDeleted(diveSiteToRemove, res.idx); // Inform frontend of removed dive site.
	diveSiteToRemove = nullptr;
}

void EditDiveSiteNew::redo()
{
	auto res = divelog.sites.register_site(std::move(diveSiteToAdd)); // Return ownership to backend.
	diveSiteToRemove = res.ptr;
	emit diveListNotifier.diveSiteAdded(diveSiteToRemove, res.idx); // Inform frontend of new dive site.
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
	d->get_dc(index)->divemode = (enum divemode_t)i;
	update_setpoint_events(d, d->get_dc(index));
}

int EditMode::data(struct dive *d) const
{
	return d->get_dc(index)->divemode;
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
		d->invalidate_cache(); // Ensure that dive is written in git_save()
	}

	std::swap(tagsToAdd, tagsToRemove);

	// Send signals.
	DiveField id = fieldId();
	emit diveListNotifier.divesChanged(stdToQt<dive *>(dives), id);
	setSelection(selectedDives, current, -1);
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
	for (const divetag *tag: d->tags)
		res.push_back(QString::fromStdString(tag->name));
	return res;
}

void EditTags::set(struct dive *d, const QStringList &v) const
{
	d->tags.clear();
	for (const QString &tag: v)
		taglist_add_tag(d->tags, tag.toStdString());
	taglist_cleanup(d->tags);
}

QString EditTags::fieldName() const
{
	return Command::Base::tr("tags");
}

// ***** Buddies *****
QStringList EditBuddies::data(struct dive *d) const
{
	return stringToList(QString::fromStdString(d->buddy));
}

void EditBuddies::set(struct dive *d, const QStringList &v) const
{
	QString text = v.join(", ");
	d->buddy = text.toStdString();
}

QString EditBuddies::fieldName() const
{
	return Command::Base::tr("buddies");
}

// ***** DiveGuide *****
QStringList EditDiveGuide::data(struct dive *d) const
{
	return stringToList(QString::fromStdString(d->diveguide));
}

void EditDiveGuide::set(struct dive *d, const QStringList &v) const
{
	QString text = v.join(", ");
	d->diveguide = text.toStdString();
}

QString EditDiveGuide::fieldName() const
{
	return Command::Base::tr("dive guide");
}

template <typename T>
ptrdiff_t comp_ptr(T *v1, T *v2)
{
	return v1 - v2;
}

PasteState::PasteState(dive &d, const dive_paste_data &data, std::vector<dive_site *> &dive_sites_changed) : d(d)
{
	notes = data.notes;
	diveguide = data.diveguide;
	buddy = data.buddy;
	suit = data.suit;
	rating = data.rating;
	visibility = data.visibility;
	wavesize = data.wavesize;
	current = data.current;
	surge = data.surge;
	chill = data.chill;
	if (data.divesite.has_value()) {
		if (data.divesite) {
			// In the undo system, we can turn the uuid into a pointer,
			// because everything is serialized.
			dive_site *ds = divelog.sites.get_by_uuid(*data.divesite);
			if (ds)
				divesite = ds;
		} else {
			divesite = nullptr;
		}
	}
	if (divesite.has_value() && *divesite != d.dive_site) {
		if (d.dive_site)
			range_insert_sorted_unique(dive_sites_changed, d.dive_site, comp_ptr<dive_site>); // Use <=> once on C++20
		if (*divesite)
			range_insert_sorted_unique(dive_sites_changed, *divesite, comp_ptr<dive_site>); // Use <=> once on C++20
	}
	tags = data.tags;
	if (data.cylinders.has_value()) {
		cylinders = data.cylinders;
		// Paste cylinders is "special":
		// 1) For cylinders that exist in the destination dive we keep the gas-mix and pressures.
		// 2) For cylinders that do not yet exist in the destination dive, we set the pressures to 0, i.e. unset.
		//    Moreover, for these we set the manually_added flag, because they weren't downloaded from a DC.
		for (size_t i = 0; i < data.cylinders->size() && i < cylinders->size(); ++i) {
			const cylinder_t &src = (*data.cylinders)[i];
			cylinder_t &dst = (*cylinders)[i];
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
		for (size_t i = data.cylinders->size(); i < cylinders->size(); ++i) {
			cylinder_t &cyl = (*cylinders)[i];
			cyl.start = 0_bar;
			cyl.end = 0_bar;
			cyl.sample_start = 0_bar;
			cyl.sample_end = 0_bar;
			cyl.manually_added = true;
		}
	}
	weightsystems = data.weights;
	number = data.number;
	when = data.when;
}

PasteState::~PasteState()
{
}

void PasteState::swap()
{
	if (notes.has_value())
		std::swap(*notes, d.notes);
	if (diveguide.has_value())
		std::swap(*diveguide, d.diveguide);
	if (buddy.has_value())
		std::swap(*buddy, d.buddy);
	if (suit.has_value())
		std::swap(*suit, d.suit);
	if (rating.has_value())
		std::swap(*rating, d.rating);
	if (visibility.has_value())
		std::swap(*visibility, d.visibility);
	if (wavesize.has_value())
		std::swap(*wavesize, d.wavesize);
	if (current.has_value())
		std::swap(*current, d.current);
	if (surge.has_value())
		std::swap(*surge, d.surge);
	if (chill.has_value())
		std::swap(*chill, d.chill);
	if (divesite.has_value() && *divesite != d.dive_site) {
		auto old_ds = unregister_dive_from_dive_site(&d);
		if (*divesite)
			(*divesite)->add_dive(&d);
		divesite = old_ds;
	}
	if (tags.has_value())
		std::swap(*tags, d.tags);
	if (cylinders.has_value())
		std::swap(*cylinders, d.cylinders);
	if (weightsystems.has_value())
		std::swap(*weightsystems, d.weightsystems);
	if (number.has_value())
		std::swap(*number, d.number);
	if (when.has_value())
		std::swap(*when, d.when);
}

// ***** Paste *****
PasteDives::PasteDives(const dive_paste_data &data)
{
	std::vector<dive *> selection = getDiveSelection();
	dives.reserve(selection.size());
	for (dive *d: selection)
		dives.emplace_back(*d, data, dive_sites_changed);

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
		divesToNotify.push_back(&state.d);
		state.swap();
		state.d.invalidate_cache(); // Ensure that dive is written in git_save()
	}

	// Send signals. We use the first data item to determine what changed
	DiveField fields(DiveField::NONE);
	const PasteState &state = dives[0];
	fields.notes = state.notes.has_value();
	fields.diveguide = state.diveguide.has_value();
	fields.buddy = state.buddy.has_value();
	fields.suit = state.suit.has_value();
	fields.rating = state.rating.has_value();
	fields.visibility = state.visibility.has_value();
	fields.wavesize = state.wavesize.has_value();
	fields.current = state.current.has_value();
	fields.surge = state.surge.has_value();
	fields.chill = state.chill.has_value();
	fields.divesite = !dive_sites_changed.empty();
	fields.tags = state.tags.has_value();
	fields.datetime = state.when.has_value();
	fields.nr = state.number.has_value();
	emit diveListNotifier.divesChanged(divesToNotify, fields);
	if (state.cylinders.has_value())
		emit diveListNotifier.cylindersReset(divesToNotify);
	if (state.weightsystems.has_value())
		emit diveListNotifier.weightsystemsReset(divesToNotify);
	for (dive_site *ds: dive_sites_changed)
		emit diveListNotifier.diveSiteDivesChanged(ds);
}

// Redo and undo do the same
void PasteDives::redo()
{
	undo();
}

// ***** ReplanDive *****
ReplanDive::ReplanDive(dive *source) : d(current_dive),
	when(0),
	salinity(0)
{
	if (!d)
		return;

	// Fix source. Things might be inconsistent after modifying the profile.
	source->maxdepth = source->dcs[0].maxdepth = 0_m;
	divelog.dives.fixup_dive(*source);

	when = source->when;
	maxdepth = source->maxdepth;
	meandepth = source->meandepth;
	notes = source->notes;
	duration = source->duration;
	salinity = source->salinity;
	surface_pressure = source->surface_pressure;

	// This resets the dive computers and cylinders of the source dive, avoiding deep copies.
	std::swap(source->cylinders, cylinders);
	std::swap(source->dcs[0], dc);

	setText(Command::Base::tr("Replan dive"));
}

ReplanDive::~ReplanDive()
{
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
	std::swap(d->dcs[0], dc);
	std::swap(d->notes, notes);
	std::swap(d->surface_pressure, surface_pressure);
	std::swap(d->duration, duration);
	std::swap(d->salinity, salinity);
	divelog.dives.fixup_dive(*d);
	d->invalidate_cache(); // Ensure that dive is written in git_save()

	QVector<dive *> divesToNotify = { d };
	// Note that we have to emit cylindersReset before divesChanged, because the divesChanged
	// updates the profile, which is out-of-sync and gets confused.
	emit diveListNotifier.cylindersReset(divesToNotify);
	emit diveListNotifier.divesChanged(divesToNotify, DiveField::DATETIME | DiveField::DURATION | DiveField::DEPTH | DiveField::MODE |
							  DiveField::NOTES | DiveField::SALINITY | DiveField::ATM_PRESS);
	if (!placingCommand())
		setSelection({ d }, d, -1);
}

// Redo and undo do the same
void ReplanDive::redo()
{
	undo();
}

// ***** EditProfile *****
QString editProfileTypeToString(EditProfileType type, int count)
{
	switch (type) {
		default:
		case EditProfileType::ADD: return Command::Base::tr("Add stop");
		case EditProfileType::REMOVE: return Command::Base::tr("Remove %n stop(s)", "", count);
		case EditProfileType::MOVE: return Command::Base::tr("Move %n stop(s)", "", count);
		case EditProfileType::EDIT: return Command::Base::tr("Edit stop");
	}
}

EditProfile::EditProfile(const dive *source, int dcNr, EditProfileType type, int count) : d(current_dive),
	dcNr(dcNr)
{
	const struct divecomputer *sdc = source->get_dc(dcNr);
	if (!sdc)
		d = nullptr; // Signal that we refuse to do anything.
	if (!d)
		return;

	maxdepth = source->maxdepth;
	dcmaxdepth = sdc->maxdepth;
	meandepth = source->meandepth;
	duration = source->duration;

	dc.samples = sdc->samples;
	dc.events = sdc->events;

	setText(editProfileTypeToString(type, count) + " " + diveNumberOrDate(d));
}

EditProfile::~EditProfile()
{
}

bool EditProfile::workToBeDone()
{
	return !!d;
}

void EditProfile::undo()
{
	struct divecomputer *sdc = d->get_dc(dcNr);
	if (!sdc)
		return;
	std::swap(sdc->samples, dc.samples);
	std::swap(sdc->events, dc.events);
	std::swap(sdc->maxdepth, dc.maxdepth);
	std::swap(d->maxdepth, maxdepth);
	std::swap(d->meandepth, meandepth);
	std::swap(d->duration, duration);
	divelog.dives.fixup_dive(*d);
	d->invalidate_cache(); // Ensure that dive is written in git_save()

	QVector<dive *> divesToNotify = { d };
	emit diveListNotifier.divesChanged(divesToNotify, DiveField::DURATION | DiveField::DEPTH);
	if (!placingCommand())
		setSelection({ d }, d, dcNr);
}

// Redo and undo do the same
void EditProfile::redo()
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
		if (d->weightsystems.empty())
			continue;
		d->weightsystems.pop_back();
		emit diveListNotifier.weightRemoved(d, d->weightsystems.size());
		d->invalidate_cache(); // Ensure that dive is written in git_save()
	}
}

void AddWeight::redo()
{
	for (dive *d: dives) {
		d->weightsystems.emplace_back();
		emit diveListNotifier.weightAdded(d, d->weightsystems.size() - 1);
		d->invalidate_cache(); // Ensure that dive is written in git_save()
	}
}

static int find_weightsystem_index(const struct dive *d, const weightsystem_t &ws)
{
	return index_of_if(d->weightsystems, [&ws](auto &ws2) { return ws == ws2; });
}

EditWeightBase::EditWeightBase(int index, bool currentDiveOnly) :
	EditDivesBase(currentDiveOnly)
{
	// Get the old weightsystem, bail if index is invalid
	if (!current || index < 0 || static_cast<size_t>(index) >= current->weightsystems.size()) {
		dives.clear();
		return;
	}
	ws = current->weightsystems[index];

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
		dives[i]->weightsystems.add(indices[i], ws);
		emit diveListNotifier.weightAdded(dives[i], indices[i]);
		dives[i]->invalidate_cache(); // Ensure that dive is written in git_save()
	}
}

void RemoveWeight::redo()
{
	for (size_t i = 0; i < dives.size(); ++i) {
		dives[i]->weightsystems.remove(indices[i]);
		emit diveListNotifier.weightRemoved(dives[i], indices[i]);
		dives[i]->invalidate_cache(); // Ensure that dive is written in git_save()
	}
}

// ***** Edit Weight *****
EditWeight::EditWeight(int index, weightsystem_t wsIn, bool currentDiveOnly) :
	EditWeightBase(index, currentDiveOnly)
{
	if (dives.empty())
		return;

	size_t num_dives = dives.size();
	if (num_dives == 1)
		setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("Edit weight")).arg(diveNumberOrDate(dives[0])));
	else
		setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("Edit weight (%n dive(s))", "", num_dives)).arg(getListOfDives(dives)));

	// Try to untranslate the weightsystem name
	new_ws = std::move(wsIn);
	QString vString = QString::fromStdString(new_ws.description);
	auto it = std::find_if(ws_info_table.begin(), ws_info_table.end(),
			       [&vString](const ws_info &info)
			       { return gettextFromC::tr(info.name.c_str()) == vString; });
	if (it != ws_info_table.end())
		new_ws.description = it->name;

	// If that doesn't change anything, do nothing
	if (ws == new_ws) {
		dives.clear();
		return;
	}
}

EditWeight::~EditWeight()
{
}

void EditWeight::redo()
{
	for (size_t i = 0; i < dives.size(); ++i) {
		add_weightsystem_description(new_ws); // This updates the weightsystem info table
		dives[i]->weightsystems.set(indices[i], new_ws);
		emit diveListNotifier.weightEdited(dives[i], indices[i]);
		dives[i]->invalidate_cache(); // Ensure that dive is written in git_save()
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
	EditDivesBase(currentDiveOnly)
{
	if (dives.empty())
		return;
	else if (dives.size() == 1)
		setText(Command::Base::tr("Add cylinder"));
	else
		setText(Command::Base::tr("Add cylinder (%n dive(s))", "", dives.size()));
	cyl = create_new_manual_cylinder(dives[0]);
	indexes.reserve(dives.size());
}

AddCylinder::~AddCylinder()
{
}

bool AddCylinder::workToBeDone()
{
	return true;
}

void AddCylinder::undo()
{
	for (size_t i = 0; i < dives.size(); ++i) {
		remove_cylinder(dives[i], indexes[i]);
		divelog.dives.update_cylinder_related_info(*dives[i]);
		emit diveListNotifier.cylinderRemoved(dives[i], indexes[i]);
		dives[i]->invalidate_cache(); // Ensure that dive is written in git_save()
	}
}

void AddCylinder::redo()
{
	indexes.clear();
	for (dive *d: dives) {
		int index = first_hidden_cylinder(d);
		indexes.push_back(index);
		d->cylinders.add(index, cyl);
		divelog.dives.update_cylinder_related_info(*d);
		emit diveListNotifier.cylinderAdded(d, index);
		d->invalidate_cache(); // Ensure that dive is written in git_save()
	}
}

static bool same_cylinder_type(const cylinder_t &cyl1, const cylinder_t &cyl2)
{
	return std::tie(cyl1.cylinder_use, cyl1.type.description) ==
	       std::tie(cyl2.cylinder_use, cyl2.type.description);
}

static bool same_cylinder_size(const cylinder_t &cyl1, const cylinder_t &cyl2)
{
	return std::tie(cyl1.type.size.mliter, cyl1.type.workingpressure.mbar) ==
	       std::tie(cyl2.type.size.mliter, cyl2.type.workingpressure.mbar);
}

// Flags for comparing cylinders
static constexpr int SAME_TYPE = 1 << 0;
static constexpr int SAME_SIZE = 1 << 1;
static constexpr int SAME_PRESS = 1 << 2;
static constexpr int SAME_GAS = 1 << 3;

EditCylinderBase::EditCylinderBase(int index, bool currentDiveOnly, bool nonProtectedOnly, int sameCylinderFlags) :
	EditDivesBase(currentDiveOnly)
{
	// Get the old cylinder, bail if index is invalid
	if (!current || index < 0 || index >= static_cast<int>(current->cylinders.size())) {
		dives.clear();
		return;
	}
	const cylinder_t &orig = *current->get_cylinder(index);

	std::vector<dive *> divesNew;
	divesNew.reserve(dives.size());
	indexes.reserve(dives.size());
	cyl.reserve(dives.size());

	for (dive *d: dives) {
		if (index >= static_cast<int>(d->cylinders.size()))
			continue;
		if (nonProtectedOnly && d->is_cylinder_prot(index))
			continue;
		// We checked that the cylinder exists above.
		const cylinder_t &cylinder = d->cylinders[index];
		if (d != current &&
		    (!same_cylinder_size(orig, cylinder) || !same_cylinder_type(orig, cylinder))) {
			// when editing cylinders, we assume that the user wanted to edit the 'n-th' cylinder
			// and we only do edit that cylinder, if it was the same type as the one in the current dive
			continue;
		}

		divesNew.push_back(d);
		// that's silly as it's always the same value - but we need this vector of indices in the case where we add
		// a cylinder to several dives as the spot will potentially be different in different dives
		indexes.push_back(index);
		cyl.push_back(cylinder);
	}
	dives = std::move(divesNew);
}

EditCylinderBase::~EditCylinderBase()
{
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
		std::vector<int> mapping = get_cylinder_map_for_add(dives[i]->cylinders.size(), indexes[i]);
		dives[i]->cylinders.add(indexes[i], cyl[i]);
		cylinder_renumber(*dives[i], &mapping[0]);
		divelog.dives.update_cylinder_related_info(*dives[i]);
		emit diveListNotifier.cylinderAdded(dives[i], indexes[i]);
		dives[i]->invalidate_cache(); // Ensure that dive is written in git_save()
	}
}

void RemoveCylinder::redo()
{
	for (size_t i = 0; i < dives.size(); ++i) {
		std::vector<int> mapping = get_cylinder_map_for_remove(dives[i]->cylinders.size(), indexes[i]);
		remove_cylinder(dives[i], indexes[i]);
		cylinder_renumber(*dives[i], &mapping[0]);
		divelog.dives.update_cylinder_related_info(*dives[i]);
		emit diveListNotifier.cylinderRemoved(dives[i], indexes[i]);
		dives[i]->invalidate_cache(); // Ensure that dive is written in git_save()
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

	// The base class copied the cylinders for us, let's edit them
	for (int i = 0; i < (int)cyl.size(); ++i) {
		switch (type) {
		case EditCylinderType::TYPE:
			cyl[i].type = cylIn.type;
			cyl[i].type.description = cylIn.type.description;
			cyl[i].cylinder_use = cylIn.cylinder_use;
			break;
		case EditCylinderType::PRESSURE:
			cyl[i].start = cylIn.start;
			cyl[i].end = cylIn.end;
			break;
		case EditCylinderType::GASMIX:
			cyl[i].gasmix = cylIn.gasmix;
			cyl[i].bestmix_o2 = cylIn.bestmix_o2;
			cyl[i].bestmix_he = cylIn.bestmix_he;
			sanitize_gasmix(cyl[i].gasmix);
			break;
		}
	}
}

void EditCylinder::redo()
{
	for (size_t i = 0; i < dives.size(); ++i) {
		const std::string &name = cyl[i].type.description;
		set_tank_info_data(tank_info_table, name, cyl[i].type.size, cyl[i].type.workingpressure);
		std::swap(*dives[i]->get_cylinder(indexes[i]), cyl[i]);
		divelog.dives.update_cylinder_related_info(*dives[i]);
		emit diveListNotifier.cylinderEdited(dives[i], indexes[i]);
		dives[i]->invalidate_cache(); // Ensure that dive is written in git_save()
	}
}

// Undo and redo do the same as just the stored value is exchanged
void EditCylinder::undo()
{
	redo();
}

EditSensors::EditSensors(unsigned int cylinderIndexIn, int16_t sensorIdIn, int dcNr)
	: d(current_dive), dc(d->get_dc(dcNr)), cylinderIndex(cylinderIndexIn), sensorId(sensorIdIn), oldTankSensorMappings(dc->tank_sensor_mappings)
{
	if (!d || !dc)
		return;

	setText(Command::Base::tr("Edit sensors"));
}

void EditSensors::undo()
{
	dc->tank_sensor_mappings = oldTankSensorMappings;

	emit diveListNotifier.diveComputerEdited(*d, *dc);
	d->invalidate_cache(); // Ensure that dive is written in git_save()
}

void EditSensors::redo()
{
	bool found = false;
	for (auto it = dc->tank_sensor_mappings.begin(); it != dc->tank_sensor_mappings.end();) {
		if (sensorId == NO_SENSOR) {
			if ((*it).cylinder_index == cylinderIndex) {
				it = dc->tank_sensor_mappings.erase(it);

				break;
			}
		} else if ((*it).sensor_id == sensorId) {
			if (!found) {
				(*it).cylinder_index = cylinderIndex;
				found = true;
			} else {
				it = dc->tank_sensor_mappings.erase(it);

				continue;
			}
		} else if ((*it).cylinder_index == cylinderIndex) {
			if (!found) {
				(*it).sensor_id = sensorId;
				found = true;
			} else {
				it = dc->tank_sensor_mappings.erase(it);

				continue;
			}
		}

		it++;
	}

	if (!found) {
		dc->tank_sensor_mappings.push_back(tank_sensor_mapping { sensorId, cylinderIndex });
	}

	emit diveListNotifier.diveComputerEdited(*d, *dc);
	d->invalidate_cache(); // Ensure that dive is written in git_save()
}

bool EditSensors::workToBeDone()
{
	return d && dc;
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
	if (oldDive->diveguide != newDive->diveguide)
		changedFields |= DiveField::DIVEGUIDE;
	if (oldDive->buddy != newDive->buddy)
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
	if (oldDive->suit != newDive->suit)
		changedFields |= DiveField::SUIT;
	if (taglist_get_tagstring(oldDive->tags) != taglist_get_tagstring(newDive->tags)) // This is cheating. Do we have a taglist comparison function?
		changedFields |= DiveField::TAGS;
	if (oldDive->dcs[0].divemode != newDive->dcs[0].divemode)
		changedFields |= DiveField::MODE;
	if (oldDive->notes != newDive->notes)
		changedFields |= DiveField::NOTES;
	if (oldDive->salinity != newDive->salinity)
		changedFields |= DiveField::SALINITY;

	newDive->dive_site = nullptr; // We will add the dive to the site manually and therefore saved the dive site.
}

void EditDive::undo()
{
	if (siteToRemove) {
		auto res = divelog.sites.pull(siteToRemove);
		siteToAdd = std::move(res.ptr);
		emit diveListNotifier.diveSiteDeleted(siteToRemove, res.idx); // Inform frontend of removed dive site.
	}

	exchangeDives();
	editDs();
}

void EditDive::redo()
{
	if (siteToAdd) {
		auto res = divelog.sites.register_site(std::move(siteToAdd)); // Return ownership to backend.
		siteToRemove = res.ptr;
		emit diveListNotifier.diveSiteAdded(siteToRemove, res.idx); // Inform frontend of new dive site.
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
		newDiveSite->add_dive(oldDive);
	newDiveSite = oldDiveSite; // remember the previous dive site
	oldDive->invalidate_cache();

	// Changing times may have unsorted the dive and trip tables
	QVector<dive *> dives = { oldDive };
	timestamp_t delta = oldDive->when - newDive->when;
	if (delta != 0) {
		divelog.dives.sort();
		divelog.trips.sort();
		if (newDive->divetrip != oldDive->divetrip)
			qWarning("Command::EditDive::redo(): This command does not support moving between trips!");
		if (oldDive->divetrip)
			newDive->divetrip->sort_dives(); // Keep the trip-table in order
		emit diveListNotifier.divesTimeChanged(delta, dives);
	}

	// Send signals
	emit diveListNotifier.divesChanged(dives, changedFields);

	// Select the changed dives
	setSelection( { oldDive }, oldDive, -1);
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
