// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEPLANNER_H
#define DIVEPLANNER_H

#include "core/divemode.h"

#include <memory>
#include <QAbstractTableModel>
#include <QAbstractButton>
#include <QDateTime>

class DivePlannerPointsModel;
class PlannerWidgets;
struct dive;

#include "ui_diveplanner.h"

class DivePlannerWidget : public QWidget {
	Q_OBJECT
public:
	explicit DivePlannerWidget(const dive &planned_dive, int &dcNr, PlannerWidgets *parent);
	~DivePlannerWidget();
	void setReplanButton(bool replan);
	void setColumnVisibility(int mode);
	void setDiveMode(int mode);
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
	void disableDecoElements(int mode, divemode_t divemode);
	void disableBackgasBreaks(bool enabled);
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
	void preparePlanDive(const dive *currentDive, int currentDc); // Create a new planned dive
	void planDive();
	void prepareReplanDive(const dive *currentDive, int currentDc); // Make a copy of the dive to be replanned
	void replanDive();
	struct dive *getDive() const;
	int getDcNr();
	divemode_t getDiveMode() const;
	void settingsChanged();
public
slots:
	void printDecoPlan();
	void setDiveMode(int mode);
private:
	std::unique_ptr<dive> planned_dive;
	int dcNr;
public:
	DivePlannerWidget plannerWidget;
	PlannerSettingsWidget plannerSettingsWidget;
	PlannerDetails plannerDetails;
};

#endif // DIVEPLANNER_H
