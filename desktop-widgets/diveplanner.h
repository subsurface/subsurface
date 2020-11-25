// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEPLANNER_H
#define DIVEPLANNER_H

#include <QAbstractTableModel>
#include <QAbstractButton>
#include <QDateTime>

class QListView;
class QModelIndex;
class DivePlannerPointsModel;

#include "ui_diveplanner.h"

class DivePlannerWidget : public QWidget {
	Q_OBJECT
public:
	explicit DivePlannerWidget(QWidget *parent = 0);
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
	explicit PlannerSettingsWidget(QWidget *parent = 0);
	~PlannerSettingsWidget();
public
slots:
	void settingsChanged();
	void setBackgasBreaks(bool dobreaks);
	void disableDecoElements(int mode);
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
	void planDive();
	void replanDive();
public
slots:
	void printDecoPlan();
public:
	DivePlannerWidget plannerWidget;
	PlannerSettingsWidget plannerSettingsWidget;
	PlannerDetails plannerDetails;
};

#endif // DIVEPLANNER_H
