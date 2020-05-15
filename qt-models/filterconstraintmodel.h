// SPDX-License-Identifier: GPL-2.0
#ifndef FILTERCONSTRAINTMODEL_H
#define FILTERCONSTRAINTMODEL_H

#include "core/filterconstraint.h"
#include <QAbstractTableModel>
#include <vector>

class FilterConstraintModel : public QAbstractListModel {
	Q_OBJECT
public:
	enum Roles {
		TYPE_ROLE = Qt::UserRole + 1,	// enum filter_constraint_type cast to int
		IS_STAR_WIDGET_ROLE,		// represent as a star widget
		HAS_DATE_WIDGET_ROLE,		// has a date widget
		HAS_TIME_WIDGET_ROLE,		// has a time widget
		NUM_DECIMALS_ROLE,		// number of decimal places for numeric data
		NEGATE_COMBO_ROLE,		// combo box entries for negate
		STRING_MODE_COMBO_ROLE,		// combo box entries for string mode or empty list if no string mode
		RANGE_MODE_COMBO_ROLE,		// combo box entries for range mode or empty list if no range mode
		MULTIPLE_CHOICE_LIST_ROLE,	// list of translated multiple-choice items
		STRING_MODE_ROLE,		// enum filter_constraint_string_mode_role cast to int
		RANGE_MODE_ROLE,		// enum filter_constraint_range_mode cast to int
		TYPE_DISPLAY_ROLE,		// type for display (i.e. translated)
		NEGATE_DISPLAY_ROLE,		// negate flag for display (i.e. translated)
		STRING_MODE_DISPLAY_ROLE,	// string mode for display (i.e. translated)
		RANGE_MODE_DISPLAY_ROLE,	// range mode for display (i.e. translated)
		NEGATE_INDEX_ROLE,		// negate index in combo box
		TYPE_INDEX_ROLE,		// type index in combo box
		STRING_MODE_INDEX_ROLE,		// string mode index in combo box
		RANGE_MODE_INDEX_ROLE,		// range mode index in combo box
		UNIT_ROLE,			// unit, if any
		STRING_ROLE,			// string data
		INTEGER_FROM_ROLE,
		INTEGER_TO_ROLE,
		FLOAT_FROM_ROLE,
		FLOAT_TO_ROLE,
		TIMESTAMP_FROM_ROLE,
		TIMESTAMP_TO_ROLE,
		TIME_FROM_ROLE,
		TIME_TO_ROLE,
		MULTIPLE_CHOICE_ROLE
	};
private:
	QVariant data(const QModelIndex &index, int role) const override;
	std::vector<filter_constraint> constraints;
public:
	using QAbstractListModel::QAbstractListModel;
	~FilterConstraintModel();
	void reload(const std::vector<filter_constraint> &);
	std::vector<filter_constraint> getConstraints() const; // filters out constraints with no user input
	void addConstraint(filter_constraint_type type);
	void deleteConstraint(int row);
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;
	int rowCount(const QModelIndex &parent) const override;
};

#endif
