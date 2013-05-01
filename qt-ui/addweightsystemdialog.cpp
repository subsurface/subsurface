/*
 * addweightsystemdialog.cpp
 *
 * classes for the add weightsystem dialog of Subsurface
 *
 */
#include "addweightsystemdialog.h"
#include "ui_addweightsystemdialog.h"
#include <QComboBox>
#include <QDoubleSpinBox>
#include "../conversions.h"
#include "models.h"

AddWeightsystemDialog::AddWeightsystemDialog(QWidget *parent) : ui(new Ui::AddWeightsystemDialog())
{
	ui->setupUi(this);
	currentWeightsystem = NULL;
}

void AddWeightsystemDialog::setWeightsystem(weightsystem_t *ws)
{
	currentWeightsystem = ws;

	ui->description->insert(QString(ws->description));
	if (get_units()->weight == units::KG)
		ui->weight->setValue(ws->weight.grams / 1000);
	else
		ui->weight->setValue(grams_to_lbs(ws->weight.grams));
}

void AddWeightsystemDialog::updateWeightsystem()
{
	currentWeightsystem->description = strdup(ui->description->text().toUtf8().data());
	if (get_units()->weight == units::KG)
		currentWeightsystem->weight.grams = ui->weight->value() * 1000;
	else
		currentWeightsystem->weight.grams = lbs_to_grams(ui->weight->value());
}

