// SPDX-License-Identifier: GPL-2.0
#include "filterpresetmodel.h"
#include "core/filterconstraint.h"
#include "core/filterpreset.h"
#include "core/qthelper.h"
#include "core/subsurface-qt/divelistnotifier.h"

FilterPresetModel::FilterPresetModel()
{
	setHeaderDataStrings(QStringList{ "", tr("Name") });
	connect(&diveListNotifier, &DiveListNotifier::dataReset, this, &FilterPresetModel::reset);
	connect(&diveListNotifier, &DiveListNotifier::filterPresetAdded, this, &FilterPresetModel::filterPresetAdded);
	connect(&diveListNotifier, &DiveListNotifier::filterPresetRemoved, this, &FilterPresetModel::filterPresetRemoved);
	connect(&diveListNotifier, &DiveListNotifier::filterPresetChanged, this, &FilterPresetModel::filterPresetChanged);
}

FilterPresetModel::~FilterPresetModel()
{
}

FilterPresetModel *FilterPresetModel::instance()
{
	static FilterPresetModel self;
	return &self;
}

QVariant FilterPresetModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() >= filter_presets_count())
		return QVariant();

	switch (role) {
	case Qt::DisplayRole:
		if (index.column() == NAME)
			return filter_preset_name_qstring(index.row());
		break;
	case Qt::DecorationRole:
		if (index.column() == REMOVE)
			return trashIcon();
		break;
	case Qt::SizeHintRole:
		if (index.column() == REMOVE)
			return trashIcon().size();
		break;
	case Qt::ToolTipRole:
		if (index.column() == REMOVE)
			return tr("Clicking here will remove this filter set.");
		break;
	}
	return QVariant();
}

int FilterPresetModel::rowCount(const QModelIndex &parent) const
{
	return filter_presets_count();
}

void FilterPresetModel::reset()
{
	beginResetModel();
	endResetModel();
}

void FilterPresetModel::filterPresetAdded(int index)
{
	beginInsertRows(QModelIndex(), index, index);
	endInsertRows();
}

void FilterPresetModel::filterPresetChanged(int i)
{
	QModelIndex idx = index(i, 0);
	dataChanged(idx, idx);
}

void FilterPresetModel::filterPresetRemoved(int index)
{
	beginRemoveRows(QModelIndex(), index, index);
	endRemoveRows();
}
