// SPDX-License-Identifier: GPL-2.0
#include "ssrfsortfilterproxymodel.h"

SsrfSortFilterProxyModel::SsrfSortFilterProxyModel(QObject *parent)
: QSortFilterProxyModel(parent), less_than(0), accepts_col(0), accepts_row(0)
{
}

bool SsrfSortFilterProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
{
	Q_ASSERT(less_than);
	const QAbstractItemModel *self = this;
	return less_than(const_cast<QAbstractItemModel*>(self), source_left, source_right);
}

bool SsrfSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
	if (!accepts_row)
		return true;
	const QAbstractItemModel *self = this;
	return accepts_row(const_cast<QAbstractItemModel*>(self), source_row, source_parent);
}

bool SsrfSortFilterProxyModel::filterAcceptsColumn(int source_column, const QModelIndex& source_parent) const
{
	if (!accepts_col)
		return true;
	const QAbstractItemModel *self = this;
	return accepts_col(const_cast<QAbstractItemModel*>(self), source_column, source_parent);
}

void SsrfSortFilterProxyModel::setLessThan(less_than_cb func)
{
	less_than = func;
}

void SsrfSortFilterProxyModel::setFilterRow(filter_accepts_row_cb func)
{
	accepts_row = func;
}

void SsrfSortFilterProxyModel::setFilterCol(filter_accepts_col_cb func)
{
	accepts_col = func;
}
