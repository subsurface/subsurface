#include "qt-models/treemodel.h"
#include "core/metrics.h"

TreeItem::TreeItem()
{
	parent = NULL;
}

TreeItem::~TreeItem()
{
	qDeleteAll(children);
}

Qt::ItemFlags TreeItem::flags(const QModelIndex &index) const
{
	Q_UNUSED(index);
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int TreeItem::row() const
{
	if (parent)
		return parent->children.indexOf(const_cast<TreeItem *>(this));
	return 0;
}

QVariant TreeItem::data(int column, int role) const
{
	Q_UNUSED(column);
	Q_UNUSED(role);
	return QVariant();
}

TreeModel::TreeModel(QObject *parent) : QAbstractItemModel(parent)
{
	columns = 0; // I'm not sure about this one - I can't see where it gets initialized
	rootItem = new TreeItem();
}

TreeModel::~TreeModel()
{
	delete rootItem;
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
	QVariant val = item->data(index.column(), role);

	if (role == Qt::FontRole && !val.isValid())
		return defaultModelFont();
	else
		return val;
}

bool TreeItem::setData(const QModelIndex &index, const QVariant &value, int role)
{
	Q_UNUSED(index);
	Q_UNUSED(value);
	Q_UNUSED(role);
	return false;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	TreeItem *parentItem = (!parent.isValid()) ? rootItem : static_cast<TreeItem *>(parent.internalPointer());

	TreeItem *childItem = parentItem->children[row];

	return (childItem) ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	TreeItem *childItem = static_cast<TreeItem *>(index.internalPointer());
	TreeItem *parentItem = childItem->parent;

	if (parentItem == rootItem || !parentItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
	TreeItem *parentItem;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<TreeItem *>(parent.internalPointer());

	int amount = parentItem->children.count();
	return amount;
}

int TreeModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return columns;
}
