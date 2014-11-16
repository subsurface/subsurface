#ifndef FILTERMODELS_H
#define FILTERMODELS_H

#include <QStringListModel>
#include <QSortFilterProxyModel>

class MultiFilterInterface {
public:
	MultiFilterInterface() : checkState(NULL){};
	virtual bool doFilter(struct dive *d, QModelIndex &index0, QAbstractItemModel *sourceModel) const = 0;
	virtual void clearFilter() = 0;
	bool *checkState;
	bool anyChecked;
};

class TagFilterModel : public QStringListModel, public MultiFilterInterface {
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

class BuddyFilterModel : public QStringListModel, public MultiFilterInterface {
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

class LocationFilterModel : public QStringListModel, public MultiFilterInterface {
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

class SuitsFilterModel : public QStringListModel, public MultiFilterInterface {
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
signals:
	void filterFinished();
private:
	MultiFilterSortModel(QObject *parent = 0);
	QList<MultiFilterInterface *> models;
	bool justCleared;
};

#endif
