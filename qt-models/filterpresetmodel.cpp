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
