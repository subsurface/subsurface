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

#include "core/dive.h"
#include "core/divelist.h"
#include "core/divecomputer.h"
#include "cleanertablemodel.h"
#include "treemodel.h"

class GasSelectionModel : public QStringListModel {
	Q_OBJECT
public:
	static GasSelectionModel *instance();
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant data(const QModelIndex &index, int role) const;
public
slots:
	void repopulate();
};

class DiveTypeSelectionModel : public QStringListModel {
	Q_OBJECT
public:
	static DiveTypeSelectionModel *instance();
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant data(const QModelIndex &index, int role) const;
public
slots:
	void repopulate();
};


class LanguageModel : public QAbstractListModel {
	Q_OBJECT
public:
	static LanguageModel *instance();
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;

private:
	LanguageModel(QObject *parent = 0);

	QStringList languages;
};

#endif // MODELS_H
