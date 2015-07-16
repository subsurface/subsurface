#include "divelocationmodel.h"
#include "dive.h"
#include <QDebug>
#include <QLineEdit>
#include <QIcon>

bool dive_site_less_than(dive_site *a, dive_site *b)
{
	return QString(a->name) <= QString(b->name);
}

LocationInformationModel *LocationInformationModel::instance()
{
	static LocationInformationModel *self = new LocationInformationModel();
	return self;
}

LocationInformationModel::LocationInformationModel(QObject *obj) : QAbstractTableModel(obj),
	internalRowCount(0),
	textField(NULL)
{
}

int LocationInformationModel::columnCount(const QModelIndex &parent) const
{
	return COLUMNS;
}

int LocationInformationModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return internalRowCount + 2;
}

static struct dive_site *get_dive_site_name_start_which_str(const QString& str) {
	struct dive_site *ds;
	int i;
	for_each_dive_site(i,ds) {
		QString dsName(ds->name);
		if (dsName.startsWith(str)) {
			return ds;
		}
	}
	return NULL;
}

QVariant LocationInformationModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	// Special case to handle the 'create dive site' with name.
	if (index.row() < 2) {
		if (index.column() == UUID)
			return 0;
		switch(role) {
			case Qt::DisplayRole : {
				if (index.row() == 1) {
					struct dive_site *ds = get_dive_site_name_start_which_str(textField->text());
					if(ds)
						return ds->name;
				}
				return textField->text();
			}
			case Qt::ToolTipRole : {
				return QString(tr("Create dive site with this name"));
			}
			case Qt::EditRole : {
				if (index.row() == 1) {
					struct dive_site *ds = get_dive_site_name_start_which_str(textField->text());
					if (!ds)
						return "NOT HERE";
					if (QString(ds->name) == textField->text())
						return "NOT HERE";
				}
				return textField->text();
			}
			case Qt::DecorationRole : return QIcon(":plus");
		}
	}

	// The dive sites are -2 because of the first two items.
	struct dive_site *ds = get_dive_site(index.row() - 2);

	if (!ds)
		return QVariant();

	switch(role) {
	case Qt::EditRole:
	case Qt::DisplayRole :
		switch(index.column()) {
		case UUID: return ds->uuid;
		case NAME: return ds->name;
		case LATITUDE: return ds->latitude.udeg;
		case LONGITUDE: return ds->longitude.udeg;
		case COORDS: return "TODO";
		case DESCRIPTION: return ds->description;
		case NOTES: return ds->name;
		case TAXONOMY_1: return "TODO";
		case TAXONOMY_2: return "TODO";
		case TAXONOMY_3: return "TODO";
		}
	break;
	case Qt::DecorationRole : {
		if (dive_site_has_gps_location(ds))
			return QIcon(":geocode");
		else
			return QVariant();
	}
	}

	return QVariant();
}

void LocationInformationModel::setFirstRowTextField(QLineEdit *t)
{
	textField = t;
}

void LocationInformationModel::update()
{
	beginResetModel();
	internalRowCount = dive_site_table.nr;
	qSort(dive_site_table.dive_sites, dive_site_table.dive_sites + dive_site_table.nr, dive_site_less_than);
	endResetModel();
}

int32_t LocationInformationModel::addDiveSite(const QString& name, int lon, int lat)
{
	degrees_t latitude, longitude;
	latitude.udeg = lat;
	longitude.udeg = lon;

	beginInsertRows(QModelIndex(), dive_site_table.nr + 2, dive_site_table.nr + 2);
	uint32_t uuid = create_dive_site_with_gps(name.toUtf8().data(), latitude, longitude);
	qSort(dive_site_table.dive_sites, dive_site_table.dive_sites + dive_site_table.nr, dive_site_less_than);
	internalRowCount = dive_site_table.nr;
	endInsertRows();
	return uuid;
}

bool LocationInformationModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid() || index.row() < 2)
		return false;

	if (role != Qt::EditRole)
		return false;

	struct dive_site *ds = get_dive_site(index.row());
	free(ds->name);
	ds->name = copy_string(qPrintable(value.toString()));
	emit dataChanged(index, index);
	return true;
}

bool LocationInformationModel::removeRows(int row, int count, const QModelIndex & parent)
{
	if(row >= rowCount())
		return false;

	beginRemoveRows(QModelIndex(), row + 2, row + 2);
	struct dive_site *ds = get_dive_site(row);
	if (ds)
		delete_dive_site(ds->uuid);
	internalRowCount = dive_site_table.nr;
	endRemoveRows();
	return true;
}

GeoReferencingOptionsModel *GeoReferencingOptionsModel::instance() {
	static GeoReferencingOptionsModel *self = new GeoReferencingOptionsModel();
	return self;
}

GeoReferencingOptionsModel::GeoReferencingOptionsModel(QObject *parent) : QStringListModel(parent)
{
	QStringList list;
	int i;
	for (i = 0; i < TC_NR_CATEGORIES; i++)
		list << taxonomy_category_names[i];
	setStringList(list);
}
