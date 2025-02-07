// SPDX-License-Identifier: GPL-2.0
#ifndef CYLINDERMODEL_H
#define CYLINDERMODEL_H

#include <QSortFilterProxyModel>

#include "cleanertablemodel.h"
#include "core/equipment.h"

/* Encapsulation of the Cylinder Model, that presents the
 * Current cylinders that are used on a dive. */
class CylindersModel : public CleanerTableModel {
	Q_OBJECT
public:
	enum Column {
		NUMBER,
		REMOVE,
		TYPE,
		SIZE,
		WORKINGPRESS,
		START,
		END,
		O2,
		HE,
		DEPTH,
		MOD,
		MND,
		USE,
		WORKINGPRESS_INT,
		SIZE_INT,
		SENSORS,
		COLUMNS
	};

	enum Roles {
		TEMP_ROLE = Qt::UserRole + 1, // Temporarily set data, but don't store in dive
		COMMIT_ROLE, // Save the temporary data to the dive. Must be set with Column == TYPE.
		REVERT_ROLE // Revert to original data from dive. Must be set with Column == TYPE.
	};
	explicit CylindersModel(bool planner, QObject *parent = 0); // First argument: true if this model is used for the planner
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

	void add();
	void clear();
	void updateDive(dive *d, int dcNr);
	void updateDecoDepths(pressure_t olddecopo2);
	void updateTrashIcon();
	void moveAtFirst(int cylid);
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	bool updateBestMixes();
	void emitDataChanged();

signals:
	void divesEdited(int num);

public
slots:
	void remove(QModelIndex index);
	void cylindersReset(const QVector<dive *> &dives);
	void cylinderAdded(dive *d, int pos);
	void cylinderRemoved(dive *d, int pos);
	void cylinderEdited(dive *d, int pos);

private:
	dive *d;
	int dcNr;
	bool inPlanner;
	int numRows; // Does not include unused cylinders at the end
	// Used if we temporarily change a line because the user is selecting a weight type
	int tempRow;
	cylinder_t tempCyl;

	cylinder_t *cylinderAt(const QModelIndex &index);
	void initTempCyl(int row);
	void clearTempCyl();
	void commitTempCyl(int row);
	bool cylinderUsed(int i) const;
	int calcNumRows() const;
	void updateNumRows();
};

#endif
