// SPDX-License-Identifier: GPL-2.0
#include "chartlistmodel.h"
#include "core/metrics.h"
#include <QIcon>
#include <QFontMetrics>

ChartListModel::ChartListModel() :
	itemFont(defaultModelFont()), 
	headerFont(itemFont.family(), itemFont.pointSize(), itemFont.weight(), true)
{
	QFontMetrics fm(itemFont);
	int fontHeight = fm.height();
	warningIcon = QIcon::fromTheme("dialog-warning");
	warningPixmap = warningIcon.pixmap(QSize(fontHeight, fontHeight));
}

ChartListModel::~ChartListModel()
{
}

int ChartListModel::rowCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : (int)items.size();
}

Qt::ItemFlags ChartListModel::flags(const QModelIndex &index) const
{
	int row = index.row();
	if (index.parent().isValid() || row < 0 || row >= (int)items.size())
		return Qt::NoItemFlags;
	return items[row].isHeader ? Qt::ItemIsEnabled
				   : Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant ChartListModel::data(const QModelIndex &index, int role) const
{
	int row = index.row();
	if (index.parent().isValid() || row < 0 || row >= (int)items.size())
		return QVariant();

	switch (role) {
	case Qt::FontRole:
		return items[row].isHeader ? headerFont : itemFont;
	case Qt::DisplayRole:
		return items[row].fullName;
	case Qt::DecorationRole:
		return items[row].warning ? QVariant::fromValue(warningIcon)
					  : QVariant();
	case PixmapRole:
		return items[row].warning ? QVariant::fromValue(warningPixmap)
					  : QVariant();
	case ChartNameRole:
		return items[row].name;
	case IsHeaderRole:
		return items[row].isHeader;
	case Qt::UserRole:
		return items[row].id;
	}
	return QVariant();
}

int ChartListModel::update(const StatsState::ChartList &charts)
{
	// Sort non-recommended entries to the back
	std::vector<StatsState::Chart> sorted;
	sorted.reserve(charts.charts.size());
	std::copy_if(charts.charts.begin(), charts.charts.end(), std::back_inserter(sorted),
		     [] (const StatsState::Chart &chart) { return !chart.warning; });
	std::copy_if(charts.charts.begin(), charts.charts.end(), std::back_inserter(sorted),
		     [] (const StatsState::Chart &chart) { return chart.warning; });

	beginResetModel();
	items.clear();
	QString act;
	int res = -1;
	for (const StatsState::Chart &chart: sorted) {
		if (act != chart.name) {
			items.push_back({ true, chart.name, QString(), -1, false });
			act = chart.name;
		}
		if (charts.selected == chart.id)
			res = (int)items.size();
		QString fullName = QString("%1 / %2").arg(chart.name, chart.subtypeName);
		items.push_back({ false, chart.subtypeName, fullName, chart.id, chart.warning });
	}
	endResetModel();
	return res;
}
