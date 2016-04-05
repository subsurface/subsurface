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
	/*reimp*/ QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex &parent = QModelIndex()) const;
	/*reimp*/ Qt::ItemFlags flags(const QModelIndex &index) const;
	/*reimp*/ bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

	void passInData(const QModelIndex &index, const QVariant &value);
	void add();
	void clear();
	void updateDive();
	weightsystem_t *weightSystemAt(const QModelIndex &index);
	bool changed;

public
slots:
	void remove(const QModelIndex &index);

private:
	int rows;
};

#endif
