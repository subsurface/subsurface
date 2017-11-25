// SPDX-License-Identifier: GPL-2.0
#ifndef FILTERMODELS_H
#define FILTERMODELS_H

#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <stdint.h>
#include <vector>

class MultiFilterInterface {
public:
	MultiFilterInterface() : anyChecked(false) {}
	virtual bool doFilter(struct dive *d, QModelIndex &index0, QAbstractItemModel *sourceModel) const = 0;
	virtual void clearFilter() = 0;
	std::vector<char> checkState;
	bool anyChecked;
};

class FilterModelBase : public QStringListModel, public MultiFilterInterface {
protected:
	explicit FilterModelBase(QObject *parent = 0);
	void updateList(const QStringList &new_list);
};

class TagFilterModel : public FilterModelBase {
	Q_OBJECT
public:
	static TagFilterModel *instance();
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	bool doFilter(struct dive *d, QModelIndex &index0, QAbstractItemModel *sourceModel) const;
	void clearFilter();
public
slots:
	void repopulate();

private:
	explicit TagFilterModel(QObject *parent = 0);
};

class BuddyFilterModel : public FilterModelBase {
	Q_OBJECT
public:
	static BuddyFilterModel *instance();
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	bool doFilter(struct dive *d, QModelIndex &index0, QAbstractItemModel *sourceModel) const;
	void clearFilter();
public
slots:
	void repopulate();

private:
	explicit BuddyFilterModel(QObject *parent = 0);
};

class LocationFilterModel : public FilterModelBase {
	Q_OBJECT
public:
	static LocationFilterModel *instance();
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	bool doFilter(struct dive *d, QModelIndex &index0, QAbstractItemModel *sourceModel) const;
	void clearFilter();
public
slots:
	void repopulate();

private:
	explicit LocationFilterModel(QObject *parent = 0);
};

class SuitsFilterModel : public FilterModelBase {
	Q_OBJECT
public:
	static SuitsFilterModel *instance();
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	bool doFilter(struct dive *d, QModelIndex &index0, QAbstractItemModel *sourceModel) const;
	void clearFilter();
public
slots:
	void repopulate();

private:
	explicit SuitsFilterModel(QObject *parent = 0);
};

class MultiFilterSortModel : public QSortFilterProxyModel {
	Q_OBJECT
public:
	static MultiFilterSortModel *instance();
	virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
	void addFilterModel(MultiFilterInterface *model);
	void removeFilterModel(MultiFilterInterface *model);
	int divesDisplayed;
public
slots:
	void myInvalidate();
	void clearFilter();
	void startFilterDiveSite(uint32_t uuid);
	void stopFilterDiveSite();

signals:
	void filterFinished();
private:
	MultiFilterSortModel(QObject *parent = 0);
	QList<MultiFilterInterface *> models;
	bool justCleared;
	struct dive_site *curr_dive_site;
};

#endif
