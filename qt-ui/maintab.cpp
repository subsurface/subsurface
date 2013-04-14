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
	QList<QLabel*> labels;
	labels 	<< ui->sac << ui->otu << ui->oxygenhelium << ui->gasused
		<< ui->date << ui->divetime << ui->surfinterval
		<< ui->maxdepth << ui->avgdepth << ui->visibility
		<< ui->watertemperature << ui->airtemperature << ui->airpress;

	Q_FOREACH(QLabel *l, labels){
		l->setText(QString());
	}
}

void MainTab::clearStats()
{
	QList<QLabel*> labels;
	labels << ui->maxdepth_2 << ui->mindepth << ui->avgdepth
		<< ui->maxsac << ui->minsac << ui->avgsac
		<< ui->dives << ui->maxtemp << ui->mintemp << ui->avgtemp
		<< ui->totaltime << ui->avgtime << ui->longestdive << ui->shortestdive;

	Q_FOREACH(QLabel *l, labels){
		l->setText(QString());
	}
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
