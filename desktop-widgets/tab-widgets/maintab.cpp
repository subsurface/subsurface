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
#include "core/selection.h"
#include "desktop-widgets/diveplanner.h"
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
#include "TabDiveComputer.h"

#include <QCompleter>
#include <QScrollBar>
#include <QShortcut>
#include <QMessageBox>

struct Completers {
	QCompleter *divemaster;
	QCompleter *buddy;
	QCompleter *tags;
};

static bool paletteIsDark(const QPalette &p)
{
	// we consider a palette dark if the text color is lighter than the windows background
	return p.window().color().valueF() < p.windowText().color().valueF();
}

MainTab::MainTab(QWidget *parent) : QTabWidget(parent),
	editMode(false),
	ignoreInput(false),
	lastSelectedDive(true),
	lastTabSelectedDive(0),
	lastTabSelectedDiveTrip(0),
	currentTrip(0)
{
	ui.setupUi(this);

	extraWidgets << new TabDiveEquipment(this);
	ui.tabWidget->addTab(extraWidgets.last(), tr("Equipment"));
	extraWidgets << new TabDiveInformation(this);
	ui.tabWidget->addTab(extraWidgets.last(), tr("Information"));
	extraWidgets << new TabDiveStatistics(this);
	ui.tabWidget->addTab(extraWidgets.last(), tr("Summary"));
	extraWidgets << new TabDivePhotos(this);
	ui.tabWidget->addTab(extraWidgets.last(), tr("Media"));
	extraWidgets << new TabDiveExtraInfo(this);
	ui.tabWidget->addTab(extraWidgets.last(), tr("Extra Info"));
	extraWidgets << new TabDiveSite(this);
	ui.tabWidget->addTab(extraWidgets.last(), tr("Dive sites"));
	extraWidgets << new TabDiveComputer(this);
	ui.tabWidget->addTab(extraWidgets.last(), tr("Device names"));

	// make sure we know if this is a light or dark mode
	isDark = paletteIsDark(palette());

	// call colorsChanged() for the initial setup now that the extraWidgets are loaded
	colorsChanged();

	updateDateTimeFields();

	closeMessage();

	connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &MainTab::divesChanged);
	connect(&diveListNotifier, &DiveListNotifier::tripChanged, this, &MainTab::tripChanged);
	connect(&diveListNotifier, &DiveListNotifier::diveSiteChanged, this, &MainTab::diveSiteEdited);
	connect(&diveListNotifier, &DiveListNotifier::commandExecuted, this, &MainTab::closeWarning);
	connect(&diveListNotifier, &DiveListNotifier::settingsChanged, this, &MainTab::updateDiveInfo);

	connect(ui.editDiveSiteButton, &QToolButton::clicked, MainWindow::instance(), &MainWindow::startDiveSiteEdit);
	connect(ui.location, &DiveLocationLineEdit::entered, MapWidget::instance(), &MapWidget::centerOnIndex);
	connect(ui.location, &DiveLocationLineEdit::currentChanged, MapWidget::instance(), &MapWidget::centerOnIndex);
	connect(ui.location, &DiveLocationLineEdit::editingFinished, this, &MainTab::on_location_diveSiteSelected);

	// One might think that we could listen to the precise property-changed signals of the preferences system.
	// Alas, this is not the case. When the user switches to system-format, the preferences sends the according
	// signal. However, the correct date and time format is set by the preferences dialog later. This should be fixed.
	connect(&diveListNotifier, &DiveListNotifier::settingsChanged, this, &MainTab::updateDateTimeFields);

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

void MainTab::updateDateTimeFields()
{
	ui.dateEdit->setDisplayFormat(prefs.date_format);
	ui.timeEdit->setDisplayFormat(prefs.time_format);
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

void MainTab::enableEdition()
{
	if (current_dive == NULL || editMode)
		return;

	ui.editDiveSiteButton->setEnabled(false);
	MainWindow::instance()->diveList->setEnabled(false);
	MainWindow::instance()->setEnabledToolbar(false);

	ui.dateEdit->setEnabled(true);
	displayMessage(tr("This dive is being edited."));

	editMode = true;
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
		MainWindow::instance()->refreshProfile();
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
	return editMode;
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

	if (ui.locationTags->text().isEmpty())
		ui.locationTags->hide();
	else
		ui.locationTags->show();
}

void MainTab::updateDiveInfo()
{
	ui.location->refreshDiveSiteCache();
	// don't execute this while adding / planning a dive
	if (editMode || DivePlannerPointsModel::instance()->isPlanner())
		return;

	// If there is no current dive, disable all widgets except the last two,
	// which are the dive site tab and the dive computer tabs.
	// TODO: Conceptually, these two shouldn't even be a tabs here!
	bool enabled = current_dive != nullptr;
	ui.notesTab->setEnabled(enabled);
	for (int i = 0; i < extraWidgets.size() - 2; ++i)
		extraWidgets[i]->setEnabled(enabled);

	ignoreInput = true; // don't trigger on changes to the widgets

	if (current_dive) {
		for (TabBase *widget: extraWidgets)
			widget->updateData();

		// If we're on the dive-site tab, we don't want to switch tab when entering / exiting
		// trip mode. The reason is that
		// 1) this disrupts the user-experience and
		// 2) the filter is reset, potentially erasing the current trip under our feet.
		// TODO: Don't hard code tab location!
		bool onDiveSiteTab = ui.tabWidget->currentIndex() == 6;
		currentTrip = single_selected_trip();
		if (currentTrip) {
			// Remember the tab selected for last dive but only if we're not on the dive site tab
			if (lastSelectedDive && !onDiveSiteTab)
				lastTabSelectedDive = ui.tabWidget->currentIndex();
			ui.tabWidget->setTabText(0, tr("Trip notes"));
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
			// Recover the tab selected for last dive but only if we're not on the dive site tab
			if (!lastSelectedDive && !onDiveSiteTab)
				ui.tabWidget->setCurrentIndex(lastTabSelectedDive);
			lastSelectedDive = true;
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
	ignoreInput = false;

	if (verbose && current_dive && current_dive->dive_site)
		qDebug() << "Set the current dive site:" << current_dive->dive_site->uuid;
}

void MainTab::refreshDisplayedDiveSite()
{
	ui.location->setCurrentDiveSite(current_dive);
}

void MainTab::acceptChanges()
{
	if (ui.location->hasFocus())
		stealFocus();

	ignoreInput = true;
	ui.dateEdit->setEnabled(true);
	hideMessage();

	MainWindow::instance()->showProfile();
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::NOTHING);
	Command::editProfile(&displayed_dive);

	int scrolledBy = MainWindow::instance()->diveList->verticalScrollBar()->sliderPosition();
	MainWindow::instance()->diveList->reload();
	MainWindow::instance()->refreshDisplay();
	MainWindow::instance()->refreshProfile();

	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::NOTHING);
	MainWindow::instance()->diveList->verticalScrollBar()->setSliderPosition(scrolledBy);
	MainWindow::instance()->diveList->setFocus();
	MainWindow::instance()->setEnabledToolbar(true);
	ui.editDiveSiteButton->setEnabled(!ui.location->text().isEmpty());
	ignoreInput = false;
	editMode = false;
}

void MainTab::rejectChanges()
{
	if (QMessageBox::warning(MainWindow::instance(), TITLE_OR_TEXT(tr("Discard the changes?"),
								       tr("You are about to discard your changes.")),
				 QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Discard) != QMessageBox::Discard) {
		return;
	}

	ui.dateEdit->setEnabled(true);
	editMode = false;
	hideMessage();
	// no harm done to call cancelPlan even if we were not PLAN mode...
	DivePlannerPointsModel::instance()->cancelPlan();

	updateDiveInfo();

	// show the profile and dive info
	MainWindow::instance()->refreshDisplay();
	MainWindow::instance()->refreshProfile();
	MainWindow::instance()->setEnabledToolbar(true);
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

void MainTab::on_buddy_editingFinished()
{
	if (ignoreInput || !current_dive)
		return;

	divesEdited(Command::editBuddies(stringToList(ui.buddy->toPlainText()), false));
}

void MainTab::on_divemaster_editingFinished()
{
	if (ignoreInput || !current_dive)
		return;

	divesEdited(Command::editDiveMaster(stringToList(ui.divemaster->toPlainText()), false));
}

void MainTab::on_duration_editingFinished()
{
	if (ignoreInput || !current_dive)
		return;

	// Duration editing is special: we only edit the current dive.
	divesEdited(Command::editDuration(parseDurationToSeconds(ui.duration->text()), true));
}

void MainTab::on_depth_editingFinished()
{
	if (ignoreInput || !current_dive)
		return;

	// Depth editing is special: we only edit the current dive.
	divesEdited(Command::editDepth(parseLengthToMm(ui.depth->text()), true));
}

// Editing of the dive time is different. If multiple dives are edited,
// all dives are shifted by an offset.
static void shiftTime(QDateTime &dateTime)
{
	timestamp_t when = dateTimeToTimestamp(dateTime);
	if (current_dive && current_dive->when != when) {
		timestamp_t offset = when - current_dive->when;
		Command::shiftTime(getDiveSelection(), (int)offset);
	}
}

void MainTab::on_dateEdit_editingFinished()
{
	if (ignoreInput || !current_dive)
		return;
	QDateTime dateTime = timestampToDateTime(current_dive->when);
	dateTime.setDate(ui.dateEdit->date());
	shiftTime(dateTime);
}

void MainTab::on_timeEdit_editingFinished()
{
	if (ignoreInput || !current_dive)
		return;
	QDateTime dateTime = timestampToDateTime(current_dive->when);
	dateTime.setTime(ui.timeEdit->time());
	shiftTime(dateTime);
}

void MainTab::on_tagWidget_editingFinished()
{
	if (ignoreInput || !current_dive)
		return;

	divesEdited(Command::editTags(ui.tagWidget->getBlockStringList(), false));
}

void MainTab::on_location_diveSiteSelected()
{
	if (ignoreInput || !current_dive)
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
	if (ignoreInput || !current_dive)
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
	if (editMode)
		rejectChanges();
	else
		stealFocus();
}

void MainTab::clearTabs()
{
	for (auto widget: extraWidgets)
		widget->clear();
}

void MainTab::changeEvent(QEvent *ev)
{
	if (ev->type() == QEvent::PaletteChange) {
		// check if this is a light or dark mode
		bool dark = paletteIsDark(palette());
		if (dark != isDark) {
			// things have changed, so setup the colors correctly
			isDark = dark;
			colorsChanged();
		}
	}
	QTabWidget::changeEvent(ev);
}

// setup the colors of 'header' elements in the tab widget
void MainTab::colorsChanged()
{
	QString colorText = isDark ? QStringLiteral("lightblue") : QStringLiteral("mediumblue");
	QString lastpart = colorText + " ;}";

	// only set the color if the widget is enabled
	QString CSSLabelcolor = "QLabel:enabled { color: " + lastpart;
	QString CSSTitlecolor = "QGroupBox::title:enabled { color: " + lastpart ;

	// apply to all the group boxes
	QList<QGroupBox *>groupBoxes = this->findChildren<QGroupBox *>();
	for (QGroupBox *gb: groupBoxes)
		gb->setStyleSheet(QString(CSSTitlecolor));

	// apply to all labels that are marked as headers in the .ui file
	QList<QLabel *>labels = this->findChildren<QLabel *>();
	for (QLabel *ql: labels) {
		if (ql->property("isHeader").toBool())
			ql->setStyleSheet(QString(CSSLabelcolor));
	}

	// finally call the individual updateUi() functions so they can overwrite these style sheets
	for (TabBase *widget: extraWidgets)
		widget->updateUi(colorText);
}
