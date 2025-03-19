// SPDX-License-Identifier: GPL-2.0
#ifndef TAB_DIVE_EQUIPMENT_H
#define TAB_DIVE_EQUIPMENT_H

#include "TabBase.h"
#include "ui_TabDiveEquipment.h"
#include "qt-models/completionmodels.h"
#include "desktop-widgets/divelistview.h"
#include "desktop-widgets/modeldelegates.h"

namespace Ui {
	class TabDiveEquipment;
};

class WeightModel;
class CylindersModel;

class TabDiveEquipment : public TabBase {
	Q_OBJECT
public:
	TabDiveEquipment(MainTab *parent);
	~TabDiveEquipment();
	void updateData(const std::vector<dive *> &selection, dive *currentDive, int currentDC) override;
	void clear() override;
	void closeWarning();

private slots:
	void divesChanged(const QVector<dive *> &dives, DiveField field);
	void addCylinder_clicked();
	void addWeight_clicked();
	void toggleTriggeredColumn();
	void editCylinderWidget(const QModelIndex &index);
	void editWeightWidget(const QModelIndex &index);
	void on_suit_editingFinished();
	void divesEdited(int count);
	void diveComputerEdited(dive &dive, divecomputer &dc);
	void cylinderRemoved(struct dive *dive, int);

private:
	Ui::TabDiveEquipment ui;
	SuitCompletionModel suitModel;
	CylindersModel *cylindersModel;
	WeightModel *weightModel;

	TankInfoDelegate tankInfoDelegate;
	TankUseDelegate tankUseDelegate;
	SensorDelegate sensorDelegate;
	WSInfoDelegate wsInfoDelegate;
	void setCylinderColumnVisibility();
};

#endif // TAB_DIVE_EQUIPMENT_H
