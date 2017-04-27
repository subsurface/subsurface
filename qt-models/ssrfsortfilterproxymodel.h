// SPDX-License-Identifier: GPL-2.0
#ifndef SSRFSORTFILTERPROXYMODEL_H
#define SSRFSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

typedef bool (*filter_accepts_col_cb) (QAbstractItemModel *model,int sourceRow, const QModelIndex& parent);
typedef bool (*filter_accepts_row_cb) (QAbstractItemModel *model,int sourceRow, const QModelIndex& parent);
typedef bool (*less_than_cb) (QAbstractItemModel *model, const QModelIndex& left, const QModelIndex& right);

/* Use this class when you wanna a quick filter.
 * instead of creating a new class, just create a new instance of this class
 * and plug your callback.
 */
class SsrfSortFilterProxyModel : public QSortFilterProxyModel {
	Q_OBJECT

public:
	SsrfSortFilterProxyModel(QObject *parent = 0);
	virtual bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const;
	virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;
	virtual bool filterAcceptsColumn(int source_column, const QModelIndex& source_parent) const;

	void setLessThan(less_than_cb func);
	void setFilterRow(filter_accepts_row_cb func);
	void setFilterCol(filter_accepts_col_cb func);

private:
	less_than_cb less_than;
	filter_accepts_col_cb accepts_col;
	filter_accepts_row_cb accepts_row;
};

#endif
