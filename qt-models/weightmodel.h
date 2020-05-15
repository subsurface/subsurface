// SPDX-License-Identifier: GPL-2.0
#ifndef WEIGHTMODEL_H
#define WEIGHTMODEL_H

#include "cleanertablemodel.h"
#include "core/equipment.h"

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
	void setTempWS(int row, weightsystem_t ws);
	void clearTempWS();
	void commitTempWS();

	void clear();
	void updateDive(dive *d);
	weightsystem_t weightSystemAt(const QModelIndex &index) const;

signals:
	void divesEdited(int num);

public
slots:
	void weightsystemsReset(const QVector<dive *> &dives);
	void weightAdded(dive *d, int pos);
	void weightRemoved(dive *d, int pos);
	void weightEdited(dive *d, int pos);

private:
	dive *d;
	// If we temporarily change a line because the user is selecting a weight type
	int tempRow;
	weightsystem_t tempWS;
};

#endif
