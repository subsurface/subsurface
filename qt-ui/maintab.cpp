/*
 * maintab.cpp
 *
 * classes for the "notebook" area of the main window of Subsurface
 *
 */
#include "maintab.h"
#include "ui_maintab.h"
#include "mainwindow.h"
#include "../helpers.h"
#include "../statistics.h"
#include "divelistview.h"
#include "modeldelegates.h"
#include "globe.h"

#include <QLabel>
#include <QDebug>
#include <QSet>

MainTab::MainTab(QWidget *parent) : QTabWidget(parent),
				    ui(new Ui::MainTab()),
				    weightModel(new WeightModel()),
				    cylindersModel(new CylindersModel()),
				    currentDive(0),
				    editMode(NONE)
{
	ui->setupUi(this);
	ui->cylinders->setModel(cylindersModel);
	ui->weights->setModel(weightModel);
	ui->diveNotesMessage->hide();
	ui->diveNotesMessage->setCloseButtonVisible(false);
#ifdef __APPLE__
	setDocumentMode(false);
#else
	setDocumentMode(true);
#endif
	// we start out with the fields read-only; once things are
	// filled from a dive, they are made writeable
	ui->location->setReadOnly(true);
	ui->divemaster->setReadOnly(true);
	ui->buddy->setReadOnly(true);
	ui->suit->setReadOnly(true);
	ui->notes->setReadOnly(true);
	ui->rating->setReadOnly(true);
	ui->visibility->setReadOnly(true);

	ui->editAccept->hide();
	ui->editReset->hide();

	ui->location->installEventFilter(this);
	ui->divemaster->installEventFilter(this);
	ui->buddy->installEventFilter(this);
	ui->suit->installEventFilter(this);
	ui->notes->installEventFilter(this);
	ui->rating->installEventFilter(this);
	ui->visibility->installEventFilter(this);

	QList<QObject *> statisticsTabWidgets = ui->statisticsTab->children();
	Q_FOREACH(QObject* obj, statisticsTabWidgets) {
		QLabel* label = qobject_cast<QLabel *>(obj);
		if (label)
			label->setAlignment(Qt::AlignHCenter);
	}

	/*Thid couldn't be done on the ui file because element
	is floating, instead of being fixed on the layout. */
	QIcon plusIcon(":plus");
	addCylinder = new QPushButton(plusIcon, QString(), ui->cylindersGroup);
	addCylinder->setFlat(true);
	addCylinder->setToolTip(tr("Add Cylinder"));
	connect(addCylinder, SIGNAL(clicked(bool)), this, SLOT(addCylinder_clicked()));
	addCylinder->setEnabled(false);
	addWeight = new QPushButton(plusIcon, QString(), ui->weightGroup);
	addWeight->setFlat(true);
	addWeight->setToolTip(tr("Add Weight System"));
	connect(addWeight, SIGNAL(clicked(bool)), this, SLOT(addWeight_clicked()));
	addWeight->setEnabled(false);

	connect(ui->cylinders, SIGNAL(clicked(QModelIndex)), ui->cylinders->model(), SLOT(remove(QModelIndex)));
	connect(ui->weights, SIGNAL(clicked(QModelIndex)), ui->weights->model(), SLOT(remove(QModelIndex)));

	QFontMetrics metrics(defaultModelFont());
	QFontMetrics metrics2(font());

	ui->cylinders->setColumnWidth(CylindersModel::REMOVE, 24);
	ui->cylinders->setColumnWidth(CylindersModel::TYPE, metrics2.width(TankInfoModel::instance()->biggerString()) + 20);
	ui->cylinders->horizontalHeader()->setResizeMode(CylindersModel::REMOVE, QHeaderView::Fixed);
	ui->cylinders->verticalHeader()->setDefaultSectionSize( metrics.height() +8 );
	ui->cylinders->setItemDelegateForColumn(CylindersModel::TYPE, new TankInfoDelegate());

	ui->weights->setColumnWidth(WeightModel::REMOVE, 24);
	ui->weights->setColumnWidth(WeightModel::TYPE, metrics2.width(WSInfoModel::instance()->biggerString()) + 20);
	ui->weights->horizontalHeader()->setResizeMode (WeightModel::REMOVE , QHeaderView::Fixed);
	ui->weights->verticalHeader()->setDefaultSectionSize( metrics.height() +8 );
	ui->weights->setItemDelegateForColumn(WeightModel::TYPE, new WSInfoDelegate());
}

// We need to manually position the 'plus' on cylinder and weight.
void MainTab::resizeEvent(QResizeEvent* event)
{
	if (ui->cylindersGroup->isVisible())
		addCylinder->setGeometry(ui->cylindersGroup->contentsRect().width() - 30, 2, 24,24);

	if (ui->weightGroup->isVisible())
		addWeight->setGeometry(ui->weightGroup->contentsRect().width() - 30, 2, 24,24);

	QTabWidget::resizeEvent(event);
}

void MainTab::showEvent(QShowEvent* event)
{
	QTabWidget::showEvent(event);
	addCylinder->setGeometry(ui->cylindersGroup->contentsRect().width() - 30, 2, 24,24);
	addWeight->setGeometry(ui->weightGroup->contentsRect().width() - 30, 2, 24,24);
}


bool MainTab::eventFilter(QObject* object, QEvent* event)
{
	if (event->type() == QEvent::FocusIn || event->type() == QEvent::MouseButtonPress) {
		if (ui->editAccept->isVisible() || !currentDive)
			return false;

		ui->editAccept->setChecked(true);
		ui->editAccept->show();
		ui->editReset->show();
		on_editAccept_clicked(true);
	}
	return false;
}

void MainTab::clearEquipment()
{
}

void MainTab::clearInfo()
{
	ui->sacText->clear();
	ui->otuText->clear();
	ui->oxygenHeliumText->clear();
	ui->gasUsedText->clear();
	ui->dateText->clear();
	ui->diveTimeText->clear();
	ui->surfaceIntervalText->clear();
	ui->maximumDepthText->clear();
	ui->averageDepthText->clear();
	ui->waterTemperatureText->clear();
	ui->airTemperatureText->clear();
	ui->airPressureText->clear();
}

void MainTab::clearStats()
{
	ui->depthLimits->clear();
	ui->sacLimits->clear();
	ui->divesAllText->clear();
	ui->tempLimits->clear();
	ui->totalTimeAllText->clear();
	ui->timeLimits->clear();
}

#define UPDATE_TEXT(d, field)				\
	if (!d || !d->field)				\
		ui->field->setText("");			\
	else						\
		ui->field->setText(d->field)


void MainTab::updateDiveInfo(int dive)
{
	// This method updates ALL tabs whenever a new dive or trip is
	// selected.
	// If exactly one trip has been selected, we show the location / notes
	// for the trip in the Info tab, otherwise we show the info of the
	// selected_dive
	volume_t sacVal;
	temperature_t temp;
	struct dive *prevd;
	struct dive *d = get_dive(dive);

	process_selected_dives();
	process_all_dives(d, &prevd);
	currentDive = d;
	UPDATE_TEXT(d, notes);
	UPDATE_TEXT(d, location);
	UPDATE_TEXT(d, suit);
	UPDATE_TEXT(d, divemaster);
	UPDATE_TEXT(d, buddy);
	if (d) {
		if (mainWindow() && mainWindow()->dive_list()->selectedTrips.count() == 1) {
			// only use trip relevant fields
			ui->divemaster->setVisible(false);
			ui->DivemasterLabel->setVisible(false);
			ui->buddy->setVisible(false);
			ui->BuddyLabel->setVisible(false);
			ui->suit->setVisible(false);
			ui->SuitLabel->setVisible(false);
			ui->rating->setVisible(false);
			ui->RatingLabel->setVisible(false);
			ui->visibility->setVisible(false);
			ui->visibilityLabel->setVisible(false);
			// rename the remaining fields and fill data from selected trip
			dive_trip_t *currentTrip = *mainWindow()->dive_list()->selectedTrips.begin();
			ui->location->setReadOnly(false);
			ui->LocationLabel->setText(tr("Trip Location"));
			ui->location->setText(currentTrip->location);
			ui->notes->setReadOnly(false);
			ui->NotesLabel->setText(tr("Trip Notes"));
			ui->notes->setText(currentTrip->notes);
		} else {
			// make all the fields visible writeable
			ui->divemaster->setVisible(true);
			ui->buddy->setVisible(true);
			ui->suit->setVisible(true);
			ui->rating->setVisible(true);
			ui->visibility->setVisible(true);
			ui->divemaster->setReadOnly(false);
			ui->buddy->setReadOnly(false);
			ui->suit->setReadOnly(false);
			ui->rating->setReadOnly(false);
			ui->visibility->setReadOnly(false);
			/* and fill them from the dive */
			ui->rating->setCurrentStars(d->rating);
			ui->visibility->setCurrentStars(d->visibility);
			// reset labels in case we last displayed trip notes
			ui->location->setReadOnly(false);
			ui->LocationLabel->setText(tr("Location"));
			ui->notes->setReadOnly(false);
			ui->NotesLabel->setText(tr("Notes"));
		}
		ui->maximumDepthText->setText(get_depth_string(d->maxdepth, TRUE));
		ui->averageDepthText->setText(get_depth_string(d->meandepth, TRUE));
		ui->otuText->setText(QString("%1").arg(d->otu));
		ui->waterTemperatureText->setText(get_temperature_string(d->watertemp, TRUE));
		ui->airTemperatureText->setText(get_temperature_string(d->airtemp, TRUE));
		ui->gasUsedText->setText(get_volume_string(get_gas_used(d), TRUE));
		ui->oxygenHeliumText->setText(get_gaslist(d));
		ui->dateText->setText(get_short_dive_date_string(d->when));
		ui->diveTimeText->setText(QString::number((int)((d->duration.seconds + 30) / 60)));
		if (prevd)
			ui->surfaceIntervalText->setText(get_time_string(d->when - (prevd->when + prevd->duration.seconds), 4));
		if ((sacVal.mliter = d->sac) > 0)
			ui->sacText->setText(get_volume_string(sacVal, TRUE).append(tr("/min")));
		else
			ui->sacText->clear();
		if (d->surface_pressure.mbar)
			/* this is ALWAYS displayed in mbar */
			ui->airPressureText->setText(QString("%1mbar").arg(d->surface_pressure.mbar));
		else
			ui->airPressureText->clear();
		ui->depthLimits->setMaximum(get_depth_string(stats_selection.max_depth, TRUE));
		ui->depthLimits->setMinimum(get_depth_string(stats_selection.min_depth, TRUE));
		ui->depthLimits->setAverage(get_depth_string(stats_selection.avg_depth, TRUE));
		ui->sacLimits->setMaximum(get_volume_string(stats_selection.max_sac, TRUE).append(tr("/min")));
		ui->sacLimits->setMinimum(get_volume_string(stats_selection.min_sac, TRUE).append(tr("/min")));
		ui->sacLimits->setAverage(get_volume_string(stats_selection.avg_sac, TRUE).append(tr("/min")));
		ui->divesAllText->setText(QString::number(stats_selection.selection_size));
		temp.mkelvin = stats_selection.max_temp;
		ui->tempLimits->setMaximum(get_temperature_string(temp, TRUE));
		temp.mkelvin = stats_selection.min_temp;
		ui->tempLimits->setMinimum(get_temperature_string(temp, TRUE));
		if (stats_selection.combined_temp && stats_selection.combined_count) {
			const char *unit;
			get_temp_units(0, &unit);
			ui->tempLimits->setAverage(QString("%1%2").arg(stats_selection.combined_temp / stats_selection.combined_count, 0, 'f', 1).arg(unit));
		}
		ui->totalTimeAllText->setText(get_time_string(stats_selection.total_time.seconds, 0));
		int seconds = stats_selection.total_time.seconds;
		if (stats_selection.selection_size)
			seconds /= stats_selection.selection_size;
		ui->timeLimits->setAverage(get_time_string(seconds, 0));
		ui->timeLimits->setMaximum(get_time_string(stats_selection.longest_time.seconds, 0));
		ui->timeLimits->setMinimum(get_time_string(stats_selection.shortest_time.seconds, 0));
		cylindersModel->setDive(d);
		weightModel->setDive(d);
		addCylinder->setEnabled(true);
		addWeight->setEnabled(true);
	} else {
		/* make the fields read-only */
		ui->location->setReadOnly(true);
		ui->divemaster->setReadOnly(true);
		ui->buddy->setReadOnly(true);
		ui->suit->setReadOnly(true);
		ui->notes->setReadOnly(true);
		ui->rating->setReadOnly(true);
		ui->visibility->setReadOnly(true);
		/* clear the fields */
		ui->rating->setCurrentStars(0);
		ui->sacText->clear();
		ui->otuText->clear();
		ui->oxygenHeliumText->clear();
		ui->dateText->clear();
		ui->diveTimeText->clear();
		ui->surfaceIntervalText->clear();
		ui->maximumDepthText->clear();
		ui->averageDepthText->clear();
		ui->visibility->setCurrentStars(0);
		ui->waterTemperatureText->clear();
		ui->airTemperatureText->clear();
		ui->gasUsedText->clear();
		ui->airPressureText->clear();
		cylindersModel->clear();
		weightModel->clear();
		addCylinder->setEnabled(false);
		addWeight->setEnabled(false);
		ui->depthLimits->clear();
		ui->sacLimits->clear();
		ui->divesAllText->clear();
		ui->tempLimits->clear();
		ui->totalTimeAllText->clear();
		ui->timeLimits->clear();
	}
}

void MainTab::addCylinder_clicked()
{
	cylindersModel->add();
}

void MainTab::addWeight_clicked()
{
	weightModel->add();
}

void MainTab::reload()
{
}

void MainTab::on_editAccept_clicked(bool edit)
{
	ui->location->setReadOnly(!edit);
	ui->divemaster->setReadOnly(!edit);
	ui->buddy->setReadOnly(!edit);
	ui->suit->setReadOnly(!edit);
	ui->notes->setReadOnly(!edit);
	ui->rating->setReadOnly(!edit);
	ui->visibility->setReadOnly(!edit);

	mainWindow()->dive_list()->setEnabled(!edit);

	if (edit) {
		if (mainWindow() && mainWindow()->dive_list()->selectedTrips.count() == 1) {
			// we are editing trip location and notes
			ui->diveNotesMessage->setText(tr("This trip is being edited. Select Save or Undo when ready."));
			ui->diveNotesMessage->animatedShow();
			notesBackup.notes = ui->notes->toPlainText();
			notesBackup.location = ui->location->text();
			editMode = TRIP;
		} else {
			ui->diveNotesMessage->setText(tr("This dive is being edited. Select Save or Undo when ready."));
			ui->diveNotesMessage->animatedShow();
			notesBackup.buddy = ui->buddy->text();
			notesBackup.suit = ui->suit->text();
			notesBackup.notes = ui->notes->toPlainText();
			notesBackup.divemaster = ui->divemaster->text();
			notesBackup.location = ui->location->text();
			notesBackup.rating = ui->rating->currentStars();
			notesBackup.visibility = ui->visibility->currentStars();
			editMode = DIVE;
		}
	} else {
		ui->diveNotesMessage->animatedHide();
		ui->editAccept->hide();
		ui->editReset->hide();
		/* now figure out if things have changed */
		if (mainWindow() && mainWindow()->dive_list()->selectedTrips.count() == 1) {
			if (notesBackup.notes != ui->notes->toPlainText() ||
			    notesBackup.location != ui->location->text())
				mark_divelist_changed(TRUE);
		} else {
			if (notesBackup.buddy != ui->buddy->text() ||
			    notesBackup.suit != ui->suit->text() ||
			    notesBackup.notes != ui->notes->toPlainText() ||
			    notesBackup.divemaster != ui->divemaster->text() ||
			    notesBackup.location != ui->location->text() ||
			    notesBackup.visibility != ui->visibility->currentStars() ||
			    notesBackup.rating != ui->rating->currentStars())
				mark_divelist_changed(TRUE);
			if (notesBackup.location != ui->location->text())
				mainWindow()->globe()->reload();
		}
		editMode = NONE;
	}
}

void MainTab::on_editReset_clicked()
{
	if (!ui->editAccept->isChecked())
		return;

	ui->notes->setText(notesBackup.notes);
	ui->location->setText(notesBackup.location);
	if (mainWindow() && mainWindow()->dive_list()->selectedTrips.count() != 1) {
		ui->buddy->setText(notesBackup.buddy);
		ui->suit->setText(notesBackup.suit);
		ui->divemaster->setText(notesBackup.divemaster);
		ui->rating->setCurrentStars(notesBackup.rating);
		ui->visibility->setCurrentStars(notesBackup.visibility);
	}
	ui->editAccept->setChecked(false);
	ui->diveNotesMessage->animatedHide();

	ui->location->setReadOnly(true);
	ui->divemaster->setReadOnly(true);
	ui->buddy->setReadOnly(true);
	ui->suit->setReadOnly(true);
	ui->notes->setReadOnly(true);
	ui->rating->setReadOnly(true);
	ui->visibility->setReadOnly(true);
	mainWindow()->dive_list()->setEnabled(true);

	ui->editAccept->hide();
	ui->editReset->hide();
	editMode = NONE;
}

#define EDIT_TEXT(what, text) \
	QByteArray textByteArray = text.toLocal8Bit(); \
	free(what);\
	what = strdup(textByteArray.data());

void MainTab::on_buddy_textChanged(const QString& text)
{
	if (!currentDive)
		return;
	EDIT_TEXT(currentDive->buddy, text);
}

void MainTab::on_divemaster_textChanged(const QString& text)
{
	if (!currentDive)
		return;
	EDIT_TEXT(currentDive->divemaster, text);
}

void MainTab::on_location_textChanged(const QString& text)
{
	if (editMode == TRIP && mainWindow() && mainWindow()->dive_list()->selectedTrips.count() == 1) {
		// we are editing a trip
		dive_trip_t *currentTrip = *mainWindow()->dive_list()->selectedTrips.begin();
		EDIT_TEXT(currentTrip->location, text);
	} else if (editMode == DIVE){
		if (!currentDive)
			return;
		EDIT_TEXT(currentDive->location, text);
	}
}

void MainTab::on_suit_textChanged(const QString& text)
{
	if (!currentDive)
		return;
	EDIT_TEXT(currentDive->suit, text);
}

void MainTab::on_notes_textChanged()
{
	if (editMode == TRIP && mainWindow() && mainWindow()->dive_list()->selectedTrips.count() == 1) {
		// we are editing a trip
		dive_trip_t *currentTrip = *mainWindow()->dive_list()->selectedTrips.begin();
		EDIT_TEXT(currentTrip->notes, ui->notes->toPlainText());
	} else if (editMode == DIVE) {
		if (!currentDive)
			return;
		EDIT_TEXT(currentDive->notes, ui->notes->toPlainText());
	}
}

#undef EDIT_TEXT

void MainTab::on_rating_valueChanged(int value)
{
	if (!currentDive)
		return;
	currentDive->rating = value;
}

void MainTab::on_visibility_valueChanged(int value)
{
	if (!currentDive)
		return;
	currentDive->visibility = value;
}
