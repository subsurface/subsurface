// SPDX-License-Identifier: GPL-2.0
#ifndef FILTERCONSTRAINTWIDGET_H
#define FILTERCONSTRAINTWIDGET_H

#include "core/filterconstraint.h"
#include <QWidget>
#include <memory>

class FilterConstraintModel;
class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QGridLayout;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTimeEdit;
class StarWidget;

// Technically, this is not a single widget, but numerous widgets,
// which are added to the row of a QGridLayout
class FilterConstraintWidget : public QObject {
	Q_OBJECT
public:
	FilterConstraintWidget(FilterConstraintModel *model, const QModelIndex &index, QGridLayout *layout);
	~FilterConstraintWidget();
	void moveToRow(int row); // call if the row of the widget has changed.
				 // this will update the index used to access the model as well as the position in the layout
	void update();
private
slots:
	void trash();
	void stringEdited(const QString &s);
	void multipleChoiceEdited();
	void negateEdited(int index);
	void rangeModeEdited(int index);
	void stringModeEdited(int index);
	void fromEditedInt(int i);
	void toEditedInt(int i);
	void fromEditedFloat(double f);
	void toEditedFloat(double f);
	void fromEditedTimestamp(const QDateTime &datetime);
	void toEditedTimestamp(const QDateTime &datetime);
private:
	QGridLayout *layout;
	FilterConstraintModel *model;
	int row;
	const filter_constraint_type type; // we don't support changing the type
	void addToLayout();
	void removeFromLayout();

	std::unique_ptr<QHBoxLayout> rangeLayout;
	std::unique_ptr<QPushButton> trashButton;
	std::unique_ptr<QLabel> typeLabel;
	std::unique_ptr<QComboBox> negate;
	std::unique_ptr<QComboBox> stringMode;
	std::unique_ptr<QComboBox> rangeMode;
	std::unique_ptr<QListWidget> multipleChoice;
	std::unique_ptr<QLineEdit> string;
	std::unique_ptr<StarWidget> starFrom, starTo;
	std::unique_ptr<QDoubleSpinBox> spinBoxFrom, spinBoxTo;
	std::unique_ptr<QDateEdit> dateFrom, dateTo;
	std::unique_ptr<QTimeEdit> timeFrom, timeTo;
	std::unique_ptr<QLabel> unitFrom, unitTo;
	std::unique_ptr<QLabel> toLabel;
};

#endif
