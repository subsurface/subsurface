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

struct dive;
struct divecomputer;

class GasSelectionModel : public QAbstractListModel {
	Q_OBJECT
public:
	GasSelectionModel(const dive &d, int dcNr, QObject *parent);
	QVariant data(const QModelIndex &index, int role) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
private:
	std::vector<std::pair<int, QString>> gasNames;
};

class DiveTypeSelectionModel : public QAbstractListModel {
	Q_OBJECT
public:
	DiveTypeSelectionModel(const dive &d, int dcNr, QObject *parent);
	QVariant data(const QModelIndex &index, int role) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
private:
	std::vector<std::pair<int, QString>> diveTypes;
};

class SensorSelectionModel : public QAbstractListModel {
	Q_OBJECT
public:
	SensorSelectionModel(const divecomputer &dc, QObject *parent);
	QVariant data(const QModelIndex &index, int role) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
private:
	std::vector<std::pair<int, QString>> sensorNames;
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
