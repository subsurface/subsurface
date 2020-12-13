// SPDX-License-Identifier: GPL-2.0
// A model to feed to the chart-selection combobox
#ifndef CHART_LIST_MODEL_H
#define CHART_LIST_MODEL_H

#include "statsstate.h" // Argh. Can't forward declare StatsState::ChartList. :-/
#include <vector>
#include <QAbstractListModel>
#include <QString>
#include <QFont>
#include <QIcon>
#include <QPixmap>

class ChartListModel : public QAbstractListModel {
	Q_OBJECT
public:
	ChartListModel();
	~ChartListModel();

	// Returns index of selected item
	int update(const StatsState::ChartList &charts);

	static const constexpr int ChartNameRole = Qt::UserRole + 1;
	static const constexpr int IsHeaderRole = Qt::UserRole + 2;
	static const constexpr int PixmapRole = Qt::UserRole + 3;
private:
	struct Item {
		bool isHeader;
		QString name;
		QString fullName;
		int id;
		bool warning;
	};
	QFont itemFont;
	QFont headerFont;
	QIcon warningIcon;
	QPixmap warningPixmap;
	std::vector<Item> items;
	int rowCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
};

#endif
