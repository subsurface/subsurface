// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEPLANNER_H
#define DIVEPLANNER_H

#include "core/divemode.h"
#include "core/owning_ptrs.h"

#include <QAbstractTableModel>
#include <QAbstractButton>
#include <QDateTime>

class DivePlannerPointsModel;
class GasSelectionModel;
class DiveTypeSelectionModel;
class PlannerWidgets;
struct dive;

#include "ui_diveplanner.h"

class DivePlannerWidget : public QWidget {
	Q_OBJECT
public:
	explicit DivePlannerWidget(dive &planned_dive, PlannerWidgets *parent);
	~DivePlannerWidget();
	void setReplanButton(bool replan);
public
slots:
	void setupStartTime(QDateTime startTime);
	void settingsChanged();
	void atmPressureChanged(const int pressure);
	void heightChanged(const int height);
	void waterTypeChanged(const int index);
	void customSalinityChanged(double density);
	void setSurfacePressure(int surface_pressure);
	void setSalinity(int salinity);
private:
	Ui::DivePlanner ui;
	QAbstractButton *replanButton;
	void waterTypeUpdateTexts();
};

#include "ui_plannerSettings.h"

class PlannerSettingsWidget : public QWidget {
	Q_OBJECT
public:
	explicit PlannerSettingsWidget(PlannerWidgets *parent);
	~PlannerSettingsWidget();
public
slots:
	void settingsChanged();
	void setBackgasBreaks(bool dobreaks);
	void disableDecoElements(int mode, divemode_t rebreathermode);
	void disableBackgasBreaks(bool enabled);
	void setDiveMode(int mode);
	void setBailoutVisibility(int mode);

private:
	Ui::plannerSettingsWidget ui;
	void updateUnitsUI();
};

#include "ui_plannerDetails.h"

class PlannerDetails : public QWidget {
	Q_OBJECT
public:
	explicit PlannerDetails(QWidget *parent = 0);
	QPushButton *printPlan() const { return ui.printPlan; }
	QTextEdit *divePlanOutput() const { return ui.divePlanOutput; }
public
slots:
	void setPlanNotes(QString plan);

private:
	Ui::plannerDetails ui;
};

// The planner widgets make up three quadrants
class PlannerWidgets : public QObject {
	Q_OBJECT
public:
	PlannerWidgets();
	~PlannerWidgets();
	void preparePlanDive(const dive *currentDive); // Create a new planned dive
	void planDive();
	void prepareReplanDive(const dive *d); // Make a copy of the dive to be replanned
	void replanDive(int currentDC);
	struct dive *getDive() const;
	divemode_t getRebreatherMode() const;
public
slots:
	void printDecoPlan();
public:
	void repopulateGasModel();
	OwningDivePtr planned_dive;
	std::unique_ptr<GasSelectionModel> gasModel;
	std::unique_ptr<DiveTypeSelectionModel> diveTypeModel;
	DivePlannerWidget plannerWidget;
	PlannerSettingsWidget plannerSettingsWidget;
	PlannerDetails plannerDetails;
};

#endif // DIVEPLANNER_H
