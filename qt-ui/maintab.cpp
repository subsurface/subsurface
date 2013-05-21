/*
 * maintab.cpp
 *
 * classes for the "notebook" area of the main window of Subsurface
 *
 */
#include "maintab.h"
#include "ui_maintab.h"
#include "addcylinderdialog.h"
#include "addweightsystemdialog.h"
#include "mainwindow.h"
#include "../helpers.h"
#include "../statistics.h"
#include "divelistview.h"

#include <QLabel>
#include <QDebug>

MainTab::MainTab(QWidget *parent) : QTabWidget(parent),
				    ui(new Ui::MainTab()),
				    weightModel(new WeightModel()),
				    cylindersModel(new CylindersModel()),
				    currentDive(0)
{
	ui->setupUi(this);
	ui->cylinders->setModel(cylindersModel);
	ui->weights->setModel(weightModel);
	ui->diveNotesMessage->hide();
	ui->diveNotesMessage->setCloseButtonVisible(false);

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

	/* example of where code is more concise than Qt designer */
	QList<QObject *> infoTabWidgets = ui->infoTab->children();
	Q_FOREACH(QObject* obj, infoTabWidgets) {
		QLabel* label = qobject_cast<QLabel *>(obj);
		if (label)
			label->setAlignment(Qt::AlignHCenter);
	}
	QList<QObject *> statisticsTabWidgets = ui->statisticsTab->children();
	Q_FOREACH(QObject* obj, statisticsTabWidgets) {
		QLabel* label = qobject_cast<QLabel *>(obj);
		if (label)
			label->setAlignment(Qt::AlignHCenter);
	}
}

bool MainTab::eventFilter(QObject* object, QEvent* event)
{
	if (event->type() == QEvent::FocusIn) {
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
	ui->maximumDepthAllText->clear();
	ui->minimumDepthAllText->clear();
	ui->averageDepthAllText->clear();
	ui->maximumSacAllText->clear();
	ui->minimumSacAllText->clear();
	ui->averageSacAllText->clear();
	ui->divesAllText->clear();
	ui->maximumTemperatureAllText->clear();
	ui->minimumTemperatureAllText->clear();
	ui->averageTemperatureAllText->clear();
	ui->totalTimeAllText->clear();
	ui->averageTimeAllText->clear();
	ui->longestAllText->clear();
	ui->shortestAllText->clear();
}

#define UPDATE_TEXT(d, field)				\
	if (!d || !d->field)				\
		ui->field->setText("");			\
	else						\
		ui->field->setText(d->field)


void MainTab::updateDiveInfo(int dive)
{
	// So, this is what happens now:
	// Every tab should be populated from this method,
	// it will be called whenever a new dive is selected
	// I'm already populating the 'notes' box
	// to show how it can be done.
	// If you are unsure about the name of something,
	// open the file maintab.ui on the designer
	// click on the item and check its objectName,
	// the access is ui->objectName from here on.
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
	/* infoTab */
	if (d) {
		/* make the fields writeable */
		ui->location->setReadOnly(false);
		ui->divemaster->setReadOnly(false);
		ui->buddy->setReadOnly(false);
		ui->suit->setReadOnly(false);
		ui->notes->setReadOnly(false);
		ui->rating->setReadOnly(false);
		ui->visibility->setReadOnly(false);
		/* and fill them from the dive */
		ui->rating->setCurrentStars(d->rating);
		ui->maximumDepthText->setText(get_depth_string(d->maxdepth, TRUE));
		ui->averageDepthText->setText(get_depth_string(d->meandepth, TRUE));
		ui->otuText->setText(QString("%1").arg(d->otu));
		ui->waterTemperatureText->setText(get_temperature_string(d->watertemp, TRUE));
		ui->airTemperatureText->setText(get_temperature_string(d->airtemp, TRUE));
		ui->gasUsedText->setText(get_volume_string(get_gas_used(d), TRUE));
		ui->oxygenHeliumText->setText(get_gaslist(d));
		ui->dateText->setText(get_dive_date_string(d->when));
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
		ui->visibility->setCurrentStars(d->visibility);
		ui->maximumDepthAllText->setText(get_depth_string(stats_selection.max_depth, TRUE));
		ui->minimumDepthAllText->setText(get_depth_string(stats_selection.min_depth, TRUE));
		ui->averageDepthAllText->setText(get_depth_string(stats_selection.avg_depth, TRUE));
		ui->maximumSacAllText->setText(get_volume_string(stats_selection.max_sac, TRUE).append(tr("/min")));
		ui->minimumSacAllText->setText(get_volume_string(stats_selection.min_sac, TRUE).append(tr("/min")));
		ui->averageSacAllText->setText(get_volume_string(stats_selection.avg_sac, TRUE).append(tr("/min")));
		ui->divesAllText->setText(QString::number(stats_selection.selection_size));
		temp.mkelvin = stats_selection.max_temp;
		ui->maximumTemperatureAllText->setText(get_temperature_string(temp, TRUE));
		temp.mkelvin = stats_selection.min_temp;
		ui->minimumTemperatureAllText->setText(get_temperature_string(temp, TRUE));
		if (stats_selection.combined_temp && stats_selection.combined_count) {
			const char *unit;
			get_temp_units(0, &unit);
			ui->averageTemperatureAllText->setText(QString("%1%2").arg(stats_selection.combined_temp / stats_selection.combined_count, 0, 'f', 1).arg(unit));
		}
		ui->totalTimeAllText->setText(get_time_string(stats_selection.total_time.seconds, 0));
		int seconds = stats_selection.total_time.seconds;
		if (stats_selection.selection_size)
			seconds /= stats_selection.selection_size;
		ui->averageTimeAllText->setText(get_time_string(seconds, 0));
		ui->longestAllText->setText(get_time_string(stats_selection.longest_time.seconds, 0));
		ui->shortestAllText->setText(get_time_string(stats_selection.shortest_time.seconds, 0));
		cylindersModel->setDive(d);
		weightModel->setDive(d);
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
	}
	/* statisticsTab*/
	/* we can access the stats_selection struct, but how do we ensure the relevant dives are selected
	 * if we don't use the gtk widget to drive this?
	 * Maybe call process_selected_dives? Or re-write to query our Qt list view.
	 */
// 	qDebug("max temp %u",stats_selection.max_temp);
// 	qDebug("min temp %u",stats_selection.min_temp);
}

void MainTab::on_addCylinder_clicked()
{
	if (cylindersModel->rowCount() >= MAX_CYLINDERS)
		return;

	AddCylinderDialog dialog(this);
	cylinder_t *newCylinder = (cylinder_t*) malloc(sizeof(cylinder_t));
	newCylinder->type.description = "";

	dialog.setCylinder(newCylinder);
	int result = dialog.exec();
	if (result == QDialog::Rejected) {
		return;
	}

	dialog.updateCylinder();
	cylindersModel->add(newCylinder);
}

void MainTab::on_editCylinder_clicked()
{
}

void MainTab::on_delCylinder_clicked()
{
}

void MainTab::on_addWeight_clicked()
{
	if (weightModel->rowCount() >= MAX_WEIGHTSYSTEMS)
		return;

	AddWeightsystemDialog dialog(this);
	weightsystem_t newWeightsystem;
	newWeightsystem.description = "";
	newWeightsystem.weight.grams = 0;

	dialog.setWeightsystem(&newWeightsystem);
	int result = dialog.exec();
	if (result == QDialog::Rejected)
		return;

	dialog.updateWeightsystem();
	weightModel->add(&newWeightsystem);
}

void MainTab::on_editWeight_clicked()
{
}

void MainTab::on_delWeight_clicked()
{
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
		ui->diveNotesMessage->setText(tr("This dive is being edited. click on finish / reset when ready."));
		ui->diveNotesMessage->animatedShow();
		notesBackup.buddy = ui->buddy->text();
		notesBackup.suit = ui->suit->text();
		notesBackup.notes = ui->notes->toPlainText();
		notesBackup.divemaster = ui->divemaster->text();
		notesBackup.location = ui->location->text();
		notesBackup.rating = ui->rating->currentStars();
		notesBackup.visibility = ui->visibility->currentStars();
	} else {
		ui->diveNotesMessage->animatedHide();
		ui->editAccept->hide();
		ui->editReset->hide();
		/* now figure out if things have changed */
		if (notesBackup.buddy != ui->buddy->text() ||
		    notesBackup.suit != ui->suit->text() ||
		    notesBackup.notes != ui->notes->toPlainText() ||
		    notesBackup.divemaster != ui->divemaster->text() ||
		    notesBackup.location != ui->location->text() ||
		    notesBackup.visibility != ui->visibility->currentStars() ||
		    notesBackup.rating != ui->rating->currentStars())
			mark_divelist_changed(TRUE);
	}
}

void MainTab::on_editReset_clicked()
{
	if (!ui->editAccept->isChecked())
		return;

	ui->buddy->setText(notesBackup.buddy);
	ui->suit->setText(notesBackup.suit);
	ui->notes->setText(notesBackup.notes);
	ui->divemaster->setText(notesBackup.divemaster);
	ui->location->setText(notesBackup.location);
	ui->rating->setCurrentStars(notesBackup.rating);
	ui->visibility->setCurrentStars(notesBackup.visibility);
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
}

#define EDIT_NOTES(what, text) \
	QByteArray textByteArray = text.toLocal8Bit(); \
	free(currentDive->what);\
	currentDive->what = strdup(textByteArray.data());

void MainTab::on_buddy_textChanged(const QString& text)
{
	if (!currentDive)
		return;
	EDIT_NOTES(buddy, text);
}

void MainTab::on_divemaster_textChanged(const QString& text)
{
	if (!currentDive)
		return;
	EDIT_NOTES(divemaster, text);
}

void MainTab::on_location_textChanged(const QString& text)
{
	if (!currentDive)
		return;
	EDIT_NOTES(location, text);
}

void MainTab::on_suit_textChanged(const QString& text)
{
	if (!currentDive)
		return;
	EDIT_NOTES(suit, text);
}

void MainTab::on_notes_textChanged()
{
	if (!currentDive)
		return;
	EDIT_NOTES(notes, ui->notes->toPlainText());
}

#undef EDIT_NOTES

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
