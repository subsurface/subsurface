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

#include <QLabel>

MainTab::MainTab(QWidget *parent) : QTabWidget(parent),
				    ui(new Ui::MainTab()),
				    weightModel(new WeightModel()),
				    cylindersModel(new CylindersModel())
{
	ui->setupUi(this);
	ui->cylinders->setModel(cylindersModel);
	ui->weights->setModel(weightModel);
}

void MainTab::clearEquipment()
{
}

void MainTab::clearInfo()
{
	ui->sacText->setText(QString());
	ui->otuText->setText(QString());
	ui->oxygenHeliumText->setText(QString());
	ui->gasUsedText->setText(QString());
	ui->dateText->setText(QString());
	ui->diveTimeText->setText(QString());
	ui->surfaceIntervalText->setText(QString());
	ui->maximumDepthText->setText(QString());
	ui->averageDepthText->setText(QString());
	ui->visibilityText->setText(QString());
	ui->waterTemperatureText->setText(QString());
	ui->airTemperatureText->setText(QString());
	ui->airPressureText->setText(QString());
}

void MainTab::clearStats()
{
	ui->maximumDepthAllText->setText(QString());
	ui->minimumDepthAllText->setText(QString());
	ui->averageDepthAllText->setText(QString());
	ui->maximumSacAllText->setText(QString());
	ui->minimumSacAllText->setText(QString());
	ui->averageSacAllText->setText(QString());
	ui->divesAllText->setText(QString());
	ui->maximumTemperatureAllText->setText(QString());
	ui->minimumTemperatureAllText->setText(QString());
	ui->averageTemperatureAllText->setText(QString());
	ui->totalTimeAllText->setText(QString());
	ui->averageTimeAllText->setText(QString());
	ui->longestAllText->setText(QString());
	ui->shortestAllText->setText(QString());
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
	// If you are unsure what's the name of anything,
	// open the file maintab.ui on the designer
	// click on the item and check its objectName,
	// the access is ui->objectName from here on.

	struct dive *d = get_dive(dive);
	UPDATE_TEXT(d, notes);
	UPDATE_TEXT(d, location);
	UPDATE_TEXT(d, suit);
	UPDATE_TEXT(d, divemaster);
	UPDATE_TEXT(d, buddy);
	if (d)
		ui->rating->setCurrentStars(d->rating);
	else
		ui->rating->setCurrentStars(0);
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
