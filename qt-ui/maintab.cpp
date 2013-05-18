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
#include "../helpers.h"
#include "../statistics.h"

#include <QLabel>

MainTab::MainTab(QWidget *parent) : QTabWidget(parent),
				    ui(new Ui::MainTab()),
				    weightModel(new WeightModel()),
				    cylindersModel(new CylindersModel())
{
	ui->setupUi(this);
	ui->cylinders->setModel(cylindersModel);
	ui->weights->setModel(weightModel);

	/* example of where code is more concise than Qt designer */
	QList<QObject *> infoTabWidgets = ui->infoTab->children();
	Q_FOREACH( QObject* obj, infoTabWidgets ){
		QLabel* label = qobject_cast<QLabel *>(obj);
		if (label)
			label->setAlignment(Qt::AlignHCenter);
	}
	QList<QObject *> statisticsTabWidgets = ui->statisticsTab->children();
	Q_FOREACH( QObject* obj, statisticsTabWidgets ){
		QLabel* label = qobject_cast<QLabel *>(obj);
		if (label)
			label->setAlignment(Qt::AlignHCenter);
	}
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
	ui->visibilityText->clear();
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
	struct dive *d = get_dive(dive);
	UPDATE_TEXT(d, notes);
	UPDATE_TEXT(d, location);
	UPDATE_TEXT(d, suit);
	UPDATE_TEXT(d, divemaster);
	UPDATE_TEXT(d, buddy);
	/* infoTab */
	if (d) {
		ui->rating->setCurrentStars(d->rating);
		ui->maximumDepthText->setText(get_depth_string(d->maxdepth, TRUE));
		ui->averageDepthText->setText(get_depth_string(d->meandepth, TRUE));
		ui->otuText->setText(QString("%1").arg(d->otu));
		ui->waterTemperatureText->setText(get_temperature_string(d->watertemp, TRUE));
		ui->airTemperatureText->setText(get_temperature_string(d->airtemp, TRUE));
		ui->gasUsedText->setText(get_volume_string(get_gas_used(d), TRUE));
		if ((sacVal.mliter = d->sac) > 0)
			ui->sacText->setText(get_volume_string(sacVal, TRUE).append("/min"));
		else
			ui->sacText->clear();
		if (d->surface_pressure.mbar)
			/* this is ALWAYS displayed in mbar */
			ui->airPressureText->setText(QString("%1mbar").arg(d->surface_pressure.mbar));
		else
			ui->airPressureText->clear();
	} else {
		ui->rating->setCurrentStars(0);
		ui->sacText->clear();
		ui->otuText->clear();
		ui->oxygenHeliumText->clear();
		ui->dateText->clear();
		ui->diveTimeText->clear();
		ui->surfaceIntervalText->clear();
		ui->maximumDepthText->clear();
		ui->averageDepthText->clear();
		ui->visibilityText->clear();
		ui->waterTemperatureText->clear();
		ui->airTemperatureText->clear();
		ui->gasUsedText->clear();
		ui->airPressureText->clear();
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
	if (result == QDialog::Rejected){
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
	cylindersModel->update();
}
