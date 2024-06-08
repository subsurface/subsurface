// SPDX-License-Identifier: GPL-2.0

#include "command_filter.h"
#include "core/divelog.h"
#include "core/filterpreset.h"
#include "core/filterpresettable.h"
#include "core/subsurface-qt/divelistnotifier.h"

namespace Command {

static int createFilterPreset(const std::string &name, const FilterData &data)
{
	int index = divelog.filter_presets.add(name, data);
	emit diveListNotifier.filterPresetAdded(index);
	return index;
}

static std::pair<std::string, FilterData> removeFilterPreset(int index)
{
	const filter_preset &preset = divelog.filter_presets[index];
	std::string oldName = preset.name;
	FilterData oldData = preset.data;
	divelog.filter_presets.remove(index);
	emit diveListNotifier.filterPresetRemoved(index);
	return { oldName, oldData };
}

CreateFilterPreset::CreateFilterPreset(const QString &nameIn, const FilterData &dataIn) :
	name(nameIn.toStdString()), data(dataIn), index(0)
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
	const std::string &name = divelog.filter_presets[index].name;
	setText(Command::Base::tr("Delete filter preset %1").arg(QString::fromStdString(name)));
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
	const std::string &name = divelog.filter_presets[index].name;
	setText(Command::Base::tr("Edit filter preset %1").arg(QString::fromStdString(name)));
}

bool EditFilterPreset::workToBeDone()
{
	return true;
}

void EditFilterPreset::redo()
{
	filter_preset &preset = divelog.filter_presets[index];
	std::swap(data, preset.data);
}

void EditFilterPreset::undo()
{
	redo(); // undo() and redo() do the same thing
}

} // namespace Command
