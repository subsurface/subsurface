#include "divelocationmodel.h"
#include "dive.h"

LocationInformationModel *LocationInformationModel::instance()
{
	static LocationInformationModel *self = new LocationInformationModel();
	return self;
}

LocationInformationModel::LocationInformationModel(QObject *obj) : QAbstractListModel(obj), internalRowCount(0)
{
}

int LocationInformationModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return internalRowCount;
}

QVariant LocationInformationModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	struct dive_site *ds = get_dive_site(index.row());

	switch(role) {
		case Qt::DisplayRole : return qPrintable(ds->name);
	}

	return QVariant();
}

void LocationInformationModel::update()
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds);

	if (rowCount()) {
		beginRemoveRows(QModelIndex(), 0, rowCount()-1);
		endRemoveRows();
	}
	if (i) {
		beginInsertRows(QModelIndex(), 0, i);
		internalRowCount = i;
		endInsertRows();
	}
}
