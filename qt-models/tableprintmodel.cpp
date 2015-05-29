#include "tableprintmodel.h"
#include "metrics.h"
#include "color.h"

TablePrintModel::TablePrintModel()
{
	columns = 7;
	rows = 0;
}

TablePrintModel::~TablePrintModel()
{
	for (int i = 0; i < list.size(); i++)
		delete list.at(i);
}

void TablePrintModel::insertRow(int index)
{
	struct TablePrintItem *item = new struct TablePrintItem();
	item->colorBackground = 0xffffffff;
	if (index == -1) {
		beginInsertRows(QModelIndex(), rows, rows);
		list.append(item);
	} else {
		beginInsertRows(QModelIndex(), index, index);
		list.insert(index, item);
	}
	endInsertRows();
	rows++;
}

void TablePrintModel::callReset()
{
	beginResetModel();
	endResetModel();
}

QVariant TablePrintModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (role == Qt::BackgroundRole)
		return QColor(list.at(index.row())->colorBackground);
	if (role == Qt::DisplayRole)
		switch (index.column()) {
		case 0:
			return list.at(index.row())->number;
		case 1:
			return list.at(index.row())->date;
		case 2:
			return list.at(index.row())->depth;
		case 3:
			return list.at(index.row())->duration;
		case 4:
			return list.at(index.row())->divemaster;
		case 5:
			return list.at(index.row())->buddy;
		case 6:
			return list.at(index.row())->location;
		}
	if (role == Qt::FontRole) {
		QFont font;
		font.setPointSizeF(7.5);
		if (index.row() == 0 && index.column() == 0) {
			font.setBold(true);
		}
		return QVariant::fromValue(font);
	}
	return QVariant();
}

bool TablePrintModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (index.isValid()) {
		if (role == Qt::DisplayRole) {
			switch (index.column()) {
			case 0:
				list.at(index.row())->number = value.toString();
			case 1:
				list.at(index.row())->date = value.toString();
			case 2:
				list.at(index.row())->depth = value.toString();
			case 3:
				list.at(index.row())->duration = value.toString();
			case 4:
				list.at(index.row())->divemaster = value.toString();
			case 5:
				list.at(index.row())->buddy = value.toString();
			case 6: {
				/* truncate if there are more than N lines of text,
				 * we don't want a row to be larger that a single page! */
				QString s = value.toString();
				const int maxLines = 15;
				int count = 0;
				for (int i = 0; i < s.length(); i++) {
					if (s.at(i) != QChar('\n'))
						continue;
					count++;
					if (count > maxLines) {
						s = s.left(i - 1);
						break;
					}
				}
				list.at(index.row())->location = s;
			}
			}
			return true;
		}
		if (role == Qt::BackgroundRole) {
			list.at(index.row())->colorBackground = value.value<unsigned int>();
			return true;
		}
	}
	return false;
}

int TablePrintModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return rows;
}

int TablePrintModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return columns;
}
