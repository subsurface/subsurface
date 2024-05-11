#ifndef DIVESITEIMPORTEDMODEL_H
#define DIVESITEIMPORTEDMODEL_H

#include <QAbstractTableModel>
#include <vector>
#include "core/divesite.h"

class DivesiteImportedModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	enum columnNames { NAME, LOCATION, COUNTRY, NEAREST, DISTANCE, SELECTED };

	DivesiteImportedModel(QObject *parent = 0);
	int columnCount(const QModelIndex& index = QModelIndex()) const;
	int rowCount(const QModelIndex& index = QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	void repopulate(dive_site_table *sites);
public
slots:
	void changeSelected(QModelIndex clickedIndex);
	void selectRow(int row);
	void selectAll();
	void selectNone();

private:
	int firstIndex;
	int lastIndex;
	std::vector<char> checkStates; // char instead of bool to avoid silly pessimization of std::vector.
	dive_site_table *importedSitesTable;
};

#endif
