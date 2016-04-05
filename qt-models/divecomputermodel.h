#ifndef DIVECOMPUTERMODEL_H
#define DIVECOMPUTERMODEL_H

#include "qt-models/cleanertablemodel.h"
#include "core/divecomputer.h"

class DiveComputerModel : public CleanerTableModel {
	Q_OBJECT
public:
	enum {
		REMOVE,
		MODEL,
		ID,
		NICKNAME
	};
	DiveComputerModel(QMultiMap<QString, DiveComputerNode> &dcMap, QObject *parent = 0);
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	void update();
	void keepWorkingList();
	void dropWorkingList();

public
slots:
	void remove(const QModelIndex &index);

private:
	int numRows;
	QMultiMap<QString, DiveComputerNode> dcWorkingMap;
};

#endif
