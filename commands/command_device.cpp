// SPDX-License-Identifier: GPL-2.0

#include "command_device.h"
#include "core/subsurface-qt/divelistnotifier.h"

namespace Command {

RemoveDevice::RemoveDevice(int indexIn) : index(indexIn)
{
	const device *dev = get_device(&device_table, index);
	if (!dev)
		return;

	setText(Command::Base::tr("Delete device %1 (0x%2)").arg(QString::fromStdString(dev->model),
								 QString::number(dev->deviceId)));
}

bool RemoveDevice::workToBeDone()
{
	return get_device(&device_table, index) != nullptr;
}

void RemoveDevice::redo()
{
	dev = *get_device(&device_table, index);
	remove_from_device_table(&device_table, index);
	emit diveListNotifier.deviceDeleted(index);
}

void RemoveDevice::undo()
{
	index = add_to_device_table(&device_table, &dev);
	emit diveListNotifier.deviceAdded(index);
}

EditDeviceNickname::EditDeviceNickname(int indexIn, const QString &nicknameIn) :
	index(indexIn), nickname(nicknameIn.toStdString())
{
	const device *dev = get_device(&device_table, index);
	if (!dev)
		return;

	setText(Command::Base::tr("Set nickname of device %1 (0x%2) to %3").arg(QString::fromStdString(dev->model),
										QString::number(dev->deviceId,1 ,16), nicknameIn));
}

bool EditDeviceNickname::workToBeDone()
{
	return get_device(&device_table, index) != nullptr;
}

void EditDeviceNickname::redo()
{
	device *dev = get_device_mutable(&device_table, index);
	if (!dev)
		return;
	std::swap(dev->nickName, nickname);
	emit diveListNotifier.deviceEdited(index);
}

void EditDeviceNickname::undo()
{
	redo(); // undo() and redo() do the same thing
}

} // namespace Command
