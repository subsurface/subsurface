// SPDX-License-Identifier: GPL-2.0
#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include <QCoreApplication>
#include <memory>

struct TreeItem {
	Q_DECLARE_TR_FUNCTIONS(TreeItemDT)

public:
	virtual ~TreeItem();
	TreeItem();
	virtual QVariant data(int column, int role) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;

	int row() const;
	QList<TreeItem *> children;
	TreeItem *parent;
};


class TreeModel : public QAbstractItemModel {
	Q_OBJECT
public:
	TreeModel(QObject *parent = 0);
	QVariant data(const QModelIndex &index, int role) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &child) const override;
	void clear();

protected:
	int columns;
	std::unique_ptr<TreeItem> rootItem;
};

#endif
