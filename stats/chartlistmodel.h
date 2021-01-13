// SPDX-License-Identifier: GPL-2.0
// A model to feed to the chart-selection combobox
#ifndef CHART_LIST_MODEL_H
#define CHART_LIST_MODEL_H

#include "statsstate.h"
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
	static const constexpr int IconRole = Qt::UserRole + 3;
	static const constexpr int IconSizeRole = Qt::UserRole + 4;

	Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

signals:
	void countChanged();

private:
	struct Item {
		bool isHeader;
		QString name;
		QString fullName;
		ChartSubType subtype;
		int id;
		bool warning;
	};

	struct SubTypeIcons {
		QPixmap normal;
		QPixmap warning;
	};
	QPixmap warningPixmap;
	SubTypeIcons subTypeIcons[(size_t)ChartSubType::Count];

	QFont itemFont;
	QFont headerFont;
	std::vector<Item> items;
	QHash<int, QByteArray> roleNames() const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	void initIcon(ChartSubType type, const char *name, int iconSize);
	const QPixmap &getIcon(ChartSubType type, bool warning) const;
};

#endif
