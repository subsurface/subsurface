// SPDX-License-Identifier: GPL-2.0

#include "command_edit.h"
#include "core/divelist.h"
#include "core/qthelper.h"
#include "desktop-widgets/mapwidget.h" // TODO: Replace desktop-dependency by signal

namespace Command {

template<typename T>
EditBase<T>::EditBase(const QVector<dive *> &divesIn, T newValue, T oldValue) :
	dives(divesIn.toStdVector()),
	value(std::move(newValue)),
	old(std::move(oldValue))
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
template
EditBase<int>::EditBase(const QVector<dive *> &dives, int oldValue, int newValue);
template
EditBase<struct dive_site *>::EditBase(const QVector<dive *> &dives, struct dive_site *oldValue, struct dive_site *newValue);

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
	d->dive_site = dive_site;
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
	bool diveSiteListChanged = false;

	// If we had taken ownership of a dive site, readd it to the system
	if (ownedDiveSite) {
		register_dive_site(ownedDiveSite.release());
		diveSiteListChanged = true;
	}

	// Call undo of base class
	EditBase<struct dive_site *>::undo();

	// If the old dive-site has no users anymore, remove it from the core,
	// but keep an owning copy. Thus if this undo command is freed, the
	// dive-site will be automatically deleted and on redo() it can be
	// readded to the system
	if (value && !is_dive_site_used(value, false)) {
		unregister_dive_site(value);
		ownedDiveSite.reset(value);
		diveSiteListChanged = true;
	}

	if (diveSiteListChanged)
		MapWidget::instance()->reload();
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

// ***** Tag based commands *****
EditTagsBase::EditTagsBase(const QVector<dive *> &divesIn, const QStringList &newListIn, struct dive *d):
	dives(divesIn.toStdVector()),
	newList(newListIn),
	oldDive(d)
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

	// Calculate tags to add and tags to remove
	QStringList oldList = data(oldDive);
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
		//: remove the part in parantheses for %n = 1
		setText(tr("Edit %1 (%n dive(s))", "", num_dives).arg(fieldName()));

	return num_dives;
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
	}

	std::swap(tagsToAdd, tagsToRemove);

	emit diveListNotifier.divesEdited(QVector<dive *>::fromStdVector(dives), fieldId());

	mark_divelist_changed(true);
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
	for (QString &s: res)
		s = s.trimmed();
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

} // namespace Command
