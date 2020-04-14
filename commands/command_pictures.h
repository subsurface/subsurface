// SPDX-License-Identifier: GPL-2.0
// Note: this header file is used by the undo-machinery and should not be included elsewhere.

#ifndef COMMAND_PICTURES_H
#define COMMAND_PICTURES_H

#include "command_base.h"

// We put everything in a namespace, so that we can shorten names without polluting the global namespace
namespace Command {

class SetPictureOffset final : public Base {
public:
	SetPictureOffset(dive *d, const QString &filename, offset_t offset); // Pictures are identified by the unique (dive,filename) pair
private:
	dive *d; // null means no work to be done
	QString filename;
	offset_t offset;

	void undo() override;
	void redo() override;
	bool workToBeDone() override;
};

} // namespace Command
#endif
