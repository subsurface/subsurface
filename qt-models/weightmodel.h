// SPDX-License-Identifier: GPL-2.0
#ifndef WEIGHTMODEL_H
#define WEIGHTMODEL_H

#include "cleanertablemodel.h"
#include "core/dive.h"

/* Encapsulation of the Weight Model, that represents
 * the current weights on a dive. */
class WeightModel : public CleanerTableModel {
	Q_OBJECT
public:
	enum Column {
		REMOVE,
		TYPE,
		WEIGHT
	};

	explicit WeightModel(QObject *parent = 0);
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

	void passInData(const QModelIndex &index, const QVariant &value);
	void add();
	void clear();
	void updateDive();
	weightsystem_t *weightSystemAt(const QModelIndex &index);
	bool changed;

public
slots:
	void remove(const QModelIndex &index);
	void weightsystemsReset(const QVector<dive *> &dives);

private:
	int rows;
};

#endif
