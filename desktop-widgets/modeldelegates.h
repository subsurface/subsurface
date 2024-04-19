// SPDX-License-Identifier: GPL-2.0
#ifndef MODELDELEGATES_H
#define MODELDELEGATES_H

#include "core/units.h"

#include <QStyledItemDelegate>
#include <QComboBox>
#include <functional>

class QPainter;
struct dive;
struct divecomputer;

class DiveListDelegate : public QStyledItemDelegate {
public:
	using QStyledItemDelegate::QStyledItemDelegate;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class StarWidgetsDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit StarWidgetsDelegate(QWidget *parent = 0);
	const QSize &starSize() const;

private:
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QWidget *parentWidget;
	QSize minStarSize;
};

class ComboBoxDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	// First parameter: function that creates a model and makes it a child of the passed-in widget.
	explicit ComboBoxDelegate(std::function<QAbstractItemModel *(QWidget *)> create_model_func, QObject *parent = 0, bool allowEdit = true);
private
slots:
	void testActivationString(const QString &currString);
	void testActivationIndex(const QModelIndex &currIndex);
	//HACK: try to get rid of this in the future.
	void fakeActivation();
protected
slots:
	virtual void editorClosed(QWidget *widget, QAbstractItemDelegate::EndEditHint hint) = 0;
private:
	std::function<QAbstractItemModel *(QWidget *)> create_model_func;
	bool editable;
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	bool eventFilter(QObject *object, QEvent *event) override;
protected:
	mutable struct CurrSelected {
		QComboBox *comboEditor;
		int currRow;
		QString activeText;
		QAbstractItemModel *model;
		bool ignoreSelection;
	} currCombo;
};

class TankInfoDelegate : public ComboBoxDelegate {
	Q_OBJECT
public:
	explicit TankInfoDelegate(QObject *parent = 0);
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
private:
	void editorClosed(QWidget *widget, QAbstractItemDelegate::EndEditHint hint) override;
};

class TankUseDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit TankUseDelegate(QObject *parent = 0);
	void setCurrentDC(divecomputer *dc);
private:
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	divecomputer *currentdc;
};

class SensorDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit SensorDelegate(QObject *parent = 0);
	void setCurrentDC(divecomputer *dc);
private:
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	divecomputer *currentdc;
};

class WSInfoDelegate : public ComboBoxDelegate {
	Q_OBJECT
public:
	explicit WSInfoDelegate(QObject *parent = 0);
private:
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	void editorClosed(QWidget *widget, QAbstractItemDelegate::EndEditHint hint) override;
};

class AirTypesDelegate : public ComboBoxDelegate {
	Q_OBJECT
public:
	explicit AirTypesDelegate(const dive &d, QObject *parent = 0);
private:
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	void editorClosed(QWidget *widget, QAbstractItemDelegate::EndEditHint hint) override;
};

class DiveTypesDelegate : public ComboBoxDelegate {
	Q_OBJECT
public:
	explicit DiveTypesDelegate(QObject *parent = 0);
private:
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	void editorClosed(QWidget *widget, QAbstractItemDelegate::EndEditHint hint) override;
};

class SpinBoxDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	SpinBoxDelegate(int min, int max, int step, QObject *parent = 0);
private:
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	int min;
	int max;
	int step;
};

class DoubleSpinBoxDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	DoubleSpinBoxDelegate(double min, double max, double step, QObject *parent = 0);
private:
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	double min;
	double max;
	double step;
};

class LocationFilterDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	LocationFilterDelegate(QObject *parent = 0);
	void setCurrentLocation(location_t loc);
private:
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	location_t currentLocation;
};

#endif // MODELDELEGATES_H
