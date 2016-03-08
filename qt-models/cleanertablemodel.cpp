#include "cleanertablemodel.h"
#include "metrics.h"

CleanerTableModel::CleanerTableModel(QObject *parent) : QAbstractTableModel(parent)
{
}

int CleanerTableModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return headers.count();
}

QVariant CleanerTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;

	if (orientation == Qt::Vertical)
		return ret;

	switch (role) {
	case Qt::FontRole:
		ret = defaultModelFont();
		break;
	case Qt::DisplayRole:
		ret = headers.at(section);
	}
	return ret;
}

void CleanerTableModel::setHeaderDataStrings(const QStringList &newHeaders)
{
	headers = newHeaders;
}
