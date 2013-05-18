/*
 * addcylinderdialog.h
 *
 * header file for the add cylinder dialog of Subsurface
 *
 */
#ifndef ADDCYLINDERDIALOG_H
#define ADDCYLINDERDIALOG_H

#include <QDialog>
#include "../dive.h"

namespace Ui{
	class AddCylinderDialog;
}

class TankInfoModel;

class AddCylinderDialog : public QDialog{
	Q_OBJECT
public:
	explicit AddCylinderDialog(QWidget* parent = 0);
	void setCylinder(cylinder_t *cylinder);
	void updateCylinder();

private:
	Ui::AddCylinderDialog *ui;
	cylinder_t *currentCylinder;
	TankInfoModel *tankInfoModel;
};


#endif
