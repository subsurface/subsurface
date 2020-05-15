// SPDX-License-Identifier: GPL-2.0
#include "filterconstraintmodel.h"
#include "core/qthelper.h"	// for timestamp conversions
#include <QTime>

FilterConstraintModel::~FilterConstraintModel()
{
}

// QTime <-> seconds in integer conversion functions
static QTime secondsToTime(int seconds)
{
	return QTime::fromMSecsSinceStartOfDay(seconds * 1000);
}

static int timeToSeconds(const QTime &t)
{
	return t.msecsSinceStartOfDay() / 1000;
}

QVariant FilterConstraintModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() >= (int)constraints.size())
		return QVariant();

	const filter_constraint &constraint = constraints[index.row()];

	switch (role) {
	case TYPE_ROLE:
		return constraint.type;
	case IS_STAR_WIDGET_ROLE:
		return filter_constraint_is_star(constraint.type);
	case HAS_DATE_WIDGET_ROLE:
		return filter_constraint_has_date_widget(constraint.type);
	case HAS_TIME_WIDGET_ROLE:
		return filter_constraint_has_time_widget(constraint.type);
	case NUM_DECIMALS_ROLE:
		return filter_constraint_num_decimals(constraint.type);
	case NEGATE_COMBO_ROLE:
		return filter_constraint_negate_list_translated();
	case STRING_MODE_COMBO_ROLE:
		return filter_constraint_has_string_mode(constraint.type) ?
			filter_constraint_string_mode_list_translated() : QStringList();
	case RANGE_MODE_COMBO_ROLE:
		return filter_constraint_has_range_mode(constraint.type) ?
			filter_constraint_range_mode_list_translated() : QStringList();
	case MULTIPLE_CHOICE_LIST_ROLE:
		return filter_contraint_multiple_choice_translated(constraint.type);
	case STRING_MODE_ROLE:
		return static_cast<int>(constraint.string_mode);
	case RANGE_MODE_ROLE:
		return static_cast<int>(constraint.range_mode);
	case TYPE_DISPLAY_ROLE:
		return filter_constraint_type_to_string_translated(constraint.type);
	case NEGATE_DISPLAY_ROLE:
		return filter_constraint_negate_to_string_translated(constraint.negate);
	case STRING_MODE_DISPLAY_ROLE:
		return filter_constraint_string_mode_to_string_translated(constraint.string_mode);
	case RANGE_MODE_DISPLAY_ROLE:
		return filter_constraint_range_mode_to_string_translated(constraint.range_mode);
	case TYPE_INDEX_ROLE:
		return filter_constraint_type_to_index(constraint.type);
	case NEGATE_INDEX_ROLE:
		return static_cast<int>(constraint.negate);
	case STRING_MODE_INDEX_ROLE:
		return filter_constraint_string_mode_to_index(constraint.string_mode);
	case RANGE_MODE_INDEX_ROLE:
		return filter_constraint_range_mode_to_index(constraint.range_mode);
	case UNIT_ROLE:
		return filter_constraint_get_unit(constraint.type);
	case STRING_ROLE:
		return filter_constraint_get_string(constraint);
	case INTEGER_FROM_ROLE:
		return filter_constraint_get_integer_from(constraint);
	case INTEGER_TO_ROLE:
		return filter_constraint_get_integer_to(constraint);
	case FLOAT_FROM_ROLE:
		return filter_constraint_get_float_from(constraint); // Converts from integers to metric or imperial units
	case FLOAT_TO_ROLE:
		return filter_constraint_get_float_to(constraint); // Converts from integers to metric or imperial units
	case TIMESTAMP_FROM_ROLE:
		return timestampToDateTime(filter_constraint_get_timestamp_from(constraint));
	case TIMESTAMP_TO_ROLE:
		return timestampToDateTime(filter_constraint_get_timestamp_to(constraint));
	case TIME_FROM_ROLE:
		return secondsToTime(filter_constraint_get_integer_from(constraint));
	case TIME_TO_ROLE:
		return secondsToTime(filter_constraint_get_integer_from(constraint));
	case MULTIPLE_CHOICE_ROLE:
		return (qulonglong)filter_constraint_get_multiple_choice(constraint);
	}
	return QVariant();
}

bool FilterConstraintModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid() || index.row() > (int)constraints.size())
		return false;
	filter_constraint &constraint = constraints[index.row()];

	switch (role) {
	case NEGATE_INDEX_ROLE:
		constraint.negate = value.value<bool>();
		break;
	case STRING_ROLE:
		filter_constraint_set_stringlist(constraint, value.value<QString>());
		break;
	case STRING_MODE_INDEX_ROLE:
		constraint.string_mode = filter_constraint_string_mode_from_index(value.value<int>());
		break;
	case RANGE_MODE_INDEX_ROLE:
		constraint.range_mode = filter_constraint_range_mode_from_index(value.value<int>());
		break;
	case INTEGER_FROM_ROLE:
		filter_constraint_set_integer_from(constraint, value.value<int>());
		break;
	case INTEGER_TO_ROLE:
		filter_constraint_set_integer_to(constraint, value.value<int>());
		break;
	case FLOAT_FROM_ROLE:
		filter_constraint_set_float_from(constraint, value.value<double>());
		break;
	case FLOAT_TO_ROLE:
		filter_constraint_set_float_to(constraint, value.value<double>());
		break;
	case TIMESTAMP_FROM_ROLE:
		filter_constraint_set_timestamp_from(constraint, dateTimeToTimestamp(value.value<QDateTime>()));
		break;
	case TIMESTAMP_TO_ROLE:
		filter_constraint_set_timestamp_to(constraint, dateTimeToTimestamp(value.value<QDateTime>()));
		break;
	case TIME_FROM_ROLE:
		filter_constraint_set_integer_from(constraint, timeToSeconds(value.value<QTime>()));
		break;
	case TIME_TO_ROLE:
		filter_constraint_set_integer_to(constraint, timeToSeconds(value.value<QTime>()));
		break;
	case MULTIPLE_CHOICE_ROLE:
		filter_constraint_set_multiple_choice(constraint, value.value<uint64_t>());
		break;
	default:
		return false;
	}
	emit dataChanged(index, index, QVector<int> { role });
	return true;
}

int FilterConstraintModel::rowCount(const QModelIndex&) const
{
	return constraints.size();
}

void FilterConstraintModel::reload(const std::vector<filter_constraint> &constraintsIn)
{
	beginResetModel();
	constraints = constraintsIn;
	endResetModel();
}

std::vector<filter_constraint> FilterConstraintModel::getConstraints() const
{
	std::vector<filter_constraint> res;
	res.reserve(constraints.size());
	for (const filter_constraint &c: constraints) {
		if (filter_constraint_is_valid(&c))
			res.push_back(c);
	}
	return res;
}

void FilterConstraintModel::addConstraint(filter_constraint_type type)
{
	int count = (int)constraints.size();
	beginInsertRows(QModelIndex(), count, count);
	constraints.emplace_back(type);
	endInsertRows();
}

void FilterConstraintModel::deleteConstraint(int row)
{
	if (row < 0 || row >= (int)constraints.size())
		return;
	beginRemoveRows(QModelIndex(), row, row);
	constraints.erase(constraints.begin() + row);
	endRemoveRows();
}
