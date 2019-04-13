// SPDX-License-Identifier: GPL-2.0
#ifndef TAB_DIVE_EQUIPMENT_H
#define TAB_DIVE_EQUIPMENT_H

#include "TabBase.h"
#include "ui_TabDiveEquipment.h"

namespace Ui {
	class TabDiveEquipment;
};

class WeightModel;
class CylindersModel;

class TabDiveEquipment : public TabBase {
	Q_OBJECT
public:
	TabDiveEquipment(QWidget *parent = 0);
	~TabDiveEquipment();
	void updateData() override;
	void clear() override;
	void acceptChanges();
	void rejectChanges();
private slots:
	void addCylinder_clicked();
	void addWeight_clicked();
	void toggleTriggeredColumn();
	void editCylinderWidget(const QModelIndex &index);
	void editWeightWidget(const QModelIndex &index);
private:
	Ui::TabDiveEquipment ui;

	CylindersModel *cylindersModel;
	WeightModel *weightModel;
};

#endif // TAB_DIVE_EQUIPMENT_H
