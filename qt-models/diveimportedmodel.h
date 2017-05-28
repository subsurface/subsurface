#ifndef DIVEIMPORTEDMODEL_H
#define DIVEIMPORTEDMODEL_H

#include <QAbstractTableModel>
#include "core/dive.h"

class DiveImportedModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	enum roleTypes { DateTime = Qt::UserRole + 1, Duration, Depth};

	DiveImportedModel(QObject *parent = 0);
	void setDiveTable(struct dive_table *table);
	int columnCount(const QModelIndex& index = QModelIndex()) const;
	int rowCount(const QModelIndex& index = QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	void setImportedDivesIndexes(int first, int last);
	Qt::ItemFlags flags(const QModelIndex &index) const;
	void clearTable();
	QHash<int, QByteArray> roleNames() const;
	Q_INVOKABLE void repopulate();
	Q_INVOKABLE void recordDives();
public
slots:
	void changeSelected(QModelIndex clickedIndex);
	void selectAll();
	void selectNone();

private:
	int firstIndex;
	int lastIndex;
	bool *checkStates;
	struct dive_table *diveTable;
};

#endif
