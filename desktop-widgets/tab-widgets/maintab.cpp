// SPDX-License-Identifier: GPL-2.0
/*
 * maintab.cpp
 *
 * classes for the "notebook" area of the main window of Subsurface
 *
 */
#include "desktop-widgets/tab-widgets/maintab.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/mapwidget.h"
#include "core/qthelper.h"
#include "core/trip.h"
#include "qt-models/diveplannermodel.h"
#include "desktop-widgets/divelistview.h"
#include "core/display.h"
#include "profile-widget/profilewidget2.h"
#include "desktop-widgets/diveplanner.h"
#include "core/divesitehelpers.h"
#include "qt-models/divecomputerextradatamodel.h"
#include "qt-models/divelocationmodel.h"
#include "qt-models/filtermodels.h"
#include "core/divesite.h"
#include "core/errorhelper.h"
#include "core/subsurface-string.h"
#include "core/gettextfromc.h"
#include "desktop-widgets/locationinformation.h"
#include "desktop-widgets/simplewidgets.h"
#include "commands/command.h"

#include "TabDiveEquipment.h"
#include "TabDiveExtraInfo.h"
#include "TabDiveInformation.h"
#include "TabDivePhotos.h"
#include "TabDiveStatistics.h"
#include "TabDiveSite.h"

#include <QCompleter>
#include <QSettings>
#include <QScrollBar>
#include <QShortcut>
#include <QMessageBox>
#include <QDesktopServices>
#include <QStringList>

struct Completers {
	QCompleter *divemaster;
	QCompleter *buddy;
	QCompleter *tags;
};

MainTab::MainTab(QWidget *parent) : QTabWidget(parent),
	editMode(NONE),
	lastSelectedDive(true),
	lastTabSelectedDive(0),
	lastTabSelectedDiveTrip(0),
	currentTrip(0)
{
	ui.setupUi(this);

	extraWidgets << new TabDiveEquipment();
	ui.tabWidget->addTab(extraWidgets.last(), tr("Equipment"));
	extraWidgets << new TabDiveInformation();
	ui.tabWidget->addTab(extraWidgets.last(), tr("Information"));
	extraWidgets << new TabDiveStatistics();
	ui.tabWidget->addTab(extraWidgets.last(), tr("Statistics"));
	extraWidgets << new TabDivePhotos();
	ui.tabWidget->addTab(extraWidgets.last(), tr("Media"));
	extraWidgets << new TabDiveExtraInfo();
	ui.tabWidget->addTab(extraWidgets.last(), tr("Extra Info"));
	extraWidgets << new TabDiveSite();
	ui.tabWidget->addTab(extraWidgets.last(), tr("Dive sites"));

	ui.dateEdit->setDisplayFormat(prefs.date_format);
	ui.timeEdit->setDisplayFormat(prefs.time_format);

	memset(&displayed_dive, 0, sizeof(displayed_dive));

	closeMessage();

	connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &MainTab::divesChanged);
	connect(&diveListNotifier, &DiveListNotifier::tripChanged, this, &MainTab::tripChanged);
	connect(&diveListNotifier, &DiveListNotifier::diveSiteChanged, this, &MainTab::diveSiteEdited);
	connect(&diveListNotifier, &DiveListNotifier::commandExecuted, this, &MainTab::closeWarning);

	connect(ui.editDiveSiteButton, &QToolButton::clicked, MainWindow::instance(), &MainWindow::startDiveSiteEdit);
	connect(ui.location, &DiveLocationLineEdit::entered, MapWidget::instance(), &MapWidget::centerOnIndex);
	connect(ui.location, &DiveLocationLineEdit::currentChanged, MapWidget::instance(), &MapWidget::centerOnIndex);
	connect(ui.location, &DiveLocationLineEdit::editingFinished, this, &MainTab::on_location_diveSiteSelected);

	QAction *action = new QAction(tr("Apply changes"), this);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(acceptChanges()));
	ui.diveNotesMessage->addAction(action);

	action = new QAction(tr("Discard changes"), this);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(rejectChanges()));
	ui.diveNotesMessage->addAction(action);

	action = new QAction(tr("OK"), this);
	connect(action, &QAction::triggered, this, &MainTab::closeWarning);
	ui.multiDiveWarningMessage->addAction(action);

	action = new QAction(tr("Undo"), this);
	connect(action, &QAction::triggered, Command::undoAction(this), &QAction::trigger);
	connect(action, &QAction::triggered, this, &MainTab::closeWarning);
	ui.multiDiveWarningMessage->addAction(action);

	QShortcut *closeKey = new QShortcut(QKeySequence(Qt::Key_Escape), this);
	connect(closeKey, SIGNAL(activated()), this, SLOT(escDetected()));

	if (qApp->style()->objectName() == "oxygen")
		setDocumentMode(true);
	else
		setDocumentMode(false);

	// we start out with the fields read-only; once things are
	// filled from a dive, they are made writeable
	setEnabled(false);

	Completers completers;
	completers.buddy = new QCompleter(&buddyModel, ui.buddy);
	completers.divemaster = new QCompleter(&diveMasterModel, ui.divemaster);
	completers.tags = new QCompleter(&tagModel, ui.tagWidget);
	completers.buddy->setCaseSensitivity(Qt::CaseInsensitive);
	completers.divemaster->setCaseSensitivity(Qt::CaseInsensitive);
	completers.tags->setCaseSensitivity(Qt::CaseInsensitive);
	ui.buddy->setCompleter(completers.buddy);
	ui.divemaster->setCompleter(completers.divemaster);
	ui.tagWidget->setCompleter(completers.tags);
	ui.diveNotesMessage->hide();
	ui.multiDiveWarningMessage->hide();
	ui.depth->hide();
	ui.depthLabel->hide();
	ui.duration->hide();
	ui.durationLabel->hide();
	setMinimumHeight(0);
	setMinimumWidth(0);

	// Current display of things on Gnome3 looks like shit, so
	// let's fix that.
	if (isGnome3Session()) {
		QPalette p;
		p.setColor(QPalette::Window, QColor(Qt::white));
		ui.scrollArea->viewport()->setPalette(p);

		// GroupBoxes in Gnome3 looks like I'v drawn them...
		static const QString gnomeCss = QStringLiteral(
			"QGroupBox {"
				"background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
				"stop: 0 #E0E0E0, stop: 1 #FFFFFF);"
				"border: 2px solid gray;"
				"border-radius: 5px;"
				"margin-top: 1ex;"
			"}"
			"QGroupBox::title {"
				"subcontrol-origin: margin;"
				"subcontrol-position: top center;"
				"padding: 0 3px;"
				"background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
				"stop: 0 #E0E0E0, stop: 1 #FFFFFF);"
			"}");
		Q_FOREACH (QGroupBox *box, findChildren<QGroupBox *>()) {
			box->setStyleSheet(gnomeCss);
		}
	}
	// QLineEdit and QLabels should have minimal margin on the left and right but not waste vertical space
	QMargins margins(3, 2, 1, 0);
	Q_FOREACH (QLabel *label, findChildren<QLabel *>()) {
		label->setContentsMargins(margins);
	}

	connect(ui.diveNotesMessage, &KMessageWidget::showAnimationFinished,
					ui.location, &DiveLocationLineEdit::fixPopupPosition);
	connect(ui.multiDiveWarningMessage, &KMessageWidget::showAnimationFinished,
					ui.location, &DiveLocationLineEdit::fixPopupPosition);

	// enable URL clickability in notes:
	new TextHyperlinkEventFilter(ui.notes);//destroyed when ui.notes is destroyed

	ui.diveTripLocation->hide();
}

MainTab::~MainTab()
{
}

void MainTab::hideMessage()
{
	ui.diveNotesMessage->animatedHide();
}

void MainTab::closeMessage()
{
	hideMessage();
	ui.diveNotesMessage->setCloseButtonVisible(false);
}

void MainTab::closeWarning()
{
	ui.multiDiveWarningMessage->hide();
}

void MainTab::displayMessage(QString str)
{
	ui.diveNotesMessage->setCloseButtonVisible(false);
	ui.diveNotesMessage->setText(str);
	ui.diveNotesMessage->animatedShow();
}

void MainTab::enableEdition(EditMode newEditMode)
{
	if (((newEditMode == DIVE || newEditMode == NONE) && current_dive == NULL) || editMode != NONE)
		return;
	modified = false;
	if ((newEditMode == DIVE || newEditMode == NONE) &&
	    current_dive->dc.model &&
	    strcmp(current_dive->dc.model, "manually added dive") == 0) {
		// editCurrentDive will call enableEdition with newEditMode == MANUALLY_ADDED_DIVE
		// so exit this function here after editCurrentDive() returns



		// FIXME : can we get rid of this recursive crap?



		MainWindow::instance()->editCurrentDive();
		return;
	}

	ui.editDiveSiteButton->setEnabled(false);
	MainWindow::instance()->diveList->setEnabled(false);
	MainWindow::instance()->setEnabledToolbar(false);
	MainWindow::instance()->enterEditState();
	ui.tabWidget->setTabEnabled(2, false);
	ui.tabWidget->setTabEnabled(3, false);
	ui.tabWidget->setTabEnabled(5, false);

	ui.dateEdit->setEnabled(true);
	if (amount_selected > 1) {
		displayMessage(tr("Multiple dives are being edited."));
	} else {
		displayMessage(tr("This dive is being edited."));
	}
	editMode = newEditMode != NONE ? newEditMode : DIVE;
}

// This function gets called if a field gets updated by an undo command.
// Refresh the corresponding UI field.
void MainTab::divesChanged(const QVector<dive *> &dives, DiveField field)
{
	// If the current dive is not in list of changed dives, do nothing
	if (!current_dive || !dives.contains(current_dive))
		return;

	if (field.duration)
		ui.duration->setText(render_seconds_to_string(current_dive->duration.seconds));
	if (field.depth)
		ui.depth->setText(get_depth_string(current_dive->maxdepth, true));
	if (field.rating)
		ui.rating->setCurrentStars(current_dive->rating);
	if (field.notes)
		updateNotes(current_dive);
	if (field.datetime) {
		updateDateTime(current_dive);
		MainWindow::instance()->graphics->dateTimeChanged();
		DivePlannerPointsModel::instance()->getDiveplan().when = current_dive->when;
	}
	if (field.divesite)
		updateDiveSite(current_dive);
	if (field.tags)
		ui.tagWidget->setText(get_taglist_string(current_dive->tag_list));
	if (field.buddy)
		ui.buddy->setText(current_dive->buddy);
	if (field.divemaster)
		ui.divemaster->setText(current_dive->divemaster);

	// If duration or depth changed, the profile needs to be replotted
	if (field.duration || field.depth)
		MainWindow::instance()->graphics->plotDive(current_dive, true);
}

void MainTab::diveSiteEdited(dive_site *ds, int)
{
	if (current_dive && current_dive->dive_site == ds)
		updateDiveSite(current_dive);
}

// This function gets called if a trip-field gets updated by an undo command.
// Refresh the corresponding UI field.
void MainTab::tripChanged(dive_trip *trip, TripField field)
{
	// If the current dive is not in list of changed dives, do nothing
	if (currentTrip != trip)
		return;

	if (field.notes)
		ui.notes->setText(currentTrip->notes);
	if (field.location)
		ui.diveTripLocation->setText(currentTrip->location);
}

void MainTab::nextInputField(QKeyEvent *event)
{
	keyPressEvent(event);
}

bool MainTab::isEditing()
{
	return editMode != NONE;
}

static bool isHtml(const QString &s)
{
	return s.contains("<div", Qt::CaseInsensitive) || s.contains("<table", Qt::CaseInsensitive);
}

void MainTab::updateNotes(const struct dive *d)
{
	QString tmp(d->notes);
	if (isHtml(tmp)) {
		ui.notes->setHtml(tmp);
	} else {
		ui.notes->setPlainText(tmp);
	}
}

static QDateTime timestampToDateTime(timestamp_t when)
{
	// Subsurface always uses "local time" as in "whatever was the local time at the location"
	// so all time stamps have no time zone information and are in UTC
	QDateTime localTime = QDateTime::fromMSecsSinceEpoch(1000 * when, Qt::UTC);
	localTime.setTimeSpec(Qt::UTC);
	return localTime;
}
void MainTab::updateDateTime(const struct dive *d)
{
	QDateTime localTime = timestampToDateTime(d->when);
	ui.dateEdit->setDate(localTime.date());
	ui.timeEdit->setTime(localTime.time());
}

void MainTab::updateTripDate(const struct dive_trip *t)
{
	QDateTime localTime = timestampToDateTime(trip_date(t));
	ui.dateEdit->setDate(localTime.date());
}

void MainTab::updateDiveSite(struct dive *d)
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
}

void MainTab::updateDiveInfo()
{
	ui.location->refreshDiveSiteCache();
	EditMode rememberEM = editMode;
	// don't execute this while adding / planning a dive
	if (editMode == MANUALLY_ADDED_DIVE || MainWindow::instance()->graphics->isPlanner())
		return;

	// If there is no current dive, disable all widgets except the last, which is the dive site tab.
	// TODO: Conceptually, the dive site tab shouldn't even be a tab here!
	bool enabled = current_dive != nullptr;
	ui.notesTab->setEnabled(enabled);
	for (int i = 0; i < extraWidgets.size() - 1; ++i)
		extraWidgets[i]->setEnabled(enabled);

	editMode = IGNORE_MODE; // don't trigger on changes to the widgets

	for (TabBase *widget: extraWidgets)
		widget->updateData();

	if (current_dive) {
		// If we're on the dive-site tab, we don't want to switch tab when entering / exiting
		// trip mode. The reason is that
		// 1) this disrupts the user-experience and
		// 2) the filter is reset, potentially erasing the current trip under our feet.
		// TODO: Don't hard code tab location!
		bool onDiveSiteTab = ui.tabWidget->currentIndex() == 6;
		if (MainWindow::instance() && MainWindow::instance()->diveList->selectedTrips().count() == 1) {
			currentTrip = MainWindow::instance()->diveList->selectedTrips().front();
			// Remember the tab selected for last dive but only if we're not on the dive site tab
			if (lastSelectedDive && !onDiveSiteTab)
				lastTabSelectedDive = ui.tabWidget->currentIndex();
			ui.tabWidget->setTabText(0, tr("Trip notes"));
			ui.tabWidget->setTabEnabled(1, false);
			ui.tabWidget->setTabEnabled(2, false);
			ui.tabWidget->setTabEnabled(5, false);
			// Recover the tab selected for last dive trip but only if we're not on the dive site tab
			if (lastSelectedDive && !onDiveSiteTab)
				ui.tabWidget->setCurrentIndex(lastTabSelectedDiveTrip);
			lastSelectedDive = false;
			// only use trip relevant fields
			ui.divemaster->setVisible(false);
			ui.DivemasterLabel->setVisible(false);
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
			// Remember the tab selected for last dive trip but only if we're not on the dive site tab
			if (!lastSelectedDive && !onDiveSiteTab)
				lastTabSelectedDiveTrip = ui.tabWidget->currentIndex();
			ui.tabWidget->setTabText(0, tr("Notes"));
			ui.tabWidget->setTabEnabled(1, true);
			ui.tabWidget->setTabEnabled(2, true);
			ui.tabWidget->setTabEnabled(3, true);
			ui.tabWidget->setTabEnabled(4, true);
			ui.tabWidget->setTabEnabled(5, true);
			// Recover the tab selected for last dive but only if we're not on the dive site tab
			if (!lastSelectedDive && !onDiveSiteTab)
				ui.tabWidget->setCurrentIndex(lastTabSelectedDive);
			lastSelectedDive = true;
			currentTrip = NULL;
			// make all the fields visible writeable
			ui.diveTripLocation->hide();
			ui.location->show();
			ui.locationPopupButton->show();
			ui.editDiveSiteButton->show();
			ui.divemaster->setVisible(true);
			ui.buddy->setVisible(true);
			ui.rating->setVisible(true);
			ui.RatingLabel->setVisible(true);
			ui.BuddyLabel->setVisible(true);
			ui.DivemasterLabel->setVisible(true);
			ui.TagLabel->setVisible(true);
			ui.tagWidget->setVisible(true);
			ui.dateEdit->setReadOnly(false);
			ui.timeLabel->setVisible(true);
			ui.timeEdit->setVisible(true);
			/* and fill them from the dive */
			ui.rating->setCurrentStars(current_dive->rating);
			// reset labels in case we last displayed trip notes
			ui.LocationLabel->setText(tr("Location"));
			ui.NotesLabel->setText(tr("Notes"));
			ui.tagWidget->setText(get_taglist_string(current_dive->tag_list));
			bool isManual = same_string(current_dive->dc.model, "manually added dive");
			ui.depth->setVisible(isManual);
			ui.depthLabel->setVisible(isManual);
			ui.duration->setVisible(isManual);
			ui.durationLabel->setVisible(isManual);

			updateNotes(current_dive);
			updateDiveSite(current_dive);
			updateDateTime(current_dive);
			ui.divemaster->setText(current_dive->divemaster);
			ui.buddy->setText(current_dive->buddy);
		}
		ui.duration->setText(render_seconds_to_string(current_dive->duration.seconds));
		ui.depth->setText(get_depth_string(current_dive->maxdepth, true));

		if(ui.locationTags->text().isEmpty())
			ui.locationTags->hide();
		else
			ui.locationTags->show();
		ui.editDiveSiteButton->setEnabled(!ui.location->text().isEmpty());
		/* unset the special value text for date and time, just in case someone dove at midnight */
		ui.dateEdit->setSpecialValueText(QString());
		ui.timeEdit->setSpecialValueText(QString());
	} else {
		/* clear the fields */
		clearTabs();
		ui.rating->setCurrentStars(0);
		ui.location->clear();
		ui.divemaster->clear();
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
	editMode = rememberEM;

	if (verbose && current_dive && current_dive->dive_site)
		qDebug() << "Set the current dive site:" << current_dive->dive_site->uuid;
}

void MainTab::reload()
{
	buddyModel.updateModel();
	diveMasterModel.updateModel();
	tagModel.updateModel();
	LocationInformationModel::instance()->update();
}

void MainTab::refreshDisplayedDiveSite()
{
	ui.location->setCurrentDiveSite(current_dive);
}

void MainTab::acceptChanges()
{
	int addedId = -1;

	if (ui.location->hasFocus())
		stealFocus();

	EditMode lastMode = editMode;
	editMode = IGNORE_MODE;
	tabBar()->setTabIcon(0, QIcon()); // Notes
	tabBar()->setTabIcon(1, QIcon()); // Equipment
	ui.dateEdit->setEnabled(true);
	hideMessage();

	if (lastMode == MANUALLY_ADDED_DIVE) {
		// preserve any changes to the profile
		free(current_dive->dc.sample);
		copy_samples(&displayed_dive.dc, &current_dive->dc);
		addedId = displayed_dive.id;
	}

	// TODO: This is a temporary hack until the equipment tab is included in the undo system:
	// The equipment tab is hardcoded at the first place of the "extra widgets".
	((TabDiveEquipment *)extraWidgets[0])->acceptChanges();

	if (lastMode == MANUALLY_ADDED_DIVE) {
		// we just added or edited the dive, let fixup_dive() make
		// sure we get the max. depth right
		current_dive->maxdepth.mm = current_dc->maxdepth.mm = 0;
		fixup_dive(current_dive);
		set_dive_nr_for_current_dive();
		MainWindow::instance()->showProfile();
		mark_divelist_changed(true);
		DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::NOTHING);
	}
	int scrolledBy = MainWindow::instance()->diveList->verticalScrollBar()->sliderPosition();
	if (lastMode == MANUALLY_ADDED_DIVE) {
		MainWindow::instance()->diveList->reload();
		int newDiveNr = get_divenr(get_dive_by_uniq_id(addedId));
		MainWindow::instance()->diveList->unselectDives();
		MainWindow::instance()->diveList->selectDive(newDiveNr, true);
		MainWindow::instance()->refreshDisplay();
		MainWindow::instance()->graphics->replot();
	} else {
		MainWindow::instance()->diveList->rememberSelection();
		MainWindow::instance()->refreshDisplay();
		MainWindow::instance()->diveList->restoreSelection();
	}
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::NOTHING);
	MainWindow::instance()->diveList->verticalScrollBar()->setSliderPosition(scrolledBy);
	MainWindow::instance()->diveList->setFocus();
	MainWindow::instance()->exitEditState();
	MainWindow::instance()->setEnabledToolbar(true);
	ui.editDiveSiteButton->setEnabled(!ui.location->text().isEmpty());
	editMode = NONE;
}

bool weightsystems_equal(const dive *d1, const dive *d2)
{
	if (d1->weightsystems.nr != d2->weightsystems.nr)
		return false;
	for (int i = 0; i < d1->weightsystems.nr; ++i) {
		if (!same_weightsystem(d1->weightsystems.weightsystems[i], d2->weightsystems.weightsystems[i]))
			return false;
	}
	return true;
}

bool cylinders_equal(const dive *d1, const dive *d2)
{
	if (d1->cylinders.nr != d2->cylinders.nr)
		return false;
	for (int i = 0; i < d1->cylinders.nr; ++i) {
		if (!same_cylinder(*get_cylinder(d1, i), *get_cylinder(d2, i)))
			return false;
	}
	return true;
}

void MainTab::rejectChanges()
{
	EditMode lastMode = editMode;

	if (lastMode != NONE && current_dive &&
	    (modified ||
	     !cylinders_equal(current_dive, &displayed_dive) ||
	     !weightsystems_equal(current_dive, &displayed_dive))) {
		if (QMessageBox::warning(MainWindow::instance(), TITLE_OR_TEXT(tr("Discard the changes?"),
									       tr("You are about to discard your changes.")),
					 QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Discard) != QMessageBox::Discard) {
			return;
		}
	}
	ui.dateEdit->setEnabled(true);
	editMode = NONE;
	tabBar()->setTabIcon(0, QIcon()); // Notes
	tabBar()->setTabIcon(1, QIcon()); // Equipment
	hideMessage();
	// no harm done to call cancelPlan even if we were not PLAN mode...
	DivePlannerPointsModel::instance()->cancelPlan();

	// now make sure that the correct dive is displayed
	if (current_dive)
		copy_dive(current_dive, &displayed_dive);
	else
		clear_dive(&displayed_dive);
	updateDiveInfo();

	for (auto widget: extraWidgets) {
		widget->updateData();
	}

	// TODO: This is a temporary hack until the equipment tab is included in the undo system:
	// The equipment tab is hardcoded at the first place of the "extra widgets".
	((TabDiveEquipment *)extraWidgets[0])->rejectChanges();

	// the user could have edited the location and then canceled the edit
	// let's get the correct location back in view
	MapWidget::instance()->centerOnDiveSite(current_dive ? current_dive->dive_site : nullptr);
	// show the profile and dive info
	MainWindow::instance()->graphics->replot();
	MainWindow::instance()->setEnabledToolbar(true);
	MainWindow::instance()->exitEditState();
	ui.editDiveSiteButton->setEnabled(!ui.location->text().isEmpty());
}

void MainTab::divesEdited(int i)
{
	// No warning if only one dive was edited
	if (i <= 1)
		return;
	ui.multiDiveWarningMessage->setCloseButtonVisible(false);
	ui.multiDiveWarningMessage->setText(tr("Warning: edited %1 dives").arg(i));
	ui.multiDiveWarningMessage->show();
}

static QStringList stringToList(const QString &s)
{
	QStringList res = s.split(",", QString::SkipEmptyParts);
	for (QString &str: res)
		str = str.trimmed();
	return res;
}

void MainTab::on_buddy_editingFinished()
{
	if (editMode == IGNORE_MODE || !current_dive)
		return;

	divesEdited(Command::editBuddies(stringToList(ui.buddy->toPlainText()), false));
}

void MainTab::on_divemaster_editingFinished()
{
	if (editMode == IGNORE_MODE || !current_dive)
		return;

	divesEdited(Command::editDiveMaster(stringToList(ui.divemaster->toPlainText()), false));
}

void MainTab::on_duration_editingFinished()
{
	if (editMode == IGNORE_MODE || !current_dive)
		return;

	// Duration editing is special: we only edit the current dive.
	divesEdited(Command::editDuration(parseDurationToSeconds(ui.duration->text()), true));
}

void MainTab::on_depth_editingFinished()
{
	if (editMode == IGNORE_MODE || !current_dive)
		return;

	// Depth editing is special: we only edit the current dive.
	divesEdited(Command::editDepth(parseLengthToMm(ui.depth->text()), true));
}

// Editing of the dive time is different. If multiple dives are edited,
// all dives are shifted by an offset.
static void shiftTime(QDateTime &dateTime)
{
	QVector<dive *> dives;
	struct dive *d;
	int i;
	for_each_dive (i, d) {
		if (d->selected)
			dives.append(d);
	}

	timestamp_t when = dateTime.toTime_t();
	if (current_dive && current_dive->when != when) {
		timestamp_t offset = when - current_dive->when;
		Command::shiftTime(dives, (int)offset);
	}
}

void MainTab::on_dateEdit_dateChanged(const QDate &date)
{
	if (editMode == IGNORE_MODE || !current_dive)
		return;
	QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(1000*current_dive->when, Qt::UTC);
	dateTime.setTimeSpec(Qt::UTC);
	dateTime.setDate(date);
	shiftTime(dateTime);
}

void MainTab::on_timeEdit_timeChanged(const QTime &time)
{
	if (editMode == IGNORE_MODE || !current_dive)
		return;
	QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(1000*current_dive->when, Qt::UTC);
	dateTime.setTimeSpec(Qt::UTC);
	dateTime.setTime(time);
	shiftTime(dateTime);
}

void MainTab::on_tagWidget_editingFinished()
{
	if (editMode == IGNORE_MODE || !current_dive)
		return;

	divesEdited(Command::editTags(ui.tagWidget->getBlockStringList(), false));
}

void MainTab::on_location_diveSiteSelected()
{
	if (editMode == IGNORE_MODE || !current_dive)
		return;

	struct dive_site *newDs = ui.location->currDiveSite();
	if (newDs == RECENTLY_ADDED_DIVESITE)
		divesEdited(Command::editDiveSiteNew(ui.location->text(), false));
	else
		divesEdited(Command::editDiveSite(newDs, false));
}

void MainTab::on_locationPopupButton_clicked()
{
	ui.location->showAllSites();
}

void MainTab::on_diveTripLocation_editingFinished()
{
	if (!currentTrip)
		return;
	Command::editTripLocation(currentTrip, ui.diveTripLocation->text());
}

void MainTab::on_notes_editingFinished()
{
	if (!currentTrip && !current_dive)
		return;

	QString html = ui.notes->toHtml();
	QString notes = isHtml(html) ? html : ui.notes->toPlainText();

	if (currentTrip)
		Command::editTripNotes(currentTrip, notes);
	else
		divesEdited(Command::editNotes(notes, false));
}

void MainTab::on_rating_valueChanged(int value)
{
	if (editMode == IGNORE_MODE || !current_dive)
		return;

	divesEdited(Command::editRating(value, false));
}

// Remove focus from any active field to update the corresponding value in the dive.
// Do this by setting the focus to ourself
void MainTab::stealFocus()
{
	setFocus();
}

void MainTab::escDetected()
{
	// In edit mode, pressing escape cancels the current changes.
	// In standard mode, remove focus of any active widget to
	if (editMode != NONE)
		rejectChanges();
	else
		stealFocus();
}

void MainTab::clearTabs()
{
	for (auto widget: extraWidgets)
		widget->clear();
}
