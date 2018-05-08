#ifndef REGULATORMODEL_H
#define REGULATORMODEL_H

#include "cleanertablemodel.h"
#include "core/dive.h"

class RegulatorModel : public CleanerTableModel {
	Q_OBJECT
public:
	enum Column {
		REMOVE,
		TYPE,
		SERVICE_INTERVAL_TIME,
		SERVICE_INTERVAL_DIVES,
		LAST_SERVICE,
		NEXT_SERVICE,
		REMARKS
	};
	
	explicit RegulatorModel(QObject *parent = 0);
	/*reimp*/ QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex &parent = QModelIndex()) const;
	/*reimp*/ Qt::ItemFlags flags(const QModelIndex &index) const;
	/*reimp*/ bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	
	void passInData(const QModelIndex &index, const QVariant &value);
	void add();
	void clear();
	void updateDive();
	regulator_t *regulatorAt(const QModelIndex &index);
	bool changed;
	
	public
	slots:
	void remove(const QModelIndex &index);
	
private:
	int rows;
};

#endif // REGULATORMODEL_H
