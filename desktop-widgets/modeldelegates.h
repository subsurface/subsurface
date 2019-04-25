// SPDX-License-Identifier: GPL-2.0
#ifndef MODELDELEGATES_H
#define MODELDELEGATES_H

#include "core/units.h"

#include <QStyledItemDelegate>
#include <QComboBox>
class QPainter;

class DiveListDelegate : public QStyledItemDelegate {
public:
	explicit DiveListDelegate(QObject *parent = 0)
	    : QStyledItemDelegate(parent)
	{
	}
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class StarWidgetsDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit StarWidgetsDelegate(QWidget *parent = 0);
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	const QSize& starSize() const;

private:
	QWidget *parentWidget;
	QSize minStarSize;
};

class ComboBoxDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit ComboBoxDelegate(QAbstractItemModel *model, QObject *parent = 0, bool allowEdit = true);
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	bool eventFilter(QObject *object, QEvent *event) override;
public
slots:
	void testActivation(const QString &currString = QString());
	void testActivation(const QModelIndex &currIndex);
	//HACK: try to get rid of this in the future.
	void fakeActivation();
	void fixTabBehavior();
	virtual void revertModelData(QWidget *widget, QAbstractItemDelegate::EndEditHint hint) = 0;
private:
	bool editable;
protected:
	QAbstractItemModel *model;
};

class TankInfoDelegate : public ComboBoxDelegate {
	Q_OBJECT
public:
	explicit TankInfoDelegate(QObject *parent = 0);
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
public
slots:
	void revertModelData(QWidget *widget, QAbstractItemDelegate::EndEditHint hint);
	void reenableReplot(QWidget *widget, QAbstractItemDelegate::EndEditHint hint);
};

class TankUseDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit TankUseDelegate(QObject *parent = 0);
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget * editor, const QModelIndex & index) const override;
};

class WSInfoDelegate : public ComboBoxDelegate {
	Q_OBJECT
public:
	explicit WSInfoDelegate(QObject *parent = 0);
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
public
slots:
	void revertModelData(QWidget *widget, QAbstractItemDelegate::EndEditHint hint);
};

class AirTypesDelegate : public ComboBoxDelegate {
	Q_OBJECT
public:
	explicit AirTypesDelegate(QObject *parent = 0);
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
public
slots:
	void revertModelData(QWidget *widget, QAbstractItemDelegate::EndEditHint hint);
};

class DiveTypesDelegate : public ComboBoxDelegate {
	Q_OBJECT
public:
	explicit DiveTypesDelegate(QObject *parent = 0);
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
public
slots:
	void revertModelData(QWidget *widget, QAbstractItemDelegate::EndEditHint hint);
};

class SpinBoxDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	SpinBoxDelegate(int min, int max, int step, QObject *parent = 0);
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
private:
	int min;
	int max;
	int step;
};

class DoubleSpinBoxDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	DoubleSpinBoxDelegate(double min, double max, double step, QObject *parent = 0);
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
private:
	double min;
	double max;
	double step;
};

class LocationFilterDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	LocationFilterDelegate(QObject *parent = 0);
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setCurrentLocation(location_t loc);
private:
	location_t currentLocation;
};

#endif // MODELDELEGATES_H
