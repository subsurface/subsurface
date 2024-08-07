// SPDX-License-Identifier: GPL-2.0

#include "command_device.h"
#include "core/divelog.h"
#include "core/subsurface-qt/divelistnotifier.h"

namespace Command {

EditDeviceNickname::EditDeviceNickname(const struct divecomputer *dc, const QString &nicknameIn) :
	nickname(nicknameIn.toStdString())
{
	index = get_or_add_device_for_dc(divelog.devices, dc);
	if (index == -1)
		return;

	setText(Command::Base::tr("Set nickname of device %1 (serial %2) to %3").arg(dc->model.c_str(), dc->serial.c_str(), nicknameIn));
}

bool EditDeviceNickname::workToBeDone()
{
	return index >= 0;
}

void EditDeviceNickname::redo()
{
	if (index < 0 || static_cast<size_t>(index) >= divelog.devices.size())
		return;
	std::swap(divelog.devices[index].nickName, nickname);
	emit diveListNotifier.deviceEdited();
}

void EditDeviceNickname::undo()
{
	redo(); // undo() and redo() do the same thing
}

} // namespace Command
