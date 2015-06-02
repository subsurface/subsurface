#include "divelocationmodel.h"
#include "dive.h"

bool dive_site_less_than(dive_site *a, dive_site *b)
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
		std::sort(dive_site_table.dive_sites, dive_site_table.dive_sites + dive_site_table.nr, dive_site_less_than);
		endInsertRows();
	}
}

int32_t LocationInformationModel::addDiveSite(const QString& name, int lon, int lat)
{
	degrees_t latitude, longitude;
	latitude.udeg = lat;
	longitude.udeg = lon;

	int32_t uuid = create_dive_site_with_gps(name.toUtf8().data(), latitude, longitude);
	update();
	return uuid;
}

bool LocationInformationModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid())
		return false;

	if (role != Qt::EditRole)
		return false;

	struct dive_site *ds = get_dive_site(index.row());
	free(ds->name);
	ds->name = copy_string(qPrintable(value.toString()));
	emit dataChanged(index, index);
	return true;
}
