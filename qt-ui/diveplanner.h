#ifndef DIVEPLANNER_H
#define DIVEPLANNER_H

#include <QGraphicsView>
#include <QGraphicsPathItem>
#include <QDialog>
#include <QAbstractTableModel>
#include <QDateTime>

#include "dive.h"

class QListView;
class QModelIndex;

class DivePlannerPointsModel : public QAbstractTableModel {
	Q_OBJECT
public:
	static DivePlannerPointsModel *instance();
	enum Sections {
		REMOVE,
		DEPTH,
		DURATION,
		RUNTIME,
		GAS,
		CCSETPOINT,
		COLUMNS
	};
	enum Mode {
		NOTHING,
		PLAN,
		ADD
	};
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	void removeSelectedPoints(const QVector<int> &rows);
	void setPlanMode(Mode mode);
	bool isPlanner();
	void createSimpleDive();
	void clear();
	Mode currentMode() const;
	bool setRecalc(bool recalc);
	bool recalcQ();
	void tanksUpdated();
	void rememberTanks();
	bool tankInUse(int o2, int he);
	void copyCylinders(struct dive *d);
	void copyCylindersFrom(struct dive *d);
	/**
	 * @return the row number.
	 */
	void editStop(int row, divedatapoint newData);
	divedatapoint at(int row);
	int size();
	struct diveplan &getDiveplan();
	QStringList &getGasList();
	QVector<QPair<int, int> > collectGases(dive *d);
	int lastEnteredPoint();
	void removeDeco();
	static bool addingDeco;

public
slots:
	int addStop(int millimeters = 0, int seconds = 0, int o2 = 0, int he = 0, int ccpoint = 0, bool entered = true);
	void addCylinder_clicked();
	void setGFHigh(const int gfhigh);
	void setGFLow(const int ghflow);
	void setSurfacePressure(int pressure);
	void setBottomSac(int sac);
	void setDecoSac(int sac);
	void setStartTime(const QTime &t);
	void setLastStop6m(bool value);
	void createPlan();
	void remove(const QModelIndex &index);
	void cancelPlan();
	void createTemporaryPlan();
	void deleteTemporaryPlan();
	void loadFromDive(dive *d);
	void restoreBackupDive();

signals:
	void planCreated();
	void planCanceled();
	void cylinderModelEdited();

private:
	explicit DivePlannerPointsModel(QObject *parent = 0);
	bool addGas(int o2, int he);
	struct diveplan diveplan;
	Mode mode;
	bool recalc;
	QVector<divedatapoint> divepoints;
	struct dive *tempDive;
	struct dive backupDive;
	void deleteTemporaryPlan(struct divedatapoint *dp);
	QVector<sample> backupSamples; // For editing added dives.
	struct dive *stagingDive;
	QVector<QPair<int, int> > oldGases;
};

class DiveHandler : public QObject, public QGraphicsEllipseItem {
	Q_OBJECT
public:
	DiveHandler();

protected:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
signals:
	void moved();
private:
	int parentIndex();
public
slots:
	void selfRemove();
	void changeGas();
};

class DivePlannerGraphics : public QGraphicsView {
	Q_OBJECT
public:
	DivePlannerGraphics(QWidget *parent = 0);
private
slots:

};

#include "ui_diveplanner.h"

class DivePlannerWidget : public QWidget {
	Q_OBJECT
public:
	explicit DivePlannerWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);

public
slots:
	void settingsChanged();
	void atmPressureChanged(const QString &pressure);
	void bottomSacChanged(const QString &bottomSac);
	void decoSacChanged(const QString &decosac);

private:
	Ui::DivePlanner ui;
};

QString gasToStr(const int o2Permille, const int hePermille);
QString dpGasToStr(const divedatapoint &p);

#endif // DIVEPLANNER_H
