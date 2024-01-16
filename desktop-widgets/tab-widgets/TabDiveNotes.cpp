// SPDX-License-Identifier: GPL-2.0
#include "TabDiveNotes.h"
#include "maintab.h"
#include "core/divesite.h"
#include "core/qthelper.h"
#include "core/selection.h"
#include "core/subsurface-string.h"
#include "core/trip.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/mapwidget.h"
#include "desktop-widgets/simplewidgets.h"
#include "qt-models/diveplannermodel.h"
#include "commands/command.h"

#include <QCompleter>
#include <QMessageBox>

struct Completers {
	QCompleter *diveguide;
	QCompleter *buddy;
	QCompleter *tags;
};

TabDiveNotes::TabDiveNotes(MainTab *parent) : TabBase(parent),
	ignoreInput(false),
	currentTrip(0)
{
	ui.setupUi(this);

	updateDateTimeFields();

	connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &TabDiveNotes::divesChanged);
	connect(&diveListNotifier, &DiveListNotifier::tripChanged, this, &TabDiveNotes::tripChanged);
	connect(&diveListNotifier, &DiveListNotifier::diveSiteChanged, this, &TabDiveNotes::diveSiteEdited);
	connect(&diveListNotifier, &DiveListNotifier::commandExecuted, this, &TabDiveNotes::closeWarning);

	connect(ui.editDiveSiteButton, &QToolButton::clicked, MainWindow::instance(), &MainWindow::startDiveSiteEdit);
#ifdef MAP_SUPPORT
	connect(ui.location, &DiveLocationLineEdit::entered, MapWidget::instance(), &MapWidget::centerOnIndex);
	connect(ui.location, &DiveLocationLineEdit::currentChanged, MapWidget::instance(), &MapWidget::centerOnIndex);
#endif
	connect(ui.location, &DiveLocationLineEdit::editingFinished, this, &TabDiveNotes::on_location_diveSiteSelected);

	// One might think that we could listen to the precise property-changed signals of the preferences system.
	// Alas, this is not the case. When the user switches to system-format, the preferences sends the according
	// signal. However, the correct date and time format is set by the preferences dialog later. This should be fixed.
	connect(&diveListNotifier, &DiveListNotifier::settingsChanged, this, &TabDiveNotes::updateDateTimeFields);

	QAction *action = new QAction(tr("OK"), this);
	connect(action, &QAction::triggered, this, &TabDiveNotes::closeWarning);
	ui.multiDiveWarningMessage->addAction(action);

	action = new QAction(tr("Undo"), this);
	connect(action, &QAction::triggered, Command::undoAction(this), &QAction::trigger);
	connect(action, &QAction::triggered, this, &TabDiveNotes::closeWarning);
	ui.multiDiveWarningMessage->addAction(action);

	Completers completers;
	completers.buddy = new QCompleter(&buddyModel, ui.buddy);
	completers.diveguide = new QCompleter(&diveGuideModel, ui.diveguide);
	completers.tags = new QCompleter(&tagModel, ui.tagWidget);
	completers.buddy->setCaseSensitivity(Qt::CaseInsensitive);
	completers.diveguide->setCaseSensitivity(Qt::CaseInsensitive);
	completers.tags->setCaseSensitivity(Qt::CaseInsensitive);
	ui.buddy->setCompleter(completers.buddy);
	ui.diveguide->setCompleter(completers.diveguide);
	ui.tagWidget->setCompleter(completers.tags);
	ui.multiDiveWarningMessage->hide();
	ui.depth->hide();
	ui.depthLabel->hide();
	ui.duration->hide();
	ui.durationLabel->hide();
	setMinimumHeight(0);
	setMinimumWidth(0);

	connect(ui.multiDiveWarningMessage, &KMessageWidget::showAnimationFinished,
					ui.location, &DiveLocationLineEdit::fixPopupPosition);

	// enable URL clickability in notes:
	new TextHyperlinkEventFilter(ui.notes); //destroyed when ui.notes is destroyed

	ui.diveTripLocation->hide();
}

void TabDiveNotes::updateDateTimeFields()
{
	ui.dateEdit->setDisplayFormat(prefs.date_format);
	ui.timeEdit->setDisplayFormat(prefs.time_format);
}

void TabDiveNotes::closeWarning()
{
	ui.multiDiveWarningMessage->hide();
}

// This function gets called if a field gets updated by an undo command.
// Refresh the corresponding UI field.
void TabDiveNotes::divesChanged(const QVector<dive *> &dives, DiveField field)
{
	// If the current dive is not in list of changed dives, do nothing
	if (!parent.includesCurrentDive(dives))
		return;

	dive *currentDive = parent.currentDive;
	if (field.duration)
		ui.duration->setText(render_seconds_to_string(currentDive->duration.seconds));
	if (field.depth)
		ui.depth->setText(get_depth_string(currentDive->maxdepth, true));
	if (field.rating)
		ui.rating->setCurrentStars(currentDive->rating);
	if (field.notes)
		updateNotes(currentDive);
	if (field.datetime) {
		updateDateTime(currentDive);
		DivePlannerPointsModel::instance()->getDiveplan().when = currentDive->when;
	}
	if (field.divesite)
		updateDiveSite(currentDive);
	if (field.tags)
		ui.tagWidget->setText(get_taglist_string(currentDive->tag_list));
	if (field.buddy)
		ui.buddy->setText(currentDive->buddy);
	if (field.diveguide)
		ui.diveguide->setText(currentDive->diveguide);
}

void TabDiveNotes::diveSiteEdited(dive_site *ds, int)
{
	if (parent.currentDive && parent.currentDive->dive_site == ds)
		updateDiveSite(parent.currentDive);
}

// This function gets called if a trip-field gets updated by an undo command.
// Refresh the corresponding UI field.
void TabDiveNotes::tripChanged(dive_trip *trip, TripField field)
{
	// If the current dive is not in list of changed dives, do nothing
	if (currentTrip != trip)
		return;

	if (field.notes)
		ui.notes->setText(currentTrip->notes);
	if (field.location)
		ui.diveTripLocation->setText(currentTrip->location);
}

static bool isHtml(const QString &s)
{
	return s.contains("<div", Qt::CaseInsensitive) || s.contains("<table", Qt::CaseInsensitive);
}

void TabDiveNotes::updateNotes(const struct dive *d)
{
	QString tmp(d->notes);
	if (isHtml(tmp)) {
		ui.notes->setHtml(tmp);
	} else {
		ui.notes->setPlainText(tmp);
	}
}

void TabDiveNotes::updateDateTime(const struct dive *d)
{
	QDateTime localTime = timestampToDateTime(d->when);
	ui.dateEdit->setDate(localTime.date());
	ui.timeEdit->setTime(localTime.time());
}

void TabDiveNotes::updateTripDate(const struct dive_trip *t)
{
	QDateTime localTime = timestampToDateTime(trip_date(t));
	ui.dateEdit->setDate(localTime.date());
}

void TabDiveNotes::updateDiveSite(struct dive *d)
{
	struct dive_site *ds = d->dive_site;
	ui.location->setCurrentDiveSite(d);
	if (ds) {
		ui.locationTags->setText(constructLocationTags(&ds->taxonomy, true));

		if (ui.locationTags->text().isEmpty() && has_location(&ds->location))
			ui.locationTags->setText(printGPSCoords(&ds->location));
		ui.editDiveSiteButton->setEnabled(true);
	} else {
		ui.locationTags->clear();
		ui.editDiveSiteButton->setEnabled(false);
	}

	if (ui.locationTags->text().isEmpty())
		ui.locationTags->hide();
	else
		ui.locationTags->show();
}

void TabDiveNotes::updateData(const std::vector<dive *> &, dive *currentDive, int currentDC)
{
	ui.location->refreshDiveSiteCache();

	ignoreInput = true; // don't trigger on changes to the widgets

	currentTrip = single_selected_trip();
	if (currentTrip) {
		// only use trip relevant fields
		ui.diveguide->setVisible(false);
		ui.DiveguideLabel->setVisible(false);
		ui.buddy->setVisible(false);
		ui.BuddyLabel->setVisible(false);
		ui.rating->setVisible(false);
		ui.RatingLabel->setVisible(false);
		ui.tagWidget->setVisible(false);
		ui.TagLabel->setVisible(false);
		ui.dateEdit->setReadOnly(true);
		ui.timeLabel->setVisible(false);
		ui.timeEdit->setVisible(false);
		ui.diveTripLocation->show();
		ui.location->hide();
		ui.locationPopupButton->hide();
		ui.editDiveSiteButton->hide();
		// rename the remaining fields and fill data from selected trip
		ui.LocationLabel->setText(tr("Trip location"));
		ui.diveTripLocation->setText(currentTrip->location);
		updateTripDate(currentTrip);
		ui.locationTags->clear();
		//TODO: Fix this.
		//ui.location->setText(currentTrip->location);
		ui.NotesLabel->setText(tr("Trip notes"));
		ui.notes->setText(currentTrip->notes);
		ui.depth->setVisible(false);
		ui.depthLabel->setVisible(false);
		ui.duration->setVisible(false);
		ui.durationLabel->setVisible(false);
	} else {
		// make all the fields visible writeable
		ui.diveTripLocation->hide();
		ui.location->show();
		ui.locationPopupButton->show();
		ui.editDiveSiteButton->show();
		ui.diveguide->setVisible(true);
		ui.buddy->setVisible(true);
		ui.rating->setVisible(true);
		ui.RatingLabel->setVisible(true);
		ui.BuddyLabel->setVisible(true);
		ui.DiveguideLabel->setVisible(true);
		ui.TagLabel->setVisible(true);
		ui.tagWidget->setVisible(true);
		ui.dateEdit->setReadOnly(false);
		ui.timeLabel->setVisible(true);
		ui.timeEdit->setVisible(true);
		/* and fill them from the dive */
		ui.rating->setCurrentStars(currentDive->rating);
		// reset labels in case we last displayed trip notes
		ui.LocationLabel->setText(tr("Location"));
		ui.NotesLabel->setText(tr("Notes"));
		ui.tagWidget->setText(get_taglist_string(currentDive->tag_list));
		bool isManual = is_manually_added_dc(&currentDive->dc);
		ui.depth->setVisible(isManual);
		ui.depthLabel->setVisible(isManual);
		ui.duration->setVisible(isManual);
		ui.durationLabel->setVisible(isManual);

		updateNotes(currentDive);
		updateDiveSite(currentDive);
		updateDateTime(currentDive);
		ui.diveguide->setText(currentDive->diveguide);
		ui.buddy->setText(currentDive->buddy);
	}
	ui.duration->setText(render_seconds_to_string(currentDive->duration.seconds));
	ui.depth->setText(get_depth_string(currentDive->maxdepth, true));

	ui.editDiveSiteButton->setEnabled(!ui.location->text().isEmpty());
	/* unset the special value text for date and time, just in case someone dove at midnight */
	ui.dateEdit->setSpecialValueText(QString());
	ui.timeEdit->setSpecialValueText(QString());

	ignoreInput = false;
}

void TabDiveNotes::clear()
{
	ui.rating->setCurrentStars(0);
	ui.location->clear();
	ui.diveguide->clear();
	ui.buddy->clear();
	ui.notes->clear();
	/* set date and time to minimums which triggers showing the special value text */
	ui.dateEdit->setSpecialValueText(QString("-"));
	ui.dateEdit->setMinimumDate(QDate(1, 1, 1));
	ui.dateEdit->setDate(QDate(1, 1, 1));
	ui.timeEdit->setSpecialValueText(QString("-"));
	ui.timeEdit->setMinimumTime(QTime(0, 0, 0, 0));
	ui.timeEdit->setTime(QTime(0, 0, 0, 0));
	ui.tagWidget->clear();
}

void TabDiveNotes::divesEdited(int i)
{
	// No warning if only one dive was edited
	if (i <= 1)
		return;
	ui.multiDiveWarningMessage->setCloseButtonVisible(false);
	ui.multiDiveWarningMessage->setText(tr("Warning: edited %1 dives").arg(i));
	ui.multiDiveWarningMessage->show();
}

void TabDiveNotes::on_buddy_editingFinished()
{
	if (ignoreInput || !parent.currentDive)
		return;

	divesEdited(Command::editBuddies(stringToList(ui.buddy->toPlainText()), false));
}

void TabDiveNotes::on_diveguide_editingFinished()
{
	if (ignoreInput || !parent.currentDive)
		return;

	divesEdited(Command::editDiveGuide(stringToList(ui.diveguide->toPlainText()), false));
}

void TabDiveNotes::on_duration_editingFinished()
{
	if (ignoreInput || !parent.currentDive)
		return;

	// Duration editing is special: we only edit the current dive.
	divesEdited(Command::editDuration(parseDurationToSeconds(ui.duration->text()), true));
}

void TabDiveNotes::on_depth_editingFinished()
{
	if (ignoreInput || !parent.currentDive)
		return;

	// Depth editing is special: we only edit the current dive.
	divesEdited(Command::editDepth(parseLengthToMm(ui.depth->text()), true));
}

// Editing of the dive time is different. If multiple dives are edited,
// all dives are shifted by an offset.
static void shiftTime(QDateTime &dateTime, dive *currentDive)
{
	timestamp_t when = dateTimeToTimestamp(dateTime);
	if (currentDive->when != when) {
		timestamp_t offset = when - currentDive->when;
		Command::shiftTime(getDiveSelection(), (int)offset);
	}
}

void TabDiveNotes::on_dateEdit_editingFinished()
{
	if (ignoreInput || !parent.currentDive)
		return;
	QDateTime dateTime = timestampToDateTime(parent.currentDive->when);
	dateTime.setDate(ui.dateEdit->date());
	shiftTime(dateTime, parent.currentDive);
}

void TabDiveNotes::on_timeEdit_editingFinished()
{
	if (ignoreInput || !parent.currentDive)
		return;
	QDateTime dateTime = timestampToDateTime(parent.currentDive->when);
	dateTime.setTime(ui.timeEdit->time());
	shiftTime(dateTime, parent.currentDive);
}

void TabDiveNotes::on_tagWidget_editingFinished()
{
	if (ignoreInput || !parent.currentDive)
		return;

	divesEdited(Command::editTags(ui.tagWidget->getBlockStringList(), false));
}

void TabDiveNotes::on_location_diveSiteSelected()
{
	if (ignoreInput || !parent.currentDive)
		return;

	struct dive_site *newDs = ui.location->currDiveSite();
	if (newDs == RECENTLY_ADDED_DIVESITE)
		divesEdited(Command::editDiveSiteNew(ui.location->text(), false));
	else
		divesEdited(Command::editDiveSite(newDs, false));
}

void TabDiveNotes::on_locationPopupButton_clicked()
{
	ui.location->showAllSites();
}

void TabDiveNotes::on_diveTripLocation_editingFinished()
{
	if (!currentTrip)
		return;
	Command::editTripLocation(currentTrip, ui.diveTripLocation->text());
}

void TabDiveNotes::on_notes_editingFinished()
{
	if (!currentTrip && !parent.currentDive)
		return;

	QString html = ui.notes->toHtml();
	QString notes = isHtml(html) ? std::move(html) : ui.notes->toPlainText();

	if (currentTrip)
		Command::editTripNotes(currentTrip, notes);
	else
		divesEdited(Command::editNotes(notes, false));
}

void TabDiveNotes::on_rating_valueChanged(int value)
{
	if (ignoreInput || !parent.currentDive)
		return;

	divesEdited(Command::editRating(value, false));
}
