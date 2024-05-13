// SPDX-License-Identifier: GPL-2.0

#include "command_device.h"
#include "core/divelog.h"
#include "core/subsurface-qt/divelistnotifier.h"

namespace Command {

EditDeviceNickname::EditDeviceNickname(const struct divecomputer *dc, const QString &nicknameIn) :
	nickname(nicknameIn.toStdString())
{
	index = get_or_add_device_for_dc(divelog.devices.get(), dc);
	if (index == -1)
		return;

	setText(Command::Base::tr("Set nickname of device %1 (serial %2) to %3").arg(dc->model, dc->serial, nicknameIn));
}

bool EditDeviceNickname::workToBeDone()
{
	return get_device(divelog.devices.get(), index) != nullptr;
}

void EditDeviceNickname::redo()
{
	device *dev = get_device_mutable(divelog.devices.get(), index);
	if (!dev)
		return;
	std::swap(dev->nickName, nickname);
	emit diveListNotifier.deviceEdited();
}

void EditDeviceNickname::undo()
{
	redo(); // undo() and redo() do the same thing
}

} // namespace Command
