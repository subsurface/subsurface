// SPDX-License-Identifier: GPL-2.0
#include "cleanertablemodel.h"
#include "core/metrics.h"

const QPixmap &trashIcon()
{
	static QPixmap trash = QPixmap(":list-remove-icon").scaledToHeight(defaultIconMetrics().sz_small);
	return trash;
}

const QPixmap &trashForbiddenIcon()
{
	static QPixmap trash = QPixmap(":list-remove-disabled-icon").scaledToHeight(defaultIconMetrics().sz_small);
	return trash;
}

const QPixmap &editIcon()
{
	static QPixmap edit = QPixmap(":edit-icon").scaledToHeight(defaultIconMetrics().sz_small);
	return edit;
}

CleanerTableModel::CleanerTableModel(QObject *parent) : QAbstractTableModel(parent)
{
}

int CleanerTableModel::columnCount(const QModelIndex&) const
{
	return headers.count();
}

QVariant CleanerTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Vertical)
		return QVariant();

	switch (role) {
	case Qt::FontRole:
		return defaultModelFont();
	case Qt::DisplayRole:
		return headers.at(section);
	}
	return QVariant();
}

void CleanerTableModel::setHeaderDataStrings(const QStringList &newHeaders)
{
	headers = newHeaders;
}
