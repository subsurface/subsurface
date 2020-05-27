// SPDX-License-Identifier: GPL-2.0

#include "command_filter.h"
#include "core/filterpreset.h"
#include "core/subsurface-qt/divelistnotifier.h"

namespace Command {

static int createFilterPreset(const QString &name, const FilterData &data)
{
	int index = filter_preset_add(name, data);
	emit diveListNotifier.filterPresetAdded(index);
	return index;
}

static std::pair<QString, FilterData> removeFilterPreset(int index)
{
	QString oldName = filter_preset_name_qstring(index);
	FilterData oldData = filter_preset_get(index);
	filter_preset_delete(index);
	emit diveListNotifier.filterPresetRemoved(index);
	return { oldName, oldData };
}

CreateFilterPreset::CreateFilterPreset(const QString &nameIn, const FilterData &dataIn) :
	name(nameIn), data(dataIn), index(0)
{
	setText(Command::Base::tr("Create filter preset %1").arg(nameIn));
}

bool CreateFilterPreset::workToBeDone()
{
	return true;
}

void CreateFilterPreset::redo()
{
	index = createFilterPreset(name, data);
}

void CreateFilterPreset::undo()
{
	// with std::tie() we can conveniently assign tuples
	std::tie(name, data) = removeFilterPreset(index);
}

RemoveFilterPreset::RemoveFilterPreset(int indexIn) : index(indexIn)
{
	setText(Command::Base::tr("Delete filter preset %1").arg(filter_preset_name_qstring(index)));
}

bool RemoveFilterPreset::workToBeDone()
{
	return true;
}

void RemoveFilterPreset::redo()
{
	// with std::tie() we can conveniently assign tuples
	std::tie(name, data) = removeFilterPreset(index);
}

void RemoveFilterPreset::undo()
{
	index = createFilterPreset(name, data);
}

EditFilterPreset::EditFilterPreset(int indexIn, const FilterData &dataIn) :
	index(indexIn), data(dataIn)
{
	setText(Command::Base::tr("Edit filter preset %1").arg(filter_preset_name_qstring(index)));
}

bool EditFilterPreset::workToBeDone()
{
	return true;
}

void EditFilterPreset::redo()
{
	FilterData oldData = filter_preset_get(index);
	filter_preset_set(index, data);
	data = std::move(oldData);
}

void EditFilterPreset::undo()
{
	redo(); // undo() and redo() do the same thing
}

} // namespace Command
