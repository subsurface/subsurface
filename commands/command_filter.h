// SPDX-License-Identifier: GPL-2.0
// Note: this header file is used by the undo-machinery and should not be included elsewhere.

#ifndef COMMAND_FILTER_H
#define COMMAND_FILTER_H

#include "command_base.h"
#include "core/divefilter.h"

struct FilterData;

// We put everything in a namespace, so that we can shorten names without polluting the global namespace
namespace Command {

class CreateFilterPreset final : public Base {
public:
	CreateFilterPreset(const QString &name, const FilterData &data);
private:
	// for redo
	QString name;
	FilterData data;

	// for undo
	int index;

	void undo() override;
	void redo() override;
	bool workToBeDone() override;
};

class RemoveFilterPreset final : public Base {
public:
	RemoveFilterPreset(int index);
private:
	// for undo
	QString name;
	FilterData data;

	// for redo
	int index;

	void undo() override;
	void redo() override;
	bool workToBeDone() override;
};

class EditFilterPreset final : public Base {
public:
	EditFilterPreset(int index, const FilterData &data);
private:
	// for redo and undo
	int index;
	FilterData data;

	void undo() override;
	void redo() override;
	bool workToBeDone() override;
};

} // namespace Command
#endif
