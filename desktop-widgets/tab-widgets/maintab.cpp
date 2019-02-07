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
#include "core/statistics.h"
#include "desktop-widgets/modeldelegates.h"
#include "qt-models/diveplannermodel.h"
#include "desktop-widgets/divelistview.h"
#include "core/display.h"
#include "profile-widget/profilewidget2.h"
#include "desktop-widgets/diveplanner.h"
#include "core/divesitehelpers.h"
#include "qt-models/cylindermodel.h"
#include "qt-models/weightmodel.h"
#include "qt-models/divecomputerextradatamodel.h"
#include "qt-models/divelocationmodel.h"
#include "qt-models/filtermodels.h"
#include "core/divesite.h"
#include "core/subsurface-string.h"
#include "core/gettextfromc.h"
#include "desktop-widgets/locationinformation.h"
#include "desktop-widgets/command.h"
#include "desktop-widgets/simplewidgets.h"

#include "TabDiveExtraInfo.h"
#include "TabDiveInformation.h"
#include "TabDivePhotos.h"
#include "TabDiveStatistics.h"

#include <QCompleter>
#include <QSettings>
#include <QScrollBar>
#include <QShortcut>
#include <QMessageBox>
#include <QDesktopServices>
#include <QStringList>

MainTab::MainTab(QWidget *parent) : QTabWidget(parent),
	weightModel(new WeightModel(this)),
	cylindersModel(new CylindersModel(this)),
	editMode(NONE),
	copyPaste(false),
	lastSelectedDive(true),
	lastTabSelectedDive(0),
	lastTabSelectedDiveTrip(0),
	currentTrip(0)
{
	ui.setupUi(this);

	extraWidgets << new TabDiveInformation();
	ui.tabWidget->addTab(extraWidgets.last(), tr("Information"));
	extraWidgets << new TabDiveStatistics();
	ui.tabWidget->addTab(extraWidgets.last(), tr("Statistics"));
	extraWidgets << new TabDivePhotos();
	ui.tabWidget->addTab(extraWidgets.last(), tr("Media"));
	extraWidgets << new TabDiveExtraInfo();
	ui.tabWidget->addTab(extraWidgets.last(), tr("Extra Info"));

	ui.dateEdit->setDisplayFormat(prefs.date_format);
	ui.timeEdit->setDisplayFormat(prefs.time_format);

	memset(&displayed_dive, 0, sizeof(displayed_dive));
	memset(&displayedTrip, 0, sizeof(displayedTrip));

	// This makes sure we only delete the models
	// after the destructor of the tables,
	// this is needed to save the column sizes.
	cylindersModel->setParent(ui.cylinders);
	weightModel->setParent(ui.weights);

	ui.cylinders->setModel(cylindersModel);
	ui.weights->setModel(weightModel);
	closeMessage();

	connect(&diveListNotifier, &DiveListNotifier::divesEdited, this, &MainTab::divesEdited);

	connect(ui.editDiveSiteButton, SIGNAL(clicked()), MainWindow::instance(), SIGNAL(startDiveSiteEdit()));
	connect(ui.location, &DiveLocationLineEdit::entered, MapWidget::instance(), &MapWidget::centerOnIndex);
	connect(ui.location, &DiveLocationLineEdit::currentChanged, MapWidget::instance(), &MapWidget::centerOnIndex);

	QAction *action = new QAction(tr("Apply changes"), this);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(acceptChanges()));
	addMessageAction(action);

	action = new QAction(tr("Discard changes"), this);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(rejectChanges()));
	addMessageAction(action);

	QShortcut *closeKey = new QShortcut(QKeySequence(Qt::Key_Escape), this);
	connect(closeKey, SIGNAL(activated()), this, SLOT(escDetected()));

	if (qApp->style()->objectName() == "oxygen")
		setDocumentMode(true);
	else
		setDocumentMode(false);

	// we start out with the fields read-only; once things are
	// filled from a dive, they are made writeable
	setEnabled(false);

	ui.cylinders->setTitle(tr("Cylinders"));
	ui.cylinders->setBtnToolTip(tr("Add cylinder"));
	connect(ui.cylinders, SIGNAL(addButtonClicked()), this, SLOT(addCylinder_clicked()));

	ui.weights->setTitle(tr("Weights"));
	ui.weights->setBtnToolTip(tr("Add weight system"));
	connect(ui.weights, SIGNAL(addButtonClicked()), this, SLOT(addWeight_clicked()));

	// This needs to be the same order as enum dive_comp_type in dive.h!
	QStringList types = QStringList();
	for (int i = 0; i < NUM_DIVEMODE; i++)
		types.append(gettextFromC::tr(divemode_text_ui[i]));
	ui.DiveType->insertItems(0, types);
	connect(ui.DiveType, SIGNAL(currentIndexChanged(int)), this, SLOT(divetype_Changed(int)));

	connect(ui.cylinders->view(), SIGNAL(clicked(QModelIndex)), this, SLOT(editCylinderWidget(QModelIndex)));
	connect(ui.weights->view(), SIGNAL(clicked(QModelIndex)), this, SLOT(editWeightWidget(QModelIndex)));

	ui.cylinders->view()->setItemDelegateForColumn(CylindersModel::TYPE, new TankInfoDelegate(this));
	ui.cylinders->view()->setItemDelegateForColumn(CylindersModel::USE, new TankUseDelegate(this));
	ui.weights->view()->setItemDelegateForColumn(WeightModel::TYPE, new WSInfoDelegate(this));
	ui.cylinders->view()->setColumnHidden(CylindersModel::DEPTH, true);
	completers.buddy = new QCompleter(&buddyModel, ui.buddy);
	completers.divemaster = new QCompleter(&diveMasterModel, ui.divemaster);
	completers.suit = new QCompleter(&suitModel, ui.suit);
	completers.tags = new QCompleter(&tagModel, ui.tagWidget);
	completers.buddy->setCaseSensitivity(Qt::CaseInsensitive);
	completers.divemaster->setCaseSensitivity(Qt::CaseInsensitive);
	completers.suit->setCaseSensitivity(Qt::CaseInsensitive);
	completers.tags->setCaseSensitivity(Qt::CaseInsensitive);
	ui.buddy->setCompleter(completers.buddy);
	ui.divemaster->setCompleter(completers.divemaster);
	ui.suit->setCompleter(completers.suit);
	ui.tagWidget->setCompleter(completers.tags);
	ui.diveNotesMessage->hide();
	ui.depth->hide();
	ui.depthLabel->hide();
	ui.duration->hide();
	ui.durationLabel->hide();
	setMinimumHeight(0);
	setMinimumWidth(0);

	// Current display of things on Gnome3 looks like shit, so
	// let`s fix that.
	if (isGnome3Session()) {
		QPalette p;
		p.setColor(QPalette::Window, QColor(Qt::white));
		ui.scrollArea->viewport()->setPalette(p);
		ui.scrollArea_2->viewport()->setPalette(p);

		// GroupBoxes in Gnome3 looks like I'v drawn them...
		static const QString gnomeCss(
			"QGroupBox {"
			"    background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
			"    stop: 0 #E0E0E0, stop: 1 #FFFFFF);"
			"    border: 2px solid gray;"
			"    border-radius: 5px;"
			"    margin-top: 1ex;"
			"}"
			"QGroupBox::title {"
			"    subcontrol-origin: margin;"
			"    subcontrol-position: top center;"
			"    padding: 0 3px;"
			"    background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
			"    stop: 0 #E0E0E0, stop: 1 #FFFFFF);"
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
	ui.cylinders->view()->horizontalHeader()->setContextMenuPolicy(Qt::ActionsContextMenu);
	ui.weights->view()->horizontalHeader()->setContextMenuPolicy(Qt::ActionsContextMenu);

	QSettings s;
	s.beginGroup("cylinders_dialog");
	for (int i = 0; i < CylindersModel::COLUMNS; i++) {
		if ((i == CylindersModel::REMOVE) || (i == CylindersModel::TYPE))
			continue;
		bool checked = s.value(QString("column%1_hidden").arg(i)).toBool();
		action = new QAction(cylindersModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString(), ui.cylinders->view());
		action->setCheckable(true);
		action->setData(i);
		action->setChecked(!checked);
		connect(action, SIGNAL(triggered(bool)), this, SLOT(toggleTriggeredColumn()));
		ui.cylinders->view()->setColumnHidden(i, checked);
		ui.cylinders->view()->horizontalHeader()->addAction(action);
	}

	connect(ui.diveNotesMessage, &KMessageWidget::showAnimationFinished,
					ui.location, &DiveLocationLineEdit::fixPopupPosition);

	// enable URL clickability in notes:
	new TextHyperlinkEventFilter(ui.notes);//destroyed when ui.notes is destroyed

	acceptingEdit = false;

	ui.diveTripLocation->hide();
}

MainTab::~MainTab()
{
	QSettings s;
	s.beginGroup("cylinders_dialog");
	for (int i = 0; i < CylindersModel::COLUMNS; i++) {
		if ((i == CylindersModel::REMOVE) || (i == CylindersModel::TYPE))
			continue;
		s.setValue(QString("column%1_hidden").arg(i), ui.cylinders->view()->isColumnHidden(i));
	}
}

void MainTab::toggleTriggeredColumn()
{
	QAction *action = qobject_cast<QAction *>(sender());
	int col = action->data().toInt();
	QTableView *view = ui.cylinders->view();

	if (action->isChecked()) {
		view->showColumn(col);
		if (view->columnWidth(col) <= 15)
			view->setColumnWidth(col, 80);
	} else
		view->hideColumn(col);
}

void MainTab::addDiveStarted()
{
	ui.tabWidget->setCurrentIndex(0);
	ui.tabWidget->setTabEnabled(2, false);
	ui.tabWidget->setTabEnabled(3, false);
	ui.tabWidget->setTabEnabled(4, false);
	ui.tabWidget->setTabEnabled(5, false);
	enableEdition(ADD);
}

void MainTab::addMessageAction(QAction *action)
{
	ui.diveNotesMessage->addAction(action);
}

void MainTab::hideMessage()
{
	ui.diveNotesMessage->animatedHide();
	updateTextLabels(false);
}

void MainTab::closeMessage()
{
	hideMessage();
	ui.diveNotesMessage->setCloseButtonVisible(false);
}

void MainTab::displayMessage(QString str)
{
	ui.diveNotesMessage->setCloseButtonVisible(false);
	ui.diveNotesMessage->setText(str);
	ui.diveNotesMessage->animatedShow();
	updateTextLabels();
}

void MainTab::updateTextLabels(bool showUnits)
{
	if (showUnits) {
		ui.airTempLabel->setText(tr("Air temp. [%1]").arg(get_temp_unit()));
		ui.waterTempLabel->setText(tr("Water temp. [%1]").arg(get_temp_unit()));
	} else {
		ui.airTempLabel->setText(tr("Air temp."));
		ui.waterTempLabel->setText(tr("Water temp."));
	}
}

void MainTab::enableEdition(EditMode newEditMode)
{
	const bool isTripEdit = MainWindow::instance() &&
		MainWindow::instance()->diveList->selectedTrips().count() == 1;

	if (((newEditMode == DIVE || newEditMode == NONE) && current_dive == NULL) || editMode != NONE)
		return;
	modified = false;
	copyPaste = false;
	if ((newEditMode == DIVE || newEditMode == NONE) &&
	    !isTripEdit &&
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

	if (isTripEdit) {
		// we are editing trip location and notes
		displayMessage(tr("This trip is being edited."));
		currentTrip = current_dive->divetrip;
		ui.dateEdit->setEnabled(false);
		editMode = TRIP;
	} else {
		ui.dateEdit->setEnabled(true);
		if (amount_selected > 1) {
			displayMessage(tr("Multiple dives are being edited."));
		} else {
			displayMessage(tr("This dive is being edited."));
		}
		editMode = newEditMode != NONE ? newEditMode : DIVE;
	}
}

// This function gets called if a field gets updated by an undo command.
// Refresh the corresponding UI field.
void MainTab::divesEdited(const QVector<dive *> &, DiveField field)
{
	// If there is no current dive, no point in updating anything
	if (!current_dive)
		return;

	switch(field) {
	case DiveField::AIR_TEMP:
		ui.airtemp->setText(get_temperature_string(current_dive->airtemp, true));
		break;
	case DiveField::WATER_TEMP:
		ui.watertemp->setText(get_temperature_string(current_dive->watertemp, true));
		break;
	case DiveField::RATING:
		ui.rating->setCurrentStars(current_dive->rating);
		break;
	case DiveField::VISIBILITY:
		ui.visibility->setCurrentStars(current_dive->visibility);
		break;
	case DiveField::SUIT:
		ui.suit->setText(QString(current_dive->suit));
		break;
	case DiveField::NOTES:
		updateNotes(current_dive);
		break;
	case DiveField::MODE:
		updateMode(current_dive);
		break;
	case DiveField::DATETIME:
		updateDateTime(current_dive);
		MainWindow::instance()->graphics->dateTimeChanged();
		DivePlannerPointsModel::instance()->getDiveplan().when = current_dive->when;
		break;
	case DiveField::DIVESITE:
		updateDiveSite(current_dive);
		emit diveSiteChanged();
		break;
	case DiveField::TAGS:
		ui.tagWidget->setText(get_taglist_string(current_dive->tag_list));
		break;
	default:
		break;
	}
}

void MainTab::clearEquipment()
{
	cylindersModel->clear();
	weightModel->clear();
}

void MainTab::nextInputField(QKeyEvent *event)
{
	keyPressEvent(event);
}

#define UPDATE_TEXT(d, field)          \
	if (clear || !d.field)         \
		ui.field->setText(QString()); \
	else                           \
		ui.field->setText(d.field)

#define UPDATE_TEMP(d, field)            \
	if (clear || d.field.mkelvin == 0) \
		ui.field->setText("");   \
	else                             \
		ui.field->setText(get_temperature_string(d.field, true))

bool MainTab::isEditing()
{
	return editMode != NONE;
}

void MainTab::updateDepthDuration()
{
	ui.depth->setVisible(true);
	ui.depthLabel->setVisible(true);
	ui.duration->setVisible(true);
	ui.durationLabel->setVisible(true);
	ui.duration->setText(render_seconds_to_string(displayed_dive.duration.seconds));
	ui.depth->setText(get_depth_string(displayed_dive.maxdepth, true));
}

void MainTab::updateNotes(const struct dive *d)
{
	QString tmp(d->notes);
	if (tmp.indexOf("<div") != -1) {
		tmp.replace(QString("\n"), QString("<br>"));
		ui.notes->setHtml(tmp);
	} else {
		ui.notes->setPlainText(tmp);
	}
}

void MainTab::updateMode(struct dive *d)
{
	ui.DiveType->setCurrentIndex(get_dive_dc(d, dc_number)->divemode);
	MainWindow::instance()->graphics->recalcCeiling();
}

void MainTab::updateDateTime(struct dive *d)
{
	// Subsurface always uses "local time" as in "whatever was the local time at the location"
	// so all time stamps have no time zone information and are in UTC
	QDateTime localTime = QDateTime::fromMSecsSinceEpoch(1000*d->when, Qt::UTC);
	localTime.setTimeSpec(Qt::UTC);
	ui.dateEdit->setDate(localTime.date());
	ui.timeEdit->setTime(localTime.time());
}

void MainTab::updateDiveSite(struct dive *d)
{
	struct dive_site *ds = NULL;
	ds = d->dive_site;
	if (ds) {
		ui.location->setCurrentDiveSite(ds);
		ui.locationTags->setText(constructLocationTags(&ds->taxonomy, true));

		if (ui.locationTags->text().isEmpty() && has_location(&ds->location)) {
			const char *coords = printGPSCoords(&ds->location);
			ui.locationTags->setText(coords);
			free((void *)coords);
		}
	} else {
		ui.location->clear();
		ui.locationTags->clear();
	}
}

void MainTab::updateDiveInfo(bool clear)
{
	ui.location->refreshDiveSiteCache();
	EditMode rememberEM = editMode;
	// don't execute this while adding / planning a dive
	if (editMode == ADD || editMode == MANUALLY_ADDED_DIVE || MainWindow::instance()->graphics->isPlanner())
		return;
	if (!isEnabled() && !clear )
		setEnabled(true);
	if (isEnabled() && clear)
		setEnabled(false);
	editMode = IGNORE; // don't trigger on changes to the widgets

	for (auto widget : extraWidgets) {
		widget->updateData();
	}

	ui.notes->setText(QString());
	if (!clear)
		updateNotes(&displayed_dive);
	UPDATE_TEXT(displayed_dive, suit);
	UPDATE_TEXT(displayed_dive, divemaster);
	UPDATE_TEXT(displayed_dive, buddy);
	UPDATE_TEMP(displayed_dive, airtemp);
	UPDATE_TEMP(displayed_dive, watertemp);
	updateMode(&displayed_dive);

	if (!clear) {
		updateDiveSite(&displayed_dive);
		updateDateTime(&displayed_dive);
		if (MainWindow::instance() && MainWindow::instance()->diveList->selectedTrips().count() == 1) {
			// Remember the tab selected for last dive
			if (lastSelectedDive)
				lastTabSelectedDive = ui.tabWidget->currentIndex();
			ui.tabWidget->setTabText(0, tr("Trip notes"));
			ui.tabWidget->setTabEnabled(1, false);
			ui.tabWidget->setTabEnabled(2, false);
			ui.tabWidget->setTabEnabled(5, false);
			// Recover the tab selected for last dive trip
			if (lastSelectedDive)
				ui.tabWidget->setCurrentIndex(lastTabSelectedDiveTrip);
			lastSelectedDive = false;
			currentTrip = *MainWindow::instance()->diveList->selectedTrips().begin();
			// only use trip relevant fields
			ui.divemaster->setVisible(false);
			ui.DivemasterLabel->setVisible(false);
			ui.buddy->setVisible(false);
			ui.BuddyLabel->setVisible(false);
			ui.suit->setVisible(false);
			ui.SuitLabel->setVisible(false);
			ui.rating->setVisible(false);
			ui.RatingLabel->setVisible(false);
			ui.visibility->setVisible(false);
			ui.visibilityLabel->setVisible(false);
			ui.tagWidget->setVisible(false);
			ui.TagLabel->setVisible(false);
			ui.airTempLabel->setVisible(false);
			ui.airtemp->setVisible(false);
			ui.DiveType->setVisible(false);
			ui.TypeLabel->setVisible(false);
			ui.waterTempLabel->setVisible(false);
			ui.watertemp->setVisible(false);
			ui.dateEdit->setReadOnly(true);
			ui.timeLabel->setVisible(false);
			ui.timeEdit->setVisible(false);
			ui.diveTripLocation->show();
			ui.location->hide();
			ui.editDiveSiteButton->hide();
			// rename the remaining fields and fill data from selected trip
			ui.LocationLabel->setText(tr("Trip location"));
			ui.diveTripLocation->setText(currentTrip->location);
			ui.locationTags->clear();
			//TODO: Fix this.
			//ui.location->setText(currentTrip->location);
			ui.NotesLabel->setText(tr("Trip notes"));
			ui.notes->setText(currentTrip->notes);
			clearEquipment();
			ui.equipmentTab->setEnabled(false);
			ui.depth->setVisible(false);
			ui.depthLabel->setVisible(false);
			ui.duration->setVisible(false);
			ui.durationLabel->setVisible(false);
		} else {
			// Remember the tab selected for last dive trip
			if (!lastSelectedDive)
				lastTabSelectedDiveTrip = ui.tabWidget->currentIndex();
			ui.tabWidget->setTabText(0, tr("Notes"));
			ui.tabWidget->setTabEnabled(1, true);
			ui.tabWidget->setTabEnabled(2, true);
			ui.tabWidget->setTabEnabled(3, true);
			ui.tabWidget->setTabEnabled(4, true);
			ui.tabWidget->setTabEnabled(5, true);
			// Recover the tab selected for last dive
			if (!lastSelectedDive)
				ui.tabWidget->setCurrentIndex(lastTabSelectedDive);
			lastSelectedDive = true;
			currentTrip = NULL;
			// make all the fields visible writeable
			ui.diveTripLocation->hide();
			ui.location->show();
			ui.editDiveSiteButton->show();
			ui.divemaster->setVisible(true);
			ui.buddy->setVisible(true);
			ui.suit->setVisible(true);
			ui.SuitLabel->setVisible(true);
			ui.rating->setVisible(true);
			ui.RatingLabel->setVisible(true);
			ui.visibility->setVisible(true);
			ui.visibilityLabel->setVisible(true);
			ui.BuddyLabel->setVisible(true);
			ui.DivemasterLabel->setVisible(true);
			ui.TagLabel->setVisible(true);
			ui.tagWidget->setVisible(true);
			ui.airTempLabel->setVisible(true);
			ui.airtemp->setVisible(true);
			ui.TypeLabel->setVisible(true);
			ui.DiveType->setVisible(true);
			ui.waterTempLabel->setVisible(true);
			ui.watertemp->setVisible(true);
			ui.dateEdit->setReadOnly(false);
			ui.timeLabel->setVisible(true);
			ui.timeEdit->setVisible(true);
			/* and fill them from the dive */
			ui.rating->setCurrentStars(displayed_dive.rating);
			ui.visibility->setCurrentStars(displayed_dive.visibility);
			// reset labels in case we last displayed trip notes
			ui.LocationLabel->setText(tr("Location"));
			ui.NotesLabel->setText(tr("Notes"));
			ui.equipmentTab->setEnabled(true);
			cylindersModel->updateDive();
			weightModel->updateDive();
			ui.tagWidget->setText(get_taglist_string(displayed_dive.tag_list));
			if (current_dive) {
				bool isManual = same_string(current_dive->dc.model, "manually added dive");
				ui.depth->setVisible(isManual);
				ui.depthLabel->setVisible(isManual);
				ui.duration->setVisible(isManual);
				ui.durationLabel->setVisible(isManual);
			}
		}
		ui.duration->setText(render_seconds_to_string(displayed_dive.duration.seconds));
		ui.depth->setText(get_depth_string(displayed_dive.maxdepth, true));

		volume_t gases[MAX_CYLINDERS] = {};
		get_gas_used(&displayed_dive, gases);
		int mean[MAX_CYLINDERS], duration[MAX_CYLINDERS];
		per_cylinder_mean_depth(&displayed_dive, select_dc(&displayed_dive), mean, duration);

		// now let's get some gas use statistics
		QVector<QPair<QString, int> > gasUsed;
		QString gasUsedString;
		volume_t vol;
		selectedDivesGasUsed(gasUsed);
		for (int j = 0; j < MAX_CYLINDERS; j++) {
			if (gasUsed.isEmpty())
				break;
			QPair<QString, int> gasPair = gasUsed.last();
			gasUsed.pop_back();
			vol.mliter = gasPair.second;
			gasUsedString.append(gasPair.first).append(": ").append(get_volume_string(vol, true)).append("\n");
		}
		if (!gasUsed.isEmpty())
			gasUsedString.append("...");
		volume_t o2_tot = {}, he_tot = {};
		selected_dives_gas_parts(&o2_tot, &he_tot);

		if(ui.locationTags->text().isEmpty())
			ui.locationTags->hide();
		else
			ui.locationTags->show();
		ui.editDiveSiteButton->setEnabled(!ui.location->text().isEmpty());
		/* unset the special value text for date and time, just in case someone dove at midnight */
		ui.dateEdit->setSpecialValueText(QString(""));
		ui.timeEdit->setSpecialValueText(QString(""));
	} else {
		/* clear the fields */
		clearTabs();
		ui.rating->setCurrentStars(0);
		ui.visibility->setCurrentStars(0);
		ui.location->clear();
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
	ui.cylinders->view()->hideColumn(CylindersModel::DEPTH);
	if (get_dive_dc(&displayed_dive, dc_number)->divemode == CCR)
		ui.cylinders->view()->showColumn(CylindersModel::USE);
	else
		ui.cylinders->view()->hideColumn(CylindersModel::USE);

	if (verbose && displayed_dive.dive_site)
		qDebug() << "Set the current dive site:" << displayed_dive.dive_site->uuid;
	emit diveSiteChanged();
}

void MainTab::addCylinder_clicked()
{
	if (editMode == NONE)
		enableEdition();
	cylindersModel->add();
}

void MainTab::addWeight_clicked()
{
	if (editMode == NONE)
		enableEdition();
	weightModel->add();
}

void MainTab::reload()
{
	suitModel.updateModel();
	buddyModel.updateModel();
	diveMasterModel.updateModel();
	tagModel.updateModel();
	LocationInformationModel::instance()->update();
}

// tricky little macro to edit all the selected dives
// loop ove all DIVES and do WHAT.
#define MODIFY_DIVES(DIVES, WHAT)                            \
	do {                                                 \
		for (dive *mydive: DIVES) {                  \
			WHAT;                                \
		}					     \
		mark_divelist_changed(true);                 \
	} while (0)

#define EDIT_TEXT(what)                                          \
	if (same_string(mydive->what, cd->what) || copyPaste) {  \
		free(mydive->what);                              \
		mydive->what = copy_string(displayed_dive.what); \
	}

MainTab::EditMode MainTab::getEditMode() const
{
	return editMode;
}

#define EDIT_VALUE(what)                             \
	if (mydive->what == cd->what || copyPaste) { \
		mydive->what = displayed_dive.what;  \
	}

void MainTab::refreshDisplayedDiveSite()
{
	struct dive_site *ds = get_dive_site_for_dive(&displayed_dive);
	if (ds)
		ui.location->setCurrentDiveSite(ds);
}

// when this is called we already have updated the current_dive and know that it exists
// there is no point in calling this function if there is no current dive
struct dive_site *MainTab::getDiveSite(struct dive_site *pickedDs, struct dive_site *origDs)
{
	if (ui.location->text().isEmpty())
		return 0;

	if (!pickedDs)
		return 0;

	if (pickedDs == origDs)
		return origDs;

	bool createdNewDive = false;

	if (pickedDs == RECENTLY_ADDED_DIVESITE) {
		QString name = ui.location->text().isEmpty() ? tr("New dive site") : ui.location->text();
		pickedDs = create_dive_site(qPrintable(name), displayed_dive.when);
		createdNewDive = true;
	}

	if (origDs) {
		if(createdNewDive) {
			uint32_t pickedUuid = pickedDs->uuid;
			copy_dive_site(origDs, pickedDs);
			free(pickedDs->name);
			pickedDs->name = copy_qstring(ui.location->text());
			pickedDs->uuid = pickedUuid;
			qDebug() << "Creating and copying dive site";
		} else if (!has_location(&pickedDs->location)) {
			pickedDs->location = origDs->location;
			qDebug() << "Copying GPS information";
		}
	}

	qDebug() << "Setting the dive site id on the dive:" << pickedDs->uuid;
	return pickedDs;
}

// Get the list of selected dives, but put the current dive at the last position of the vector
static QVector<dive *> getSelectedDivesCurrentLast()
{
	QVector<dive *> res;
	struct dive *d;
	int i;
	for_each_dive (i, d) {
		if (d->selected && d != current_dive)
			res.append(d);
	}
	res.append(current_dive);
	return res;
}

void MainTab::acceptChanges()
{
	int i, addedId = -1;
	struct dive *d;
	bool do_replot = false;

	if (ui.location->hasFocus())
		setFocus();

	acceptingEdit = true;
	tabBar()->setTabIcon(0, QIcon()); // Notes
	tabBar()->setTabIcon(1, QIcon()); // Equipment
	ui.dateEdit->setEnabled(true);
	hideMessage();
	ui.equipmentTab->setEnabled(true);
	if (editMode == ADD) {
		// make sure that the dive site is handled as well
		displayed_dive.dive_site = getDiveSite(ui.location->currDiveSite(), displayed_dive.dive_site);
		copyTagsToDisplayedDive();

		Command::addDive(&displayed_dive, autogroup, true);

		editMode = NONE;
		MainWindow::instance()->exitEditState();
		cylindersModel->changed = false;
		weightModel->changed = false;
		MainWindow::instance()->setEnabledToolbar(true);
		acceptingEdit = false;
		ui.editDiveSiteButton->setEnabled(!ui.location->text().isEmpty());
		emit addDiveFinished();
		DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::NOTHING);
		MainWindow::instance()->diveList->setFocus();
		resetPallete();
		displayed_dive.divetrip = nullptr; // Should not be necessary, just in case!
		return;
	} else if (MainWindow::instance() && MainWindow::instance()->diveList->selectedTrips().count() == 1) {
		/* now figure out if things have changed */
		if (displayedTrip.notes && !same_string(displayedTrip.notes, currentTrip->notes)) {
			currentTrip->notes = copy_string(displayedTrip.notes);
			mark_divelist_changed(true);
		}
		if (displayedTrip.location && !same_string(displayedTrip.location, currentTrip->location)) {
			currentTrip->location = copy_string(displayedTrip.location);
			mark_divelist_changed(true);
		}
		currentTrip = NULL;
		ui.dateEdit->setEnabled(true);
	} else {
		// Get list of selected dives, but put the current dive last;
		// this is required in case the invocation wants to compare things
		// to the original value in current_dive like it should
		QVector<dive *> selectedDives = getSelectedDivesCurrentLast();
		if (editMode == MANUALLY_ADDED_DIVE) {
			// preserve any changes to the profile
			free(current_dive->dc.sample);
			copy_samples(&displayed_dive.dc, &current_dive->dc);
			addedId = displayed_dive.id;
		}
		// now check if something has changed and if yes, edit the selected dives that
		// were identical with the master dive shown (and mark the divelist as changed)
		struct dive *cd = current_dive;

		// three text fields are somewhat special and are represented as tags
		// in the UI - they need somewhat smarter handling
		saveTaggedStrings(selectedDives);

		if (cylindersModel->changed) {
			mark_divelist_changed(true);
			MODIFY_DIVES(selectedDives,
				for (int i = 0; i < MAX_CYLINDERS; i++) {
					if (mydive != cd) {
						if (same_string(mydive->cylinder[i].type.description, cd->cylinder[i].type.description) || copyPaste) {
							// if we started out with the same cylinder description (for multi-edit) or if we do copt & paste
							// make sure that we have the same cylinder type and copy the gasmix, but DON'T copy the start
							// and end pressures (those are per dive after all)
							if (!same_string(mydive->cylinder[i].type.description, displayed_dive.cylinder[i].type.description)) {
								free((void*)mydive->cylinder[i].type.description);
								mydive->cylinder[i].type.description = copy_string(displayed_dive.cylinder[i].type.description);
							}
							mydive->cylinder[i].type.size = displayed_dive.cylinder[i].type.size;
							mydive->cylinder[i].type.workingpressure = displayed_dive.cylinder[i].type.workingpressure;
							mydive->cylinder[i].gasmix = displayed_dive.cylinder[i].gasmix;
							mydive->cylinder[i].cylinder_use = displayed_dive.cylinder[i].cylinder_use;
							mydive->cylinder[i].depth = displayed_dive.cylinder[i].depth;
						}
					}
				}
			);
			for (int i = 0; i < MAX_CYLINDERS; i++) {
				// copy the cylinder but make sure we have our own copy of the strings
				free((void*)cd->cylinder[i].type.description);
				cd->cylinder[i] = displayed_dive.cylinder[i];
				cd->cylinder[i].type.description = copy_string(displayed_dive.cylinder[i].type.description);
			}
			/* if cylinders changed we may have changed gas change events
			 * and sensor idx in samples as well
			 * - so far this is ONLY supported for a single selected dive */
			struct divecomputer *tdc = &current_dive->dc;
			struct divecomputer *sdc = &displayed_dive.dc;
			while(tdc && sdc) {
				free_events(tdc->events);
				copy_events(sdc, tdc);
				free(tdc->sample);
				copy_samples(sdc, tdc);
				tdc = tdc->next;
				sdc = sdc->next;
			}
			do_replot = true;
		}

		if (weightModel->changed) {
			mark_divelist_changed(true);
			MODIFY_DIVES(selectedDives,
				for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++) {
					if (mydive != cd && (copyPaste || same_string(mydive->weightsystem[i].description, cd->weightsystem[i].description))) {
						mydive->weightsystem[i] = displayed_dive.weightsystem[i];
						mydive->weightsystem[i].description = copy_string(displayed_dive.weightsystem[i].description);
					}
				}
			);
			for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++) {
				cd->weightsystem[i] = displayed_dive.weightsystem[i];
				cd->weightsystem[i].description = copy_string(displayed_dive.weightsystem[i].description);
			}
		}

		// each dive that was selected might have had the temperatures in its active divecomputer changed
		// so re-populate the temperatures - easiest way to do this is by calling fixup_dive
		for_each_dive (i, d) {
			if (d->selected) {
				fixup_dive(d);
				invalidate_dive_cache(d);
			}
		}
	}
	if (editMode == MANUALLY_ADDED_DIVE) {
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
	resetPallete();
	if (editMode == MANUALLY_ADDED_DIVE) {
		MainWindow::instance()->diveList->reload();
		int newDiveNr = get_divenr(get_dive_by_uniq_id(addedId));
		MainWindow::instance()->diveList->unselectDives();
		MainWindow::instance()->diveList->selectDive(newDiveNr, true);
		editMode = NONE;
		MainWindow::instance()->refreshDisplay();
		MainWindow::instance()->graphics->replot();
	} else {
		editMode = NONE;
		if (do_replot)
			MainWindow::instance()->graphics->replot();
		MainWindow::instance()->diveList->rememberSelection();
		MainWindow::instance()->refreshDisplay();
		MainWindow::instance()->diveList->restoreSelection();
	}
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::NOTHING);
	MainWindow::instance()->diveList->verticalScrollBar()->setSliderPosition(scrolledBy);
	MainWindow::instance()->diveList->setFocus();
	MainWindow::instance()->exitEditState();
	cylindersModel->changed = false;
	weightModel->changed = false;
	MainWindow::instance()->setEnabledToolbar(true);
	acceptingEdit = false;
	ui.editDiveSiteButton->setEnabled(!ui.location->text().isEmpty());
}

void MainTab::resetPallete()
{
	QPalette p;
	ui.buddy->setPalette(p);
	ui.notes->setPalette(p);
	ui.location->setPalette(p);
	ui.divemaster->setPalette(p);
	ui.suit->setPalette(p);
	ui.airtemp->setPalette(p);
	ui.DiveType->setPalette(p);
	ui.watertemp->setPalette(p);
	ui.dateEdit->setPalette(p);
	ui.timeEdit->setPalette(p);
	ui.tagWidget->setPalette(p);
	ui.diveTripLocation->setPalette(p);
	ui.duration->setPalette(p);
	ui.depth->setPalette(p);
}

void MainTab::rejectChanges()
{
	EditMode lastMode = editMode;

	if (lastMode != NONE && current_dive &&
	    (modified ||
	     memcmp(&current_dive->cylinder[0], &displayed_dive.cylinder[0], sizeof(cylinder_t) * MAX_CYLINDERS) ||
	     memcmp(&current_dive->weightsystem[0], &displayed_dive.weightsystem[0], sizeof(weightsystem_t) * MAX_WEIGHTSYSTEMS))) {
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
	resetPallete();
	// no harm done to call cancelPlan even if we were not in ADD or PLAN mode...
	DivePlannerPointsModel::instance()->cancelPlan();
	if(lastMode == ADD)
		MainWindow::instance()->diveList->restoreSelection();

	// now make sure that the correct dive is displayed
	if (current_dive)
		copy_dive(current_dive, &displayed_dive);
	else
		clear_dive(&displayed_dive);
	updateDiveInfo(!current_dive);

	for (auto widget : extraWidgets) {
		widget->updateData();
	}
	// the user could have edited the location and then canceled the edit
	// let's get the correct location back in view
	MapWidget::instance()->centerOnDiveSite(displayed_dive.dive_site);
	// show the profile and dive info
	MainWindow::instance()->graphics->replot();
	MainWindow::instance()->setEnabledToolbar(true);
	MainWindow::instance()->exitEditState();
	cylindersModel->changed = false;
	weightModel->changed = false;
	cylindersModel->updateDive();
	weightModel->updateDive();
	ui.editDiveSiteButton->setEnabled(!ui.location->text().isEmpty());
}

void MainTab::markChangedWidget(QWidget *w)
{
	QPalette p;
	qreal h, s, l, a;
	enableEdition();
	qApp->palette().color(QPalette::Text).getHslF(&h, &s, &l, &a);
	p.setBrush(QPalette::Base, (l <= 0.3) ? QColor(Qt::yellow).lighter() : (l <= 0.6) ? QColor(Qt::yellow).light() : /* else */ QColor(Qt::yellow).darker(300));
	w->setPalette(p);
	modified = true;
}

void MainTab::on_buddy_textChanged()
{
	if (editMode == IGNORE || acceptingEdit == true)
		return;

	if (same_string(displayed_dive.buddy, qPrintable(ui.buddy->toPlainText())))
		return;

	QStringList text_list = ui.buddy->toPlainText().split(",", QString::SkipEmptyParts);
	for (int i = 0; i < text_list.size(); i++)
		text_list[i] = text_list[i].trimmed();
	QString text = text_list.join(", ");
	free(displayed_dive.buddy);
	displayed_dive.buddy = copy_qstring(text);
	markChangedWidget(ui.buddy);
}

void MainTab::on_divemaster_textChanged()
{
	if (editMode == IGNORE || acceptingEdit == true)
		return;

	if (same_string(displayed_dive.divemaster, qPrintable(ui.divemaster->toPlainText())))
		return;

	QStringList text_list = ui.divemaster->toPlainText().split(",", QString::SkipEmptyParts);
	for (int i = 0; i < text_list.size(); i++)
		text_list[i] = text_list[i].trimmed();
	QString text = text_list.join(", ");
	free(displayed_dive.divemaster);
	displayed_dive.divemaster = copy_qstring(text);
	markChangedWidget(ui.divemaster);
}

void MainTab::on_duration_textChanged(const QString &text)
{
	if (editMode == IGNORE || acceptingEdit == true)
		return;
	// parse this
	MainWindow::instance()->graphics->setReplot(false);
	if (!isEditing())
		enableEdition();
	displayed_dive.dc.duration.seconds = parseDurationToSeconds(text);
	displayed_dive.duration = displayed_dive.dc.duration;
	displayed_dive.dc.meandepth.mm = 0;
	displayed_dive.dc.samples = 0;
	DivePlannerPointsModel::instance()->loadFromDive(&displayed_dive);
	markChangedWidget(ui.duration);
	MainWindow::instance()->graphics->setReplot(true);
	MainWindow::instance()->graphics->plotDive();

}

void MainTab::on_depth_textChanged(const QString &text)
{
	if (editMode == IGNORE || acceptingEdit == true)
		return;
	// don't replot until we set things up the way we want them
	MainWindow::instance()->graphics->setReplot(false);
	if (!isEditing())
		enableEdition();
	displayed_dive.dc.maxdepth.mm = parseLengthToMm(text);
	displayed_dive.maxdepth = displayed_dive.dc.maxdepth;
	displayed_dive.dc.meandepth.mm = 0;
	displayed_dive.dc.samples = 0;
	DivePlannerPointsModel::instance()->loadFromDive(&displayed_dive);
	markChangedWidget(ui.depth);
	MainWindow::instance()->graphics->setReplot(true);
	MainWindow::instance()->graphics->plotDive();
}

void MainTab::on_airtemp_editingFinished()
{
	if (editMode == IGNORE || acceptingEdit == true || !current_dive)
		return;
	Command::editAirTemp(getSelectedDivesCurrentLast(),
			     parseTemperatureToMkelvin(ui.airtemp->text()), current_dive->airtemp.mkelvin);
}

void MainTab::divetype_Changed(int index)
{
	if (editMode == IGNORE || !current_dive)
		return;
	Command::editMode(getSelectedDivesCurrentLast(), dc_number, (enum divemode_t)index,
			  get_dive_dc(current_dive, dc_number)->divemode);
}

void MainTab::on_watertemp_editingFinished()
{
	if (editMode == IGNORE || acceptingEdit == true || !current_dive)
		return;
	Command::editWaterTemp(getSelectedDivesCurrentLast(),
			       parseTemperatureToMkelvin(ui.watertemp->text()),
			       current_dive->watertemp.mkelvin);
}

// Editing of the dive time is different. If multiple dives are edited,
// all dives are shifted by an offset.
static void shiftTime(QDateTime &dateTime)
{
	timestamp_t when = dateTime.toTime_t();
	if (current_dive && current_dive->when != when) {
		timestamp_t offset = current_dive->when - when;
		Command::shiftTime(getSelectedDivesCurrentLast(), (int)offset);
	}
}

void MainTab::on_dateEdit_dateChanged(const QDate &date)
{
	if (editMode == IGNORE || acceptingEdit == true || !current_dive)
		return;
	QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(1000*current_dive->when, Qt::UTC);
	dateTime.setTimeSpec(Qt::UTC);
	dateTime.setDate(date);
	shiftTime(dateTime);
}

void MainTab::on_timeEdit_timeChanged(const QTime &time)
{
	if (editMode == IGNORE || acceptingEdit == true || !current_dive)
		return;
	QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(1000*current_dive->when, Qt::UTC);
	dateTime.setTimeSpec(Qt::UTC);
	dateTime.setTime(time);
	shiftTime(dateTime);
}

void MainTab::copyTagsToDisplayedDive()
{
	taglist_free(displayed_dive.tag_list);
	displayed_dive.tag_list = NULL;
	Q_FOREACH (const QString &tag, ui.tagWidget->getBlockStringList())
		taglist_add_tag(&displayed_dive.tag_list, qPrintable(tag));
	taglist_cleanup(&displayed_dive.tag_list);
}

// buddy and divemaster are represented in the UI just like the tags, but the internal
// representation is just a string (with commas as delimiters). So we need to do the same
// thing we did for tags, just differently
void MainTab::saveTaggedStrings(const QVector<dive *> &selectedDives)
{
	QStringList addedList, removedList;
	struct dive *cd = current_dive;

	if (diffTaggedStrings(cd->buddy, displayed_dive.buddy, addedList, removedList))
		MODIFY_DIVES(selectedDives,
			QStringList oldList = QString(mydive->buddy).split(QRegExp("\\s*,\\s*"), QString::SkipEmptyParts);
			QString newString;
			QString comma;
			Q_FOREACH (const QString tag, oldList) {
				if (!removedList.contains(tag, Qt::CaseInsensitive)) {
					newString += comma + tag;
					comma = ", ";
				}
			}
			Q_FOREACH (const QString tag, addedList) {
				if (!oldList.contains(tag, Qt::CaseInsensitive)) {
					newString += comma + tag;
					comma = ", ";
				}
			}
			free(mydive->buddy);
			mydive->buddy = copy_qstring(newString);
		);
	addedList.clear();
	removedList.clear();
	if (diffTaggedStrings(cd->divemaster, displayed_dive.divemaster, addedList, removedList))
		MODIFY_DIVES(selectedDives,
			QStringList oldList = QString(mydive->divemaster).split(QRegExp("\\s*,\\s*"), QString::SkipEmptyParts);
			QString newString;
			QString comma;
			Q_FOREACH (const QString tag, oldList) {
				if (!removedList.contains(tag, Qt::CaseInsensitive)) {
					newString += comma + tag;
					comma = ", ";
				}
			}
			Q_FOREACH (const QString tag, addedList) {
				if (!oldList.contains(tag, Qt::CaseInsensitive)) {
					newString += comma + tag;
					comma = ", ";
				}
			}
			free(mydive->divemaster);
			mydive->divemaster = copy_qstring(newString);
		);
}

int MainTab::diffTaggedStrings(QString currentString, QString displayedString, QStringList &addedList, QStringList &removedList)
{
	QStringList displayedList, currentList;
	currentList = currentString.split(',', QString::SkipEmptyParts);
	displayedList = displayedString.split(',', QString::SkipEmptyParts);
	Q_FOREACH ( const QString tag, currentList) {
		if (!displayedList.contains(tag, Qt::CaseInsensitive))
			removedList << tag.trimmed();
	}
	Q_FOREACH (const QString tag, displayedList) {
		if (!currentList.contains(tag, Qt::CaseInsensitive))
			addedList << tag.trimmed();
	}
	return removedList.length() + addedList.length();
}

void MainTab::on_tagWidget_editingFinished()
{
	if (editMode == IGNORE || acceptingEdit == true || !current_dive)
		return;

	Command::editTags(getSelectedDivesCurrentLast(), ui.tagWidget->getBlockStringList(), current_dive);
}

void MainTab::on_location_diveSiteSelected()
{
	if (editMode == IGNORE || acceptingEdit == true || !current_dive)
		return;

	struct dive_site *newDs = getDiveSite(ui.location->currDiveSite(), current_dive->dive_site);
	Command::editDiveSite(getSelectedDivesCurrentLast(), newDs, current_dive->dive_site);
}

void MainTab::on_diveTripLocation_textEdited(const QString& text)
{
	if (currentTrip) {
		free(displayedTrip.location);
		displayedTrip.location = copy_qstring(text);
		markChangedWidget(ui.diveTripLocation);
	}
}

void MainTab::on_suit_editingFinished()
{
	if (editMode == IGNORE || acceptingEdit == true || !current_dive)
		return;

	Command::editSuit(getSelectedDivesCurrentLast(), ui.suit->text(), QString(current_dive->suit));
}

void MainTab::on_notes_textChanged()
{
	if (editMode == IGNORE || acceptingEdit == true)
		return;
	if (currentTrip) {
		if (same_string(displayedTrip.notes, qPrintable(ui.notes->toPlainText())))
			return;
		free(displayedTrip.notes);
		displayedTrip.notes = copy_qstring(ui.notes->toPlainText());
		markChangedWidget(ui.notes);
	}
}

void MainTab::on_notes_editingFinished()
{
	if (currentTrip || !current_dive)
		return; // Trip-note editing is done via on_notes_textChanged()

	QString notes = ui.notes->toHtml().indexOf("<div") != -1 ?
		ui.notes->toHtml() : ui.notes->toPlainText();

	Command::editNotes(getSelectedDivesCurrentLast(), notes, QString(current_dive->notes));
}

void MainTab::on_rating_valueChanged(int value)
{
	if (acceptingEdit == true || !current_dive)
		return;

	Command::editRating(getSelectedDivesCurrentLast(), value, current_dive->rating);
}

void MainTab::on_visibility_valueChanged(int value)
{
	if (acceptingEdit == true || !current_dive)
		return;

	Command::editVisibility(getSelectedDivesCurrentLast(), value, current_dive->visibility);
}

#undef MODIFY_DIVES
#undef EDIT_TEXT
#undef EDIT_VALUE

void MainTab::editCylinderWidget(const QModelIndex &index)
{
	// we need a local copy or bad things happen when enableEdition() is called
	QModelIndex editIndex = index;
	if (cylindersModel->changed && editMode == NONE) {
		enableEdition();
		return;
	}
	if (editIndex.isValid() && editIndex.column() != CylindersModel::REMOVE) {
		if (editMode == NONE)
			enableEdition();
		ui.cylinders->edit(editIndex);
	}
}

void MainTab::editWeightWidget(const QModelIndex &index)
{
	if (editMode == NONE)
		enableEdition();

	if (index.isValid() && index.column() != WeightModel::REMOVE)
		ui.weights->edit(index);
}

void MainTab::escDetected()
{
	// In edit mode, pressing escape cancels the current changes.
	// In standard mode, remove focus of any active widget to
	if (editMode != NONE)
		rejectChanges();
	else
		setFocus();
}

void MainTab::clearTabs() {
	for (auto widget : extraWidgets) {
		widget->clear();
	}
	clearEquipment();
}

#define SHOW_SELECTIVE(_component) \
	if (what._component)       \
		ui._component->setText(displayed_dive._component);

void MainTab::showAndTriggerEditSelective(struct dive_components what)
{
	// take the data in our copyPasteDive and apply it to selected dives
	enableEdition();
	copyPaste = true;
	SHOW_SELECTIVE(buddy);
	SHOW_SELECTIVE(divemaster);
	SHOW_SELECTIVE(suit);
	if (what.notes) {
		QString tmp(displayed_dive.notes);
		if (tmp.contains("<div")) {
			tmp.replace(QString("\n"), QString("<br>"));
			ui.notes->setHtml(tmp);
		} else {
			ui.notes->setPlainText(tmp);
		}
	}
	if (what.rating)
		ui.rating->setCurrentStars(displayed_dive.rating);
	if (what.visibility)
		ui.visibility->setCurrentStars(displayed_dive.visibility);
	if (what.divesite)
		ui.location->setCurrentDiveSite(displayed_dive.dive_site);
	if (what.tags) {
		ui.tagWidget->setText(get_taglist_string(displayed_dive.tag_list));
	}
	if (what.cylinders) {
		cylindersModel->updateDive();
		cylindersModel->changed = true;
	}
	if (what.weights) {
		weightModel->updateDive();
		weightModel->changed = true;
	}
}
