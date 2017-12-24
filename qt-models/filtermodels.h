// SPDX-License-Identifier: GPL-2.0
#ifndef FILTERMODELS_H
#define FILTERMODELS_H

#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <stdint.h>
#include <vector>

class FilterModelBase : public QStringListModel {
	Q_OBJECT
public:
	virtual bool doFilter(struct dive *d, QModelIndex &index0, QAbstractItemModel *sourceModel) const = 0;
	void clearFilter();
	void selectAll();
	void invertSelection();
	std::vector<char> checkState;
	bool anyChecked;
	bool negate;
public
slots:
	void setNegate(bool negate);
protected:
	explicit FilterModelBase(QObject *parent = 0);
	void updateList(const QStringList &new_list);
	virtual int countDives(const char *) const = 0;
private:
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
};

class TagFilterModel : public FilterModelBase {
	Q_OBJECT
public:
	static TagFilterModel *instance();
	bool doFilter(struct dive *d, QModelIndex &index0, QAbstractItemModel *sourceModel) const;
public
slots:
	void repopulate();

private:
	explicit TagFilterModel(QObject *parent = 0);
	int countDives(const char *) const;
};

class BuddyFilterModel : public FilterModelBase {
	Q_OBJECT
public:
	static BuddyFilterModel *instance();
	bool doFilter(struct dive *d, QModelIndex &index0, QAbstractItemModel *sourceModel) const;
public
slots:
	void repopulate();

private:
	explicit BuddyFilterModel(QObject *parent = 0);
	int countDives(const char *) const;
};

class LocationFilterModel : public FilterModelBase {
	Q_OBJECT
public:
	static LocationFilterModel *instance();
	bool doFilter(struct dive *d, QModelIndex &index0, QAbstractItemModel *sourceModel) const;
public
slots:
	void repopulate();
	void changeName(const QString &oldName, const QString &newName);
	void addName(const QString &newName);

private:
	explicit LocationFilterModel(QObject *parent = 0);
	int countDives(const char *) const;
};

class SuitsFilterModel : public FilterModelBase {
	Q_OBJECT
public:
	static SuitsFilterModel *instance();
	bool doFilter(struct dive *d, QModelIndex &index0, QAbstractItemModel *sourceModel) const;
public
slots:
	void repopulate();

private:
	explicit SuitsFilterModel(QObject *parent = 0);
	int countDives(const char *) const;
};

class MultiFilterSortModel : public QSortFilterProxyModel {
	Q_OBJECT
public:
	static MultiFilterSortModel *instance();
	virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
	void addFilterModel(FilterModelBase *model);
	void removeFilterModel(FilterModelBase *model);
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
	QList<FilterModelBase *> models;
	bool justCleared;
	struct dive_site *curr_dive_site;
};

#endif
