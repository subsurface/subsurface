// SPDX-License-Identifier: GPL-2.0
#ifndef CYLINDERMODEL_H
#define CYLINDERMODEL_H

#include <QSortFilterProxyModel>

#include "cleanertablemodel.h"
#include "core/dive.h"

/* Encapsulation of the Cylinder Model, that presents the
 * Current cylinders that are used on a dive. */
class CylindersModel : public CleanerTableModel {
	Q_OBJECT
public:
	enum Column {
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
		COLUMNS
	};

	enum Roles {
		PASS_IN_ROLE = Qt::UserRole + 1 // For setting data: don't do any conversions
	};
	explicit CylindersModel(QObject *parent = 0);
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

	void add();
	void clear();
	void updateDive(dive *d);
	void updateDecoDepths(pressure_t olddecopo2);
	void updateTrashIcon();
	void moveAtFirst(int cylid);
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	bool updateBestMixes();
	bool cylinderUsed(int i) const;

public
slots:
	void remove(QModelIndex index);
	void cylindersReset(const QVector<dive *> &dives);
	void cylinderAdded(dive *d, int pos);
	void cylinderRemoved(dive *d, int pos);
	void cylinderEdited(dive *d, int pos);

private:
	dive *d;
	cylinder_t *cylinderAt(const QModelIndex &index);
};

// Cylinder model that hides unused cylinders if the pref.show_unused_cylinders flag is not set
class CylindersModelFiltered : public QSortFilterProxyModel {
	Q_OBJECT
public:
	CylindersModelFiltered(QObject *parent = 0);
	CylindersModel *model(); // Access to unfiltered base model

	void clear();
	void add();
	void updateDive(dive *d);
public
slots:
	void remove(QModelIndex index);
private:
	CylindersModel source;
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
};

#endif
