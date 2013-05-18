/*
 * addcylinderdialog.cpp
 *
 * classes for the add cylinder dialog of Subsurface
 *
 */
#include "addcylinderdialog.h"
#include "ui_addcylinderdialog.h"
#include <QComboBox>
#include <QDoubleSpinBox>
#include "../conversions.h"
#include "models.h"

AddCylinderDialog::AddCylinderDialog(QWidget *parent) : ui(new Ui::AddCylinderDialog())
, tankInfoModel(new TankInfoModel())
{
	ui->setupUi(this);
	ui->cylinderType->setModel(tankInfoModel);
}

void AddCylinderDialog::setCylinder(cylinder_t *cylinder)
{
	double volume, pressure;
	int index;

	currentCylinder = cylinder;
	convert_volume_pressure(cylinder->type.size.mliter, cylinder->type.workingpressure.mbar, &volume, &pressure);

	index = ui->cylinderType->findText(QString(cylinder->type.description));
	ui->cylinderType->setCurrentIndex(index);
	ui->size->setValue(volume);
	ui->pressure->setValue(pressure);

	ui->o2percent->setValue(cylinder->gasmix.o2.permille / 10.0);
	ui->hepercent->setValue(cylinder->gasmix.he.permille / 10.0);

	convert_pressure(cylinder->start.mbar, &pressure);
	ui->start->setValue(pressure);

	convert_pressure(cylinder->end.mbar, &pressure);
	ui->end->setValue(pressure);
}

void AddCylinderDialog::updateCylinder()
{
	QByteArray description = ui->cylinderType->currentText().toLocal8Bit();

	currentCylinder->type.description = description.data();
	currentCylinder->type.size.mliter = ui->size->value();
	currentCylinder->type.workingpressure.mbar = ui->pressure->value();
	currentCylinder->gasmix.o2.permille = ui->o2percent->value();
	currentCylinder->gasmix.he.permille = ui->hepercent->value();
	currentCylinder->start.mbar = ui->start->value();
	currentCylinder->end.mbar = ui->end->value();
}

