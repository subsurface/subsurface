// SPDX-License-Identifier: GPL-2.0
#ifndef FILTERMODELS_H
#define FILTERMODELS_H

#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <stdint.h>
#include <vector>

struct dive;
struct dive_trip;

class FilterModelBase : public QAbstractListModel {
	Q_OBJECT
private:
	int findInsertionIndex(const QString &name);
protected:
	struct Item {
		QString name;
		bool checked;
		int count;
	};
	std::vector<Item> items;
	int indexOf(const QString &name) const;
	void addItem(const QString &name, bool checked, int count);
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	void decreaseCount(const QString &d);
	void increaseCount(const QString &d);
public:
	virtual bool doFilter(const dive *d) const = 0;
	virtual void diveAdded(const dive *d) = 0;
	virtual void diveDeleted(const dive *d) = 0;
	void clearFilter();
	void selectAll();
	void invertSelection();
	bool anyChecked;
	bool negate;
public
slots:
	void setNegate(bool negate);
	void changeName(const QString &oldName, const QString &newName);
protected:
	explicit FilterModelBase(QObject *parent = 0);
	void updateList(QStringList &new_list);
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
	bool doFilter(const dive *d) const;
public
slots:
	void repopulate();

private:
	explicit TagFilterModel(QObject *parent = 0);
	int countDives(const char *) const;
	void diveAdded(const dive *d);
	void diveDeleted(const dive *d);
};

class BuddyFilterModel : public FilterModelBase {
	Q_OBJECT
public:
	static BuddyFilterModel *instance();
	bool doFilter(const dive *d) const;
public
slots:
	void repopulate();

private:
	explicit BuddyFilterModel(QObject *parent = 0);
	int countDives(const char *) const;
	void diveAdded(const dive *d);
	void diveDeleted(const dive *d);
};

class LocationFilterModel : public FilterModelBase {
	Q_OBJECT
public:
	static LocationFilterModel *instance();
	bool doFilter(const dive *d) const;
public
slots:
	void repopulate();
	void addName(const QString &newName);

private:
	explicit LocationFilterModel(QObject *parent = 0);
	int countDives(const char *) const;
	void diveAdded(const dive *d);
	void diveDeleted(const dive *d);
};

class SuitsFilterModel : public FilterModelBase {
	Q_OBJECT
public:
	static SuitsFilterModel *instance();
	bool doFilter(const dive *d) const;
public
slots:
	void repopulate();

private:
	explicit SuitsFilterModel(QObject *parent = 0);
	int countDives(const char *) const;
	void diveAdded(const dive *d);
	void diveDeleted(const dive *d);
};

class MultiFilterSortModel : public QSortFilterProxyModel {
	Q_OBJECT
public:
	static MultiFilterSortModel *instance();
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
	void addFilterModel(FilterModelBase *model);
	void removeFilterModel(FilterModelBase *model);
	void divesAdded(const QVector<dive *> &dives);
	void divesDeleted(const QVector<dive *> &dives);
	bool showDive(const struct dive *d) const;
	int divesDisplayed;
public
slots:
	void myInvalidate();
	void clearFilter();
	void startFilterDiveSite(struct dive_site *ds);
	void stopFilterDiveSite();
	void filterChanged(const QModelIndex &from, const QModelIndex &to, const QVector<int> &roles);

signals:
	void filterFinished();
private:
	MultiFilterSortModel(QObject *parent = 0);
	QList<FilterModelBase *> models;
	struct dive_site *curr_dive_site;
};

#endif
