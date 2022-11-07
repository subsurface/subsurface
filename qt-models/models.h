// SPDX-License-Identifier: GPL-2.0
/*
 * models.h
 *
 * header file for the equipment models of Subsurface
 *
 */
#ifndef MODELS_H
#define MODELS_H

#include <QAbstractTableModel>
#include <QStringList>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <QPixmap>

#include "core/metrics.h"

#include "cleanertablemodel.h"
#include "treemodel.h"

class GasSelectionModel : public QStringListModel {
	Q_OBJECT
public:
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant data(const QModelIndex &index, int role) const override;
public
slots:
	void repopulate();
};

class DiveTypeSelectionModel : public QStringListModel {
	Q_OBJECT
public:
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant data(const QModelIndex &index, int role) const override;
public
slots:
	void repopulate();
};

class LanguageModel : public QAbstractListModel {
	Q_OBJECT
public:
	static LanguageModel *instance();
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;

private:
	LanguageModel(QObject *parent = 0);

	QStringList languages;
};

#endif // MODELS_H
