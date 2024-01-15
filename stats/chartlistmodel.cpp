// SPDX-License-Identifier: GPL-2.0
#include "chartlistmodel.h"
#include "core/metrics.h"
#include <QIcon>
#include <QFontMetrics>
#include <QPainter>

ChartListModel::ChartListModel() :
	itemFont(defaultModelFont()),
	headerFont(itemFont.family(), itemFont.pointSize(), itemFont.weight(), true)
{
	QFontMetrics fm(itemFont);
	int fontHeight = fm.height();

	int iconSize = fontHeight * 2;
	warningPixmap = QPixmap(":chart-warning-icon").scaled(fontHeight, fontHeight, Qt::KeepAspectRatio);
	initIcon(ChartSubType::Vertical, ":chart-bar-vertical-icon", iconSize);
	initIcon(ChartSubType::VerticalGrouped, ":chart-bar-grouped-vertical-icon", iconSize);
	initIcon(ChartSubType::VerticalStacked, ":chart-bar-stacked-vertical-icon", iconSize);
	initIcon(ChartSubType::Horizontal, ":chart-bar-horizontal-icon", iconSize);
	initIcon(ChartSubType::HorizontalGrouped, ":chart-bar-grouped-horizontal-icon", iconSize);
	initIcon(ChartSubType::HorizontalStacked, ":chart-bar-stacked-horizontal-icon", iconSize);
	initIcon(ChartSubType::Dots, ":chart-points-icon", iconSize);
	initIcon(ChartSubType::Box, ":chart-box-icon", iconSize);
	initIcon(ChartSubType::Pie, ":chart-pie-icon", iconSize);
}

ChartListModel::~ChartListModel()
{
}

void ChartListModel::initIcon(ChartSubType type, const char *name, int iconSize)
{
	QPixmap icon = QPixmap(name).scaled(iconSize, iconSize, Qt::KeepAspectRatio);
	QPixmap iconWarning = icon.copy();
	QPainter painter(&iconWarning);
	painter.drawPixmap(0, 0, warningPixmap);
	subTypeIcons[(size_t)type].normal = std::move(icon);
	subTypeIcons[(size_t)type].warning = std::move(iconWarning);
}

const QPixmap &ChartListModel::getIcon(ChartSubType type, bool warning) const
{
	int idx = std::clamp((int)type, 0, (int)ChartSubType::Count);
	return warning ? subTypeIcons[idx].warning : subTypeIcons[idx].normal;
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

QHash<int, QByteArray> ChartListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[Qt::DisplayRole] = "display";
	roles[IconRole] = "icon";
	roles[IconSizeRole] = "iconSize";
	roles[ChartNameRole] = "chartName";
	roles[IsHeaderRole] = "isHeader";
	roles[Qt::UserRole] = "id";
	return roles;
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
		return items[row].warning ? QVariant::fromValue(QIcon(warningPixmap))
					  : QVariant();
	case IconRole:
		return items[row].isHeader ? QVariant()
					   : QVariant::fromValue(getIcon(items[row].subtype, items[row].warning));
	case IconSizeRole:
		return items[row].isHeader ? QVariant()
					   : QVariant::fromValue(getIcon(items[row].subtype, items[row].warning).size());
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
			items.push_back({ true, chart.name, QString(), (ChartSubType)-1, -1, false });
			act = chart.name;
		}
		if (charts.selected == chart.id)
			res = (int)items.size();
		QString fullName = QString("%1 / %2").arg(chart.name, chart.subtypeName);
		items.push_back({ false, chart.subtypeName, fullName, chart.subtype, chart.id, chart.warning });
	}
	endResetModel();
	emit countChanged();
	return res;
}
