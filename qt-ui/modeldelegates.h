#ifndef MODELDELEGATES_H
#define MODELDELEGATES_H

#include <QStyledItemDelegate>

class StarWidgetsDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit StarWidgetsDelegate(QWidget* parent = 0);
	virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
private:
	QWidget *parentWidget;
};

class ComboBoxDelegate : public QStyledItemDelegate{
	Q_OBJECT
public:
	explicit ComboBoxDelegate(QAbstractItemModel *model, QObject* parent = 0);
	virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	virtual void setEditorData(QWidget* editor, const QModelIndex& index) const;
protected:
	QAbstractItemModel *model;
};

class TankInfoDelegate : public ComboBoxDelegate{
	Q_OBJECT
public:
	explicit TankInfoDelegate(QObject* parent = 0);
	virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;
};

class WSInfoDelegate : public ComboBoxDelegate{
	Q_OBJECT
public:
	explicit WSInfoDelegate(QObject* parent = 0);
	virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;
};

#endif
