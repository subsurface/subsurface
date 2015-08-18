#ifndef DIVEPLANNER_H
#define DIVEPLANNER_H

#include <QGraphicsPathItem>
#include <QAbstractTableModel>
#include <QAbstractButton>
#include <QDateTime>
#include <QSignalMapper>

#include "dive.h"

class QListView;
class QModelIndex;
class DivePlannerPointsModel;

class DiveHandler : public QObject, public QGraphicsEllipseItem {
	Q_OBJECT
public:
	DiveHandler();

protected:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
signals:
	void moved();
	void clicked();
	void released();
private:
	int parentIndex();
public
slots:
	void selfRemove();
	void changeGas();
private:
	QTime t;
};

#include "ui_diveplanner.h"

class DivePlannerWidget : public QWidget {
	Q_OBJECT
public:
	explicit DivePlannerWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);
	void setReplanButton(bool replan);
public
slots:
	void setupStartTime(QDateTime startTime);
	void settingsChanged();
	void atmPressureChanged(const int pressure);
	void heightChanged(const int height);
	void salinityChanged(const double salinity);
	void printDecoPlan();

private:
	Ui::DivePlanner ui;
	QAbstractButton *replanButton;
};

#include "ui_plannerSettings.h"

class PlannerSettingsWidget : public QWidget {
	Q_OBJECT
public:
	explicit PlannerSettingsWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);
	virtual ~PlannerSettingsWidget();
public
slots:
	void settingsChanged();
	void atmPressureChanged(const QString &pressure);
	void bottomSacChanged(const double bottomSac);
	void decoSacChanged(const double decosac);
	void printDecoPlan();
	void setAscRate75(int rate);
	void setAscRate50(int rate);
	void setAscRateStops(int rate);
	void setAscRateLast6m(int rate);
	void setDescRate(int rate);
	void setBottomPo2(double po2);
	void setDecoPo2(double po2);
	void setBackgasBreaks(bool dobreaks);
	void disableDecoElements(int mode);

private:
	Ui::plannerSettingsWidget ui;
	void updateUnitsUI();
	QSignalMapper *modeMapper;
};

#include "ui_plannerDetails.h"

class PlannerDetails : public QWidget {
	Q_OBJECT
public:
	explicit PlannerDetails(QWidget *parent = 0);
	QPushButton *printPlan() const { return ui.printPlan; }
	QTextEdit *divePlanOutput() const { return ui.divePlanOutput; }

private:
	Ui::plannerDetails ui;
};

#endif // DIVEPLANNER_H
