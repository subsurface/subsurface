/*
 * maintab.cpp
 *
 * classes for the "notebook" area of the main window of Subsurface
 *
 */
#include "maintab.h"
#include "ui_maintab.h"
#include "addcylinderdialog.h"

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
	ui->sac->setText(QString());
	ui->otu->setText(QString());
	ui->oxygenhelium->setText(QString());
	ui->gasused->setText(QString());
	ui->date->setText(QString());
	ui->divetime->setText(QString());
	ui->surfinterval->setText(QString());
	ui->maxdepth->setText(QString());
	ui->avgdepth->setText(QString());
	ui->visibility->setText(QString());
	ui->watertemperature->setText(QString());
	ui->airtemperature->setText(QString());
	ui->airpress->setText(QString());
}

void MainTab::clearStats()
{
	ui->maxdepth_2->setText(QString());
	ui->mindepth->setText(QString());
	ui->avgdepth->setText(QString());
	ui->maxsac->setText(QString());
	ui->minsac->setText(QString());
	ui->avgsac->setText(QString());
	ui->dives->setText(QString());
	ui->maxtemp->setText(QString());
	ui->mintemp->setText(QString());
	ui->avgtemp->setText(QString());
	ui->totaltime->setText(QString());
	ui->avgtime->setText(QString());
	ui->longestdive->setText(QString());
	ui->shortestdive->setText(QString());
}

void MainTab::on_addCylinder_clicked()
{
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

void MainTab::reload()
{
	cylindersModel->update();
}
