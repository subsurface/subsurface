/*
 * models.h
 *
 * header file for the equipment models of Subsurface
 *
 */
#ifndef MODELS_H
#define MODELS_H

#include <QAbstractTableModel>
#include <QCoreApplication>
#include <QStringList>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <QPixmap>

#include "metrics.h"

#include "../dive.h"
#include "../divelist.h"
#include "../divecomputer.h"
#include "cleanertablemodel.h"
#include "treemodel.h"

/* ProfilePrintModel:
 * this model is used when printing a data table under a profile. it requires
 * some exact usage of setSpan(..) on the target QTableView widget.
 */
class ProfilePrintModel : public QAbstractTableModel {
	Q_OBJECT

private:
	int diveId;
	double fontSize;

public:
	ProfilePrintModel(QObject *parent = 0);
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	void setDive(struct dive *divePtr);
	void setFontsize(double size);
};

class GasSelectionModel : public QStringListModel {
	Q_OBJECT
public:
	static GasSelectionModel *instance();
	Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
public
slots:
	void repopulate();
};


class LanguageModel : public QAbstractListModel {
	Q_OBJECT
public:
	static LanguageModel *instance();
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

private:
	LanguageModel(QObject *parent = 0);

	QStringList languages;
};

#endif // MODELS_H
