/*
 * maintab.cpp
 *
 * classes for the "notebook" area of the main window of Subsurface
 *
 */
#include "maintab.h"
#include "mainwindow.h"
#include "../helpers.h"
#include "../statistics.h"
#include "divelistview.h"
#include "modeldelegates.h"
#include "globe.h"
#include "completionmodels.h"
#include "diveplanner.h"
#include "divelist.h"
#include "qthelper.h"

#include <QLabel>
#include <QCompleter>
#include <QDebug>
#include <QSet>
#include <QSettings>
#include <QTableView>
#include <QPalette>
#include <QScrollBar>

MainTab::MainTab(QWidget *parent) : QTabWidget(parent),
				    weightModel(new WeightModel(this)),
				    cylindersModel(CylindersModel::instance()),
				    editMode(NONE)
{
	ui.setupUi(this);
	ui.tagWidget->setFocusPolicy(Qt::StrongFocus); // Don't get focus by 'Wheel'
	ui.cylinders->setModel(cylindersModel);
	ui.weights->setModel(weightModel);
	closeMessage();

	QAction *action = new QAction(tr("Save"), this);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(acceptChanges()));
	addMessageAction(action);

	action = new QAction(tr("Cancel"), this);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(rejectChanges()));
	addMessageAction(action);

	if (qApp->style()->objectName() == "oxygen")
		setDocumentMode(true);
	else
		setDocumentMode(false);

	// we start out with the fields read-only; once things are
	// filled from a dive, they are made writeable
	setEnabled(false);

	ui.location->installEventFilter(this);
	ui.coordinates->installEventFilter(this);
	ui.divemaster->installEventFilter(this);
	ui.buddy->installEventFilter(this);
	ui.suit->installEventFilter(this);
	ui.notes->viewport()->installEventFilter(this);
	ui.rating->installEventFilter(this);
	ui.visibility->installEventFilter(this);
	ui.airtemp->installEventFilter(this);
	ui.watertemp->installEventFilter(this);
	ui.dateTimeEdit->installEventFilter(this);
	ui.tagWidget->installEventFilter(this);

	QList<QObject *> statisticsTabWidgets = ui.statisticsTab->children();
	Q_FOREACH(QObject* obj, statisticsTabWidgets) {
		QLabel* label = qobject_cast<QLabel *>(obj);
		if (label)
			label->setAlignment(Qt::AlignHCenter);
	}
	ui.cylinders->setTitle(tr("Cylinders"));
	ui.cylinders->setBtnToolTip(tr("Add Cylinder"));
	connect(ui.cylinders, SIGNAL(addButtonClicked()), this, SLOT(addCylinder_clicked()));

	ui.weights->setTitle(tr("Weights"));
	ui.weights->setBtnToolTip(tr("Add Weight System"));
	connect(ui.weights, SIGNAL(addButtonClicked()), this, SLOT(addWeight_clicked()));

	connect(ui.cylinders->view(), SIGNAL(clicked(QModelIndex)), this, SLOT(editCylinderWidget(QModelIndex)));
	connect(ui.weights->view(), SIGNAL(clicked(QModelIndex)), this, SLOT(editWeightWidget(QModelIndex)));

	ui.cylinders->view()->setItemDelegateForColumn(CylindersModel::TYPE, new TankInfoDelegate(this));
	ui.weights->view()->setItemDelegateForColumn(WeightModel::TYPE, new WSInfoDelegate(this));
	ui.cylinders->view()->setColumnHidden(CylindersModel::DEPTH, true);
	completers.buddy = new QCompleter(BuddyCompletionModel::instance(), ui.buddy);
	completers.divemaster = new QCompleter(DiveMasterCompletionModel::instance(), ui.divemaster);
	completers.location = new QCompleter(LocationCompletionModel::instance(), ui.location);
	completers.suit = new QCompleter(SuitCompletionModel::instance(), ui.suit);
	completers.tags = new QCompleter(TagCompletionModel::instance(), ui.tagWidget);
	completers.buddy->setCaseSensitivity(Qt::CaseInsensitive);
	completers.divemaster->setCaseSensitivity(Qt::CaseInsensitive);
	completers.location->setCaseSensitivity(Qt::CaseInsensitive);
	completers.suit->setCaseSensitivity(Qt::CaseInsensitive);
	completers.tags->setCaseSensitivity(Qt::CaseInsensitive);
	ui.buddy->setCompleter(completers.buddy);
	ui.divemaster->setCompleter(completers.divemaster);
	ui.location->setCompleter(completers.location);
	ui.suit->setCompleter(completers.suit);
	ui.tagWidget->setCompleter(completers.tags);

	setMinimumHeight(0);
	setMinimumWidth(0);

	// Current display of things on Gnome3 looks like shit, so
	// let`s fix that.
	if (isGnome3Session()) {
		QPalette p;
		p.setColor(QPalette::Window, QColor(Qt::white));
		ui.scrollArea->viewport()->setPalette(p);
		ui.scrollArea_2->viewport()->setPalette(p);
		ui.scrollArea_3->viewport()->setPalette(p);
		ui.scrollArea_4->viewport()->setPalette(p);

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
		Q_FOREACH(QGroupBox *box, findChildren<QGroupBox*>()) {
			box->setStyleSheet(gnomeCss);
		}

	}
	ui.cylinders->view()->horizontalHeader()->setContextMenuPolicy(Qt::ActionsContextMenu);

	QSettings s;
	s.beginGroup("cylinders_dialog");
	for(int i = 0; i < CylindersModel::COLUMNS; i++) {
		if ((i == CylindersModel::REMOVE) || (i == CylindersModel::TYPE))
			  continue;
		bool checked = s.value(QString("column%1_hidden").arg(i)).toBool();
		QAction *action = new QAction(cylindersModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString(), ui.cylinders->view());
		action->setCheckable(true);
		action->setData(i);
		action->setChecked(!checked);
		connect(action, SIGNAL(triggered(bool)), this, SLOT(toggleTriggeredColumn()));
		ui.cylinders->view()->setColumnHidden(i, checked);
		ui.cylinders->view()->horizontalHeader()->addAction(action);
	}
}

MainTab::~MainTab()
{
	QSettings s;
	s.beginGroup("cylinders_dialog");
	for(int i = 0; i < CylindersModel::COLUMNS; i++) {
		if ((i == CylindersModel::REMOVE) || (i == CylindersModel::TYPE))
			  continue;
		s.setValue(QString("column%1_hidden").arg(i), ui.cylinders->view()->isColumnHidden(i));
	}
}

void MainTab::toggleTriggeredColumn()
{
	QAction *action = qobject_cast<QAction*>(sender());
	int col = action->data().toInt();
	QTableView *view = ui.cylinders->view();

	if (action->isChecked()) {
		view->showColumn(col);
		if (view->columnWidth(col) <= 15)
			view->setColumnWidth(col, 80);
	}
	else
		view->hideColumn(col);
}

void MainTab::addDiveStarted()
{
	enableEdition(ADD);
}

void MainTab::addMessageAction(QAction* action)
{
	ui.diveEquipmentMessage->addAction(action);
	ui.diveNotesMessage->addAction(action);
	ui.diveInfoMessage->addAction(action);
	ui.diveStatisticsMessage->addAction(action);
}

void MainTab::hideMessage()
{
	ui.diveNotesMessage->animatedHide();
	ui.diveEquipmentMessage->animatedHide();
	ui.diveInfoMessage->animatedHide();
	ui.diveStatisticsMessage->animatedHide();
}

void MainTab::closeMessage()
{
	hideMessage();
	ui.diveNotesMessage->setCloseButtonVisible(false);
	ui.diveEquipmentMessage->setCloseButtonVisible(false);
	ui.diveInfoMessage->setCloseButtonVisible(false);
	ui.diveStatisticsMessage->setCloseButtonVisible(false);
}

void MainTab::displayMessage(QString str)
{
	ui.diveNotesMessage->setText(str);
	ui.diveNotesMessage->animatedShow();
	ui.diveEquipmentMessage->setText(str);
	ui.diveEquipmentMessage->animatedShow();
	ui.diveInfoMessage->setText(str);
	ui.diveInfoMessage->animatedShow();
	ui.diveStatisticsMessage->setText(str);
	ui.diveStatisticsMessage->animatedShow();
}

void MainTab::enableEdition(EditMode newEditMode)
{
	if (current_dive == NULL || editMode != NONE)
		return;
	if ((newEditMode == DIVE || newEditMode == NONE) &&
	    current_dive->dc.model &&
	    strcmp(current_dive->dc.model, "manually added dive") == 0) {
		// editCurrentDive will call enableEdition with newEditMode == MANUALLY_ADDED_DIVE
		// so exit this function here after editCurrentDive() returns
		mainWindow()->editCurrentDive();
		return;
	}
	mainWindow()->dive_list()->setEnabled(false);
	mainWindow()->globe()->prepareForGetDiveCoordinates();
	// We may be editing one or more dives here. backup everything.
	notesBackup.clear();
	if (mainWindow() && mainWindow()->dive_list()->selectedTrips().count() == 1) {
		// we are editing trip location and notes
		displayMessage(tr("This trip is being edited."));
		notesBackup[NULL].notes = ui.notes->toPlainText();
		notesBackup[NULL].location = ui.location->text();
		editMode = TRIP;
	} else {
		if (amount_selected > 1) {
			displayMessage(tr("Multiple dives are being edited."));
		} else {
			displayMessage(tr("This dive is being edited."));
		}

		// We may be editing one or more dives here. backup everything.
		struct dive *mydive;
		for (int i = 0; i < dive_table.nr; i++) {
			mydive = get_dive(i);
			if (!mydive)
				continue;
			if (!mydive->selected)
				continue;

			notesBackup[mydive].buddy = QString(mydive->buddy);
			notesBackup[mydive].suit = QString(mydive->suit);
			notesBackup[mydive].notes = QString(mydive->notes);
			notesBackup[mydive].divemaster = QString(mydive->divemaster);
			notesBackup[mydive].location = QString(mydive->location);
			notesBackup[mydive].rating = mydive->rating;
			notesBackup[mydive].visibility = mydive->visibility;
			notesBackup[mydive].latitude = mydive->latitude;
			notesBackup[mydive].longitude = mydive->longitude;
			notesBackup[mydive].coordinates  = ui.coordinates->text();
			notesBackup[mydive].airtemp = get_temperature_string(mydive->airtemp, true);
			notesBackup[mydive].watertemp = get_temperature_string(mydive->watertemp, true);
			notesBackup[mydive].datetime = QDateTime::fromTime_t(mydive->when).toUTC().toString();
			char buf[1024];
			taglist_get_tagstring(mydive->tag_list, buf, 1024);
			notesBackup[mydive].tags = QString(buf);

			// maybe this is a place for memset?
			for (int i = 0; i < MAX_CYLINDERS; i++) {
				notesBackup[mydive].cylinders[i] = mydive->cylinder[i];
			}
			for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++) {
				notesBackup[mydive].weightsystem[i] = mydive->weightsystem[i];
			}
		}

		editMode = newEditMode != NONE ? newEditMode : DIVE;
	}
}

bool MainTab::eventFilter(QObject* object, QEvent* event)
{
	if (!isEnabled())
		return false;

	if (editMode != NONE)
		return false;
	// for the dateTimeEdit widget we need to ignore Wheel events as well (as long as we aren't editing)
	if (object->objectName() == "dateTimeEdit" &&
	    (event->type() == QEvent::FocusIn || event->type() == QEvent::Wheel))
		return true;
	// MouseButtonPress in any widget (not all will ever get this), KeyPress in the dateTimeEdit,
	// FocusIn for the starWidgets or RequestSoftwareInputPanel for tagWidget start the editing
	if ((event->type() == QEvent::MouseButtonPress) ||
	    (event->type() == QEvent::KeyPress && object == ui.dateTimeEdit) ||
	    (event->type() == QEvent::FocusIn && (object == ui.rating || object == ui.visibility || object == ui.buddy)) ||
	    (event->type() == QEvent::RequestSoftwareInputPanel && object == ui.tagWidget)) {
		tabBar()->setTabIcon(currentIndex(), QIcon(":warning"));
		enableEdition();
	}
	return false; // don't "eat" the event.
}

void MainTab::clearEquipment()
{
	cylindersModel->clear();
	weightModel->clear();
}

void MainTab::clearInfo()
{
	ui.sacText->clear();
	ui.otuText->clear();
	ui.oxygenHeliumText->clear();
	ui.gasUsedText->clear();
	ui.dateText->clear();
	ui.diveTimeText->clear();
	ui.surfaceIntervalText->clear();
	ui.maximumDepthText->clear();
	ui.averageDepthText->clear();
	ui.waterTemperatureText->clear();
	ui.airTemperatureText->clear();
	ui.airPressureText->clear();
	ui.salinityText->clear();
	ui.tagWidget->clear();
}

void MainTab::clearStats()
{
	ui.depthLimits->clear();
	ui.sacLimits->clear();
	ui.divesAllText->clear();
	ui.tempLimits->clear();
	ui.totalTimeAllText->clear();
	ui.timeLimits->clear();
}

#define UPDATE_TEXT(d, field)				\
	if (!d || !d->field)				\
		ui.field->setText("");			\
	else						\
		ui.field->setText(d->field)

#define UPDATE_TEMP(d, field)				\
	if (!d || d->field.mkelvin == 0)		\
		ui.field->setText("");			\
	else						\
		ui.field->setText(get_temperature_string(d->field, true))

bool MainTab::isEditing()
{
	return editMode != NONE;
}

void MainTab::updateDiveInfo(int dive)
{
	// don't execute this while adding a dive
	if (editMode == ADD || editMode == MANUALLY_ADDED_DIVE)
		return;
	if (!isEnabled() && dive != -1)
		setEnabled(true);
	if (isEnabled() && dive == -1)
		setEnabled(false);
	editMode = NONE;
	// This method updates ALL tabs whenever a new dive or trip is
	// selected.
	// If exactly one trip has been selected, we show the location / notes
	// for the trip in the Info tab, otherwise we show the info of the
	// selected_dive
	temperature_t temp;
	struct dive *prevd;
	struct dive *d = get_dive(dive);
	char buf[1024];

	process_selected_dives();
	process_all_dives(d, &prevd);

	UPDATE_TEXT(d, notes);
	UPDATE_TEXT(d, location);
	UPDATE_TEXT(d, suit);
	UPDATE_TEXT(d, divemaster);
	UPDATE_TEXT(d, buddy);
	UPDATE_TEMP(d, airtemp);
	UPDATE_TEMP(d, watertemp);
	if (d) {
		updateGpsCoordinates(d);
		ui.dateTimeEdit->setDateTime(QDateTime::fromTime_t(d->when).toUTC());
		if (mainWindow() && mainWindow()->dive_list()->selectedTrips().count() == 1) {
			setTabText(0, tr("Trip Notes"));
			// only use trip relevant fields
			ui.coordinates->setVisible(false);
			ui.CoordinatedLabel->setVisible(false);
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
			ui.waterTempLabel->setVisible(false);
			ui.watertemp->setVisible(false);
			// rename the remaining fields and fill data from selected trip
			dive_trip_t *currentTrip = *mainWindow()->dive_list()->selectedTrips().begin();
			ui.LocationLabel->setText(tr("Trip Location"));
			ui.location->setText(currentTrip->location);
			ui.NotesLabel->setText(tr("Trip Notes"));
			ui.notes->setText(currentTrip->notes);
			clearEquipment();
			ui.equipmentTab->setEnabled(false);
		} else {
			setTabText(0, tr("Dive Notes"));
			// make all the fields visible writeable
			ui.coordinates->setVisible(true);
			ui.CoordinatedLabel->setVisible(true);
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
			ui.waterTempLabel->setVisible(true);
			ui.watertemp->setVisible(true);
			/* and fill them from the dive */
			ui.rating->setCurrentStars(d->rating);
			ui.visibility->setCurrentStars(d->visibility);
			// reset labels in case we last displayed trip notes
			ui.LocationLabel->setText(tr("Location"));
			ui.NotesLabel->setText(tr("Notes"));
			ui.equipmentTab->setEnabled(true);
			multiEditEquipmentPlaceholder = *d;
			cylindersModel->setDive(&multiEditEquipmentPlaceholder);
			weightModel->setDive(&multiEditEquipmentPlaceholder);
			taglist_get_tagstring(d->tag_list, buf, 1024);
			ui.tagWidget->setText(QString(buf));
		}
		ui.maximumDepthText->setText(get_depth_string(d->maxdepth, true));
		ui.averageDepthText->setText(get_depth_string(d->meandepth, true));
		ui.otuText->setText(QString("%1").arg(d->otu));
		ui.waterTemperatureText->setText(get_temperature_string(d->watertemp, true));
		ui.airTemperatureText->setText(get_temperature_string(d->airtemp, true));
		volume_t gases[MAX_CYLINDERS] = {};
		get_gas_used(d, gases);
		QString volumes = get_volume_string(gases[0], true);
		int mean[MAX_CYLINDERS], duration[MAX_CYLINDERS];
		per_cylinder_mean_depth(d, select_dc(&d->dc), mean, duration);
		volume_t sac;
		QString SACs;
		if (mean[0] && duration[0]) {
			sac.mliter = gases[0].mliter * 1000.0 / (depth_to_mbar(mean[0], d) * duration[0] / 60.0);
			SACs = get_volume_string(sac, true).append(tr("/min"));
		} else {
			SACs = QString(tr("unknown"));
		}
		for(int i=1; i < MAX_CYLINDERS && gases[i].mliter != 0; i++) {
			volumes.append("\n" + get_volume_string(gases[i], true));
			if (duration[i]) {
				sac.mliter = gases[i].mliter * 1000.0 / (depth_to_mbar(mean[i], d) * duration[i] / 60);
				SACs.append("\n" + get_volume_string(sac, true).append(tr("/min")));
			} else {
				SACs.append("\n");
			}
		}
		ui.gasUsedText->setText(volumes);
		ui.oxygenHeliumText->setText(get_gaslist(d));
		ui.dateText->setText(get_short_dive_date_string(d->when));
		ui.diveTimeText->setText(QString::number((int)((d->duration.seconds + 30) / 60)));
		if (prevd)
			ui.surfaceIntervalText->setText(get_time_string(d->when - (prevd->when + prevd->duration.seconds), 4));
		else
			ui.surfaceIntervalText->clear();
		if (mean[0])
			ui.sacText->setText(SACs);
		else
			ui.sacText->clear();
		if (d->surface_pressure.mbar)
			/* this is ALWAYS displayed in mbar */
			ui.airPressureText->setText(QString("%1mbar").arg(d->surface_pressure.mbar));
		else
			ui.airPressureText->clear();
		if (d->salinity)
			ui.salinityText->setText(QString("%1g/l").arg(d->salinity/10.0));
		else
			ui.salinityText->clear();
		ui.depthLimits->setMaximum(get_depth_string(stats_selection.max_depth, true));
		ui.depthLimits->setMinimum(get_depth_string(stats_selection.min_depth, true));
		ui.depthLimits->setAverage(get_depth_string(stats_selection.avg_depth, true));
		ui.sacLimits->setMaximum(get_volume_string(stats_selection.max_sac, true).append(tr("/min")));
		ui.sacLimits->setMinimum(get_volume_string(stats_selection.min_sac, true).append(tr("/min")));
		ui.sacLimits->setAverage(get_volume_string(stats_selection.avg_sac, true).append(tr("/min")));
		ui.divesAllText->setText(QString::number(stats_selection.selection_size));
		temp.mkelvin = stats_selection.max_temp;
		ui.tempLimits->setMaximum(get_temperature_string(temp, true));
		temp.mkelvin = stats_selection.min_temp;
		ui.tempLimits->setMinimum(get_temperature_string(temp, true));
		if (stats_selection.combined_temp && stats_selection.combined_count) {
			const char *unit;
			get_temp_units(0, &unit);
			ui.tempLimits->setAverage(QString("%1%2").arg(stats_selection.combined_temp / stats_selection.combined_count, 0, 'f', 1).arg(unit));
		}
		ui.totalTimeAllText->setText(get_time_string(stats_selection.total_time.seconds, 0));
		int seconds = stats_selection.total_time.seconds;
		if (stats_selection.selection_size)
			seconds /= stats_selection.selection_size;
		ui.timeLimits->setAverage(get_time_string(seconds, 0));
		ui.timeLimits->setMaximum(get_time_string(stats_selection.longest_time.seconds, 0));
		ui.timeLimits->setMinimum(get_time_string(stats_selection.shortest_time.seconds, 0));
	} else {
		/* clear the fields */
		clearInfo();
		clearStats();
		clearEquipment();
		ui.rating->setCurrentStars(0);
		ui.coordinates->clear();
		ui.visibility->setCurrentStars(0);
		/* turns out this is non-trivial for a dateTimeEdit... this is a partial hack */
		QLineEdit *le = ui.dateTimeEdit->findChild<QLineEdit*>();
		le->setText("");
	}
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
	SuitCompletionModel::instance()->updateModel();
	BuddyCompletionModel::instance()->updateModel();
	LocationCompletionModel::instance()->updateModel();
	DiveMasterCompletionModel::instance()->updateModel();
	TagCompletionModel::instance()->updateModel();
}

void MainTab::acceptChanges()
{
	mainWindow()->dive_list()->setEnabled(true);
	tabBar()->setTabIcon(0, QIcon()); // Notes
	tabBar()->setTabIcon(1, QIcon()); // Equipment
	hideMessage();
	ui.equipmentTab->setEnabled(true);
	/* now figure out if things have changed */
	if (mainWindow() && mainWindow()->dive_list()->selectedTrips().count() == 1) {
		if (notesBackup[NULL].notes != ui.notes->toPlainText() ||
			notesBackup[NULL].location != ui.location->text())
			mark_divelist_changed(true);
	} else {
		struct dive *curr = current_dive;
		//Reset coordinates field, in case it contains garbage.
		updateGpsCoordinates(curr);
		if (notesBackup[curr].buddy != ui.buddy->text() ||
			notesBackup[curr].suit != ui.suit->text() ||
			notesBackup[curr].notes != ui.notes->toPlainText() ||
			notesBackup[curr].divemaster != ui.divemaster->text() ||
			notesBackup[curr].location  != ui.location->text() ||
			notesBackup[curr].coordinates != ui.coordinates->text() ||
			notesBackup[curr].rating != ui.visibility->currentStars() ||
			notesBackup[curr].airtemp != ui.airtemp->text() ||
			notesBackup[curr].watertemp != ui.watertemp->text() ||
			notesBackup[curr].datetime != ui.dateTimeEdit->dateTime().toString() ||
			notesBackup[curr].visibility != ui.rating->currentStars() ||
			notesBackup[curr].tags != ui.tagWidget->text()) {
			mark_divelist_changed(true);
		}
		if (notesBackup[curr].location != ui.location->text() ||
			notesBackup[curr].coordinates != ui.coordinates->text()) {
			mainWindow()->globe()->reload();
		}

		if (notesBackup[curr].tags != ui.tagWidget->text())
			saveTags();
		if (editMode == MANUALLY_ADDED_DIVE) {
			DivePlannerPointsModel::instance()->copyCylinders(curr);
		} else if (editMode != ADD && cylindersModel->changed) {
			mark_divelist_changed(true);
			Q_FOREACH (dive *d, notesBackup.keys()) {
				for (int i = 0; i < MAX_CYLINDERS; i++) {
					if (notesBackup.keys().count() > 1)
						// only copy the cylinder type, none of the other values
						d->cylinder[i].type = multiEditEquipmentPlaceholder.cylinder[i].type;
					else
						d->cylinder[i] = multiEditEquipmentPlaceholder.cylinder[i];
				}
			}
		}

		if (weightModel->changed) {
			mark_divelist_changed(true);
			Q_FOREACH (dive *d, notesBackup.keys()) {
				for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++) {
					d->weightsystem[i] = multiEditEquipmentPlaceholder.weightsystem[i];
				}
			}
		}

	}
	if (current_dive->divetrip) {
		current_dive->divetrip->when = current_dive->when;
		find_new_trip_start_time(current_dive->divetrip);
	}
	if (editMode == ADD || editMode == MANUALLY_ADDED_DIVE) {
		// clean up the dive data (get duration, depth information from samples)
		fixup_dive(current_dive);
		if (dive_table.nr == 1)
			current_dive->number = 1;
		else if (selected_dive == dive_table.nr - 1 && get_dive(dive_table.nr - 2)->number)
			current_dive->number = get_dive(dive_table.nr - 2)->number + 1;
		DivePlannerPointsModel::instance()->cancelPlan();
		mainWindow()->showProfile();
		mark_divelist_changed(true);
		DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::NOTHING);
	}
	// each dive that was selected might have had the temperatures in its active divecomputer changed
	// so re-populate the temperatures - easiest way to do this is by calling fixup_dive
	Q_FOREACH(dive *d, notesBackup.keys()) {
		if (d)
			fixup_dive(d);
	}
	int scrolledBy = mainWindow()->dive_list()->verticalScrollBar()->sliderPosition();
	resetPallete();
	if (editMode == ADD || editMode == MANUALLY_ADDED_DIVE) {
		mainWindow()->dive_list()->unselectDives();
		struct dive *d = get_dive(dive_table.nr -1 );
		// mark the dive as remembered (abusing the selected flag)
		// and then clear that flag out on the other side of the sort_table()
		d->selected = true;
		sort_table(&dive_table);
		int i = 0;
		for_each_dive(i,d) {
			if (d->selected) {
				d->selected = false;
				break;
			}
		}
		editMode = NONE;
		mainWindow()->refreshDisplay();
		mainWindow()->dive_list()->selectDive( i, true );
	} else {
		editMode = NONE;
		mainWindow()->dive_list()->rememberSelection();
		sort_table(&dive_table);
		mainWindow()->refreshDisplay();
		mainWindow()->dive_list()->restoreSelection();
	}
	mainWindow()->dive_list()->verticalScrollBar()->setSliderPosition(scrolledBy);
	mainWindow()->dive_list()->setFocus();
}

void MainTab::resetPallete()
{
	QPalette p;
	ui.buddy->setPalette(p);
	ui.notes->setPalette(p);
	ui.location->setPalette(p);
	ui.coordinates->setPalette(p);
	ui.divemaster->setPalette(p);
	ui.suit->setPalette(p);
	ui.airtemp->setPalette(p);
	ui.watertemp->setPalette(p);
	ui.dateTimeEdit->setPalette(p);
	ui.tagWidget->setPalette(p);
}

#define EDIT_TEXT2(what, text) \
	textByteArray = text.toUtf8(); \
	free(what);\
	what = strdup(textByteArray.data());

#define EDIT_TEXT(what, text) \
	QByteArray textByteArray = text.toUtf8(); \
	free(what);\
	what = strdup(textByteArray.data());

void MainTab::rejectChanges()
{
	EditMode lastMode = editMode;
	tabBar()->setTabIcon(0, QIcon()); // Notes
	tabBar()->setTabIcon(1, QIcon()); // Equipment

	mainWindow()->dive_list()->setEnabled(true);
	if (mainWindow() && mainWindow()->dive_list()->selectedTrips().count() == 1) {
		ui.notes->setText(notesBackup[NULL].notes );
		ui.location->setText(notesBackup[NULL].location);
	} else {
		if (lastMode == ADD) {
			// clean up
			DivePlannerPointsModel::instance()->cancelPlan();
		} else if (lastMode == MANUALLY_ADDED_DIVE ) {
			// when we tried to edit a manually added dive, we destroyed
			// the dive we edited, so let's just restore it from backup
			DivePlannerPointsModel::instance()->restoreBackupDive();
		}
		struct dive *curr = current_dive;
		ui.notes->setText(notesBackup[curr].notes );
		ui.location->setText(notesBackup[curr].location);
		ui.buddy->setText(notesBackup[curr].buddy);
		ui.suit->setText(notesBackup[curr].suit);
		ui.divemaster->setText(notesBackup[curr].divemaster);
		ui.rating->setCurrentStars(notesBackup[curr].rating);
		ui.visibility->setCurrentStars(notesBackup[curr].visibility);
		ui.airtemp->setText(notesBackup[curr].airtemp);
		ui.watertemp->setText(notesBackup[curr].watertemp);
		ui.tagWidget->setText(notesBackup[curr].tags);
		// it's a little harder to do the right thing for the date time widget
		if (curr) {
			ui.dateTimeEdit->setDateTime(QDateTime::fromString(notesBackup[curr].datetime));
		} else {
			QLineEdit *le = ui.dateTimeEdit->findChild<QLineEdit*>();
			le->setText("");
		}

		struct dive *mydive;
		for (int i = 0; i < dive_table.nr; i++) {
			mydive = get_dive(i);
			if (!mydive)
				continue;
			if (!mydive->selected)
				continue;

			QByteArray textByteArray;
			EDIT_TEXT2(mydive->buddy, notesBackup[mydive].buddy);
			EDIT_TEXT2(mydive->suit, notesBackup[mydive].suit);
			EDIT_TEXT2(mydive->notes, notesBackup[mydive].notes);
			EDIT_TEXT2(mydive->divemaster, notesBackup[mydive].divemaster);
			EDIT_TEXT2(mydive->location, notesBackup[mydive].location);
			mydive->latitude = notesBackup[mydive].latitude;
			mydive->longitude = notesBackup[mydive].longitude;
			mydive->rating = notesBackup[mydive].rating;
			mydive->visibility = notesBackup[mydive].visibility;

			// maybe this is a place for memset?
			for (int i = 0; i < MAX_CYLINDERS; i++) {
				mydive->cylinder[i] = notesBackup[mydive].cylinders[i];
			}
			for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++) {
				mydive->weightsystem[i] = notesBackup[mydive].weightsystem[i];
			}
		}
		updateGpsCoordinates(curr);
		if (lastMode == ADD) {
			delete_single_dive(selected_dive);
			mainWindow()->dive_list()->reload(DiveTripModel::CURRENT);
			mainWindow()->dive_list()->restoreSelection();
		}
		if (selected_dive >= 0) {
			multiEditEquipmentPlaceholder = *get_dive(selected_dive);
			cylindersModel->setDive(&multiEditEquipmentPlaceholder);
			weightModel->setDive(&multiEditEquipmentPlaceholder);
		} else {
			cylindersModel->clear();
			weightModel->clear();
			setEnabled(false);
		}
	}

	hideMessage();
	mainWindow()->dive_list()->setEnabled(true);
	notesBackup.clear();
	resetPallete();
	editMode = NONE;
	mainWindow()->globe()->reload();
	if (lastMode == ADD || lastMode == MANUALLY_ADDED_DIVE) {
		// more clean up
		updateDiveInfo(selected_dive);
		mainWindow()->showProfile();
		// we already reloaded the divelist above, so don't recreate it or we'll lose the selection
		mainWindow()->refreshDisplay(false);
		DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::NOTHING);
	}
	mainWindow()->dive_list()->setFocus();
}
#undef EDIT_TEXT2

#define EDIT_SELECTED_DIVES( WHAT ) do { \
	if (editMode == NONE) \
		return; \
\
	for (int i = 0; i < dive_table.nr; i++) { \
		struct dive *mydive = get_dive(i); \
		if (!mydive) \
			continue; \
		if (!mydive->selected) \
			continue; \
\
		WHAT; \
	} \
} while(0)

void markChangedWidget(QWidget *w) {
	QPalette p;
	qreal h, s, l, a;
	qApp->palette().color(QPalette::Text).getHslF(&h, &s, &l, &a);
	p.setBrush(QPalette::Base, ( l <= 0.3 ) ? QColor(Qt::yellow).lighter()
				  :( l <= 0.6 ) ? QColor(Qt::yellow).light()
				  :/* else */     QColor(Qt::yellow).darker(300)
				  );
	w->setPalette(p);
}

void MainTab::on_buddy_textChanged()
{
	QString text = ui.buddy->toPlainText().split(",", QString::SkipEmptyParts).join(", ");
	EDIT_SELECTED_DIVES( EDIT_TEXT(mydive->buddy, text) );
	markChangedWidget(ui.buddy);
}

void MainTab::on_divemaster_textChanged(const QString& text)
{
	EDIT_SELECTED_DIVES( EDIT_TEXT(mydive->divemaster, text) );
	markChangedWidget(ui.divemaster);
}

void MainTab::on_airtemp_textChanged(const QString& text)
{
	EDIT_SELECTED_DIVES( select_dc(&mydive->dc)->airtemp.mkelvin = parseTemperatureToMkelvin(text) );
	markChangedWidget(ui.airtemp);
}

void MainTab::on_watertemp_textChanged(const QString& text)
{
	EDIT_SELECTED_DIVES( select_dc(&mydive->dc)->watertemp.mkelvin = parseTemperatureToMkelvin(text) );
	markChangedWidget(ui.watertemp);
}

void MainTab::on_dateTimeEdit_dateTimeChanged(const QDateTime& datetime)
{
	QDateTime dateTimeUtc(datetime);
	dateTimeUtc.setTimeSpec(Qt::UTC);
	EDIT_SELECTED_DIVES( mydive->when = dateTimeUtc.toTime_t() );
	markChangedWidget(ui.dateTimeEdit);
}

void MainTab::saveTags()
{
	EDIT_SELECTED_DIVES(
		QString tag;
		taglist_clear(mydive->tag_list);
		foreach (tag, ui.tagWidget->getBlockStringList())
			taglist_add_tag(mydive->tag_list, tag.toUtf8().data());
	);
}

void MainTab::on_tagWidget_textChanged()
{
	markChangedWidget(ui.tagWidget);
}

void MainTab::on_location_textChanged(const QString& text)
{
	if (editMode == NONE)
		return;
	if (editMode == TRIP && mainWindow() && mainWindow()->dive_list()->selectedTrips().count() == 1) {
		// we are editing a trip
		dive_trip_t *currentTrip = *mainWindow()->dive_list()->selectedTrips().begin();
		EDIT_TEXT(currentTrip->location, text);
	} else if (editMode == DIVE || editMode == ADD) {
		if (!ui.coordinates->isModified() ||
		    ui.coordinates->text().trimmed().isEmpty()) {
			struct dive* dive;
			int i = 0;
			for_each_dive(i, dive) {
				QString location(dive->location);
				if (location == text &&
				    (dive->latitude.udeg || dive->longitude.udeg)) {
					EDIT_SELECTED_DIVES( mydive->latitude = dive->latitude );
					EDIT_SELECTED_DIVES( mydive->longitude = dive->longitude );
					//Don't use updateGpsCoordinates() since we don't want to set modified state yet
					ui.coordinates->setText(printGPSCoords(dive->latitude.udeg, dive->longitude.udeg));
					markChangedWidget(ui.coordinates);
					break;
				}
			}
		}
		EDIT_SELECTED_DIVES( EDIT_TEXT(mydive->location, text) );
		mainWindow()->globe()->repopulateLabels();
	}
	markChangedWidget(ui.location);
}

void MainTab::on_suit_textChanged(const QString& text)
{
	EDIT_SELECTED_DIVES( EDIT_TEXT(mydive->suit, text) );
	markChangedWidget(ui.suit);
}

void MainTab::on_notes_textChanged()
{
	if (editMode == NONE)
		return;
	if (editMode == TRIP && mainWindow() && mainWindow()->dive_list()->selectedTrips().count() == 1) {
		// we are editing a trip
		dive_trip_t *currentTrip = *mainWindow()->dive_list()->selectedTrips().begin();
		EDIT_TEXT(currentTrip->notes, ui.notes->toPlainText());
	} else if (editMode == DIVE || editMode == ADD || editMode == MANUALLY_ADDED_DIVE) {
		EDIT_SELECTED_DIVES( EDIT_TEXT(mydive->notes, ui.notes->toPlainText()) );
	}
	markChangedWidget(ui.notes);
}

#undef EDIT_TEXT

void MainTab::on_coordinates_textChanged(const QString& text)
{
	bool gpsChanged = false;
	bool parsed = false;
	EDIT_SELECTED_DIVES(gpsChanged |= gpsHasChanged(mydive, current_dive, text, &parsed));
	if (gpsChanged) {
		markChangedWidget(ui.coordinates);
	} else if (!parsed) {
		QPalette p;
		p.setBrush(QPalette::Base, QColor(Qt::red).lighter());
		ui.coordinates->setPalette(p);
	}
}

void MainTab::on_rating_valueChanged(int value)
{
	EDIT_SELECTED_DIVES(mydive->rating  = value );
}

void MainTab::on_visibility_valueChanged(int value)
{
	EDIT_SELECTED_DIVES( mydive->visibility = value );
}

void MainTab::editCylinderWidget(const QModelIndex& index)
{
	if (editMode == NONE)
		enableEdition();

	if (index.isValid() && index.column() != CylindersModel::REMOVE)
		ui.cylinders->edit(index);
}

void MainTab::editWeightWidget(const QModelIndex& index)
{
	if (editMode == NONE)
		enableEdition();

	if (index.isValid() && index.column() != WeightModel::REMOVE)
		ui.weights->edit(index);
}

QString MainTab::printGPSCoords(int lat, int lon)
{
	unsigned int latdeg, londeg;
	unsigned int latmin, lonmin;
	double  latsec, lonsec;
	QString lath, lonh, result;

	if (!lat && !lon)
		return QString("");

	lath = lat >= 0 ? tr("N") : tr("S");
	lonh = lon >= 0 ? tr("E") : tr("W");
	lat = abs(lat);
	lon = abs(lon);
	latdeg = lat / 1000000;
	londeg = lon / 1000000;
	latmin = (lat % 1000000) * 60;
	lonmin = (lon % 1000000) * 60;
	latsec = (latmin % 1000000) * 60;
	lonsec = (lonmin % 1000000) * 60;
	result.sprintf("%u%s%02d\'%06.3f\"%s %u%s%02d\'%06.3f\"%s",
		       latdeg, UTF8_DEGREE, latmin / 1000000, latsec / 1000000, lath.toUtf8().data(),
		       londeg, UTF8_DEGREE, lonmin / 1000000, lonsec / 1000000, lonh.toUtf8().data());
	return result;
}

void MainTab::updateCoordinatesText(qreal lat, qreal lon)
{
	int ulat = rint(lat * 1000000);
	int ulon = rint(lon * 1000000);
	ui.coordinates->setText(printGPSCoords(ulat, ulon));
}

void MainTab::updateGpsCoordinates(const struct dive *dive)
{
	if (dive) {
		ui.coordinates->setText(printGPSCoords(dive->latitude.udeg, dive->longitude.udeg));
		ui.coordinates->setModified(dive->latitude.udeg || dive->longitude.udeg);
	} else {
		ui.coordinates->clear();
	}
}
