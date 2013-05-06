/*
 * maintab.h
 *
 * header file for the main tab of Subsurface
 *
 */
#ifndef MAINTAB_H
#define MAINTAB_H

#include <QTabWidget>
#include <QDialog>

#include "models.h"

namespace Ui
{
	class MainTab;
}

class MainTab : public QTabWidget
{
	Q_OBJECT
public:
	MainTab(QWidget *parent);
	void clearStats();
	void clearInfo();
	void clearEquipment();
	void reload();

public Q_SLOTS:
	void on_addCylinder_clicked();
	void on_editCylinder_clicked();
	void on_delCylinder_clicked();
	void on_addWeight_clicked();
	void on_editWeight_clicked();
	void on_delWeight_clicked();

	void updateDiveInfo(int dive);

private:
	Ui::MainTab *ui;
	WeightModel *weightModel;
	CylindersModel *cylindersModel;
};

#endif
