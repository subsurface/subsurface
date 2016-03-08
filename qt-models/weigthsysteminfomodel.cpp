#include "weigthsysteminfomodel.h"
#include "dive.h"
#include "metrics.h"
#include "gettextfromc.h"

WSInfoModel *WSInfoModel::instance()
{
	static QScopedPointer<WSInfoModel> self(new WSInfoModel());
	return self.data();
}

bool WSInfoModel::insertRows(int row, int count, const QModelIndex &parent)
{
	Q_UNUSED(row);
	beginInsertRows(parent, rowCount(), rowCount());
	rows += count;
	endInsertRows();
	return true;
}

bool WSInfoModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	//WARN: check for Qt::EditRole
	Q_UNUSED(role);
	struct ws_info_t *info = &ws_info[index.row()];
	switch (index.column()) {
	case DESCRIPTION:
		info->name = strdup(value.toByteArray().data());
		break;
	case GR:
		info->grams = value.toInt();
		break;
	}
	emit dataChanged(index, index);
	return true;
}

void WSInfoModel::clear()
{
}

QVariant WSInfoModel::data(const QModelIndex &index, int role) const
{
	QVariant ret;
	if (!index.isValid()) {
		return ret;
	}
	struct ws_info_t *info = &ws_info[index.row()];

	int gr = info->grams;
	switch (role) {
	case Qt::FontRole:
		ret = defaultModelFont();
		break;
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch (index.column()) {
		case GR:
			ret = gr;
			break;
		case DESCRIPTION:
			ret = gettextFromC::instance()->tr(info->name);
			break;
		}
		break;
	}
	return ret;
}

int WSInfoModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return rows + 1;
}

const QString &WSInfoModel::biggerString() const
{
	return biggerEntry;
}

WSInfoModel::WSInfoModel() : rows(-1)
{
	setHeaderDataStrings(QStringList() << tr("Description") << tr("kg"));
	struct ws_info_t *info = ws_info;
	for (info = ws_info; info->name; info++, rows++) {
		QString wsInfoName = gettextFromC::instance()->tr(info->name);
		if (wsInfoName.count() > biggerEntry.count())
			biggerEntry = wsInfoName;
	}

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
}

void WSInfoModel::updateInfo()
{
	struct ws_info_t *info = ws_info;
	beginRemoveRows(QModelIndex(), 0, this->rows);
	endRemoveRows();
	rows = -1;
	for (info = ws_info; info->name; info++, rows++) {
		QString wsInfoName = gettextFromC::instance()->tr(info->name);
		if (wsInfoName.count() > biggerEntry.count())
			biggerEntry = wsInfoName;
	}

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
}

void WSInfoModel::update()
{
	if (rows > -1) {
		beginRemoveRows(QModelIndex(), 0, rows);
		endRemoveRows();
		rows = -1;
	}
	struct ws_info_t *info = ws_info;
	for (info = ws_info; info->name; info++, rows++)
		;

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
}
