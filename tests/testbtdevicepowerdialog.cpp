// SPDX-License-Identifier: GPL-2.0
#include "testbtdevicepowerdialog.h"
#include "desktop-widgets/btdeviceselectiondialog.h"

#include <QCheckBox>
#include <QLabel>
#include <QPushButton>

void TestBtDevicePowerDialog::powerControls()
{
	BtDeviceSelectionDialog dialog;
	auto toggle = dialog.findChild<QPushButton *>("changeDeviceState");
	auto guidance = dialog.findChild<QLabel *>("bluetoothPowerGuidance");
	auto status = dialog.findChild<QLabel *>("dialogStatus");
	auto powered = dialog.findChild<QCheckBox *>("deviceState");
	auto scan = dialog.findChild<QPushButton *>("scan");
	QVERIFY(toggle);
	QVERIFY(guidance);
	QVERIFY(status);
	QVERIFY(powered);
	QVERIFY(scan);

#if defined(Q_OS_MACOS)
	QVERIFY(toggle->isHidden());
	QVERIFY(!guidance->isHidden());
	QVERIFY(guidance->text().contains("System Settings"));
	const QString originalStatus = status->text();
	QVERIFY(QMetaObject::invokeMethod(&dialog, "on_changeDeviceState_clicked"));
	QCOMPARE(status->text(), originalStatus);

	// Simulate a host-mode notification without changing the adapter's power.
	QVERIFY(QMetaObject::invokeMethod(&dialog, "hostModeStateChanged", Qt::DirectConnection,
		Q_ARG(QBluetoothLocalDevice::HostMode, QBluetoothLocalDevice::HostPoweredOff)));
	QVERIFY(!powered->isChecked());
	QVERIFY(!scan->isEnabled());
	QVERIFY(QMetaObject::invokeMethod(&dialog, "hostModeStateChanged", Qt::DirectConnection,
		Q_ARG(QBluetoothLocalDevice::HostMode, QBluetoothLocalDevice::HostConnectable)));
	QVERIFY(powered->isChecked());
	QVERIFY(scan->isEnabled());
	QVERIFY(!guidance->isHidden());
#else
	QVERIFY(!toggle->isHidden());
	QVERIFY(guidance->isHidden());
#endif
}

QTEST_MAIN(TestBtDevicePowerDialog)
