// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEPLANNERMODEL_H
#define DIVEPLANNERMODEL_H

#include <QAbstractTableModel>
#include <QDateTime>

#include "core/deco.h"
#include "core/planner.h"
#include "qt-models/cylindermodel.h"

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
		DIVEMODE,
		COLUMNS
	};
	enum Mode {
		NOTHING,
		PLAN,
		ADD
	};
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	void gasChange(const QModelIndex &index, int newcylinderid);
	void cylinderRenumber(int mapping[]);
	void removeSelectedPoints(const QVector<int> &rows);
	void setPlanMode(Mode mode);
	bool isPlanner();
	void createSimpleDive();
	void clear();
	Mode currentMode() const;
	bool setRecalc(bool recalc);
	bool recalcQ();
	bool tankInUse(int cylinderid);
	CylindersModel *cylindersModel();

	int ascrate75Display();
	int ascrate50Display();
	int ascratestopsDisplay();
	int ascratelast6mDisplay();
	int descrateDisplay();
	int getSurfacePressure();

	/**
	 * @return the row number.
	 */
	void editStop(int row, divedatapoint newData);
	divedatapoint at(int row);
	int size();
	struct diveplan &getDiveplan();
	void removeDeco();
	static bool addingDeco;
	struct deco_state final_deco_state;

	void loadFromDive(dive *d);
public
slots:
	int addStop(int millimeters = 0, int seconds = 0, int cylinderid_in = -1, int ccpoint = 0, bool entered = true, enum divemode_t = UNDEF_COMP_TYPE);
	void addCylinder_clicked();
	void setGFHigh(const int gfhigh);
	void setGFLow(const int gflow);
	void setVpmbConservatism(int level);
	void setSurfacePressure(int pressure);
	void setSalinity(int salinity);
	void setBottomSac(double sac);
	void setDecoSac(double sac);
	void setStartTime(const QTime &t);
	void setStartDate(const QDate &date);
	void setLastStop6m(bool value);
	void setDropStoneMode(bool value);
	void setVerbatim(bool value);
	void setDisplayRuntime(bool value);
	void setDisplayDuration(bool value);
	void setDisplayTransitions(bool value);
	void setDisplayVariations(bool value);
	void setDecoMode(int mode);
	void setSafetyStop(bool value);
	void savePlan();
	void saveDuplicatePlan();
	void remove(const QModelIndex &index);
	void cancelPlan();
	void createTemporaryPlan();
	void deleteTemporaryPlan();
	void emitDataChanged();
	void setRebreatherMode(int mode);
	void setReserveGas(int reserve);
	void setSwitchAtReqStop(bool value);
	void setMinSwitchDuration(int duration);
	void setSurfaceSegment(int duration);
	void setSacFactor(double factor);
	void setProblemSolvingTime(int minutes);
	void setAscrate75Display(int rate);
	void setAscrate50Display(int rate);
	void setAscratestopsDisplay(int rate);
	void setAscratelast6mDisplay(int rate);
	void setDescrateDisplay(int rate);

signals:
	void planCreated();
	void planCanceled();
	void cylinderModelEdited();
	void recreationChanged(bool);
	void calculatedPlanNotes(QString);
	void variationsComputed(QString);

private:
	explicit DivePlannerPointsModel(QObject *parent = 0);
	void setupStartTime();
	void setupCylinders();
	int lastEnteredPoint();
	bool updateMaxDepth();
	void createPlan(bool replanCopy);
	struct diveplan diveplan;
	struct divedatapoint *cloneDiveplan(struct diveplan *plan_src, struct diveplan *plan_copy);
	void computeVariationsDone(QString text);
	void computeVariations(struct diveplan *diveplan, const struct deco_state *ds);
	void computeVariationsFreeDeco(struct diveplan *diveplan, struct deco_state *ds);
	int analyzeVariations(struct decostop *min, struct decostop *mid, struct decostop *max, const char *unit);
	CylindersModel cylinders;
	Mode mode;
	bool recalc;
	QVector<divedatapoint> divepoints;
	QDateTime startTime;
	int instanceCounter = 0;
	struct deco_state ds_after_previous_dives;
	duration_t preserved_until;
};

#endif
