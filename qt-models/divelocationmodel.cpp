#include "divelocationmodel.h"
#include "dive.h"

bool dive_site_less_then(dive_site *a, dive_site *b)
{
	return QString(a->name) <= QString(b->name);
}

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
		case DIVE_SITE_UUID  : return ds->uuid;
	}

	return QVariant();
}

void LocationInformationModel::update()
{
	if (rowCount()) {
		beginRemoveRows(QModelIndex(), 0, rowCount()-1);
		endRemoveRows();
	}
	if (dive_site_table.nr) {
		beginInsertRows(QModelIndex(), 0, dive_site_table.nr);
		internalRowCount = dive_site_table.nr;
		std::sort(dive_site_table.dive_sites, dive_site_table.dive_sites + dive_site_table.nr - 1, dive_site_less_then);
		endInsertRows();
	}
}
