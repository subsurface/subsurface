// SPDX-License-Identifier: GPL-2.0
#include "testostc4dialog.h"
#include "desktop-widgets/configuredivecomputerdialog.h"
#include "core/pref.h"
#ifdef BT_SUPPORT
#include "core/qtserialbluetooth.h"
#include <QBluetoothSocket>
#endif
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QSignalSpy>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QDesktopServices>
#include <map>
#include <atomic>

static QString testDialogPath(const char *name)
{
	return QDir(QDir::tempPath()).filePath(QString::fromLatin1(name));
}

class FileUrlHandlerGuard {
public:
	explicit FileUrlHandlerGuard(QObject *receiver)
	{
		QDesktopServices::setUrlHandler(QStringLiteral("file"), receiver, "recordOpenedUrl");
	}
	~FileUrlHandlerGuard() { QDesktopServices::unsetUrlHandler(QStringLiteral("file")); }
};

void TestOstc4Dialog::recordOpenedUrl(QUrl url)
{
	openedUrl = std::move(url);
}

void TestOstc4Dialog::rfcommDisconnectWait()
{
#if defined(BT_SUPPORT) && defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	class SimulatedSocket : public QBluetoothSocket {
	public:
		void transition(SocketState state) { setSocketState(state); }
	};
	SimulatedSocket socket;
	QVERIFY(waitForRfcommDisconnect(&socket, 50)); // already disconnected
	socket.transition(QBluetoothSocket::SocketState::ConnectedState);
	QTimer::singleShot(10, &socket, [&socket] {
		socket.transition(QBluetoothSocket::SocketState::UnconnectedState);
	});
	QVERIFY(waitForRfcommDisconnect(&socket, 100)); // actual production event loop
	socket.transition(QBluetoothSocket::SocketState::ConnectedState);
	QElapsedTimer elapsed;
	elapsed.start();
	QVERIFY(!waitForRfcommDisconnect(&socket, 30));
	QVERIFY(elapsed.elapsed() >= 30 && elapsed.elapsed() < 500);
#endif
}

void TestOstc4Dialog::clampedGradientFactorsStayUnchanged()
{
	using Transaction = Ostc4SettingsTransaction;
	const Transaction::Identity identity { 0x43, 428, static_cast<quint16>((1 << 11) | (7 << 6) | (6 << 1) | 1) };
	std::map<quint8, QByteArray> device;
	for (int id = 0x10; id <= 0x44; ++id)
		device[id] = QByteArray::fromHex("00000000");
	device[0x25][0] = 5; // UI minimum is 10.
	device[0x26][0] = 50; // UI minimum is 60, but 60 passes transaction validation.
	Transaction snapshot;
	QVERIFY(snapshot.capture([&](Transaction::Identity &out) { out = identity; return true; },
		[&](quint8 id, QByteArray &out) { out = device.at(id); return true; }));
	DeviceDetails details;
	details.model = QStringLiteral("OSTC 4");
	details.ostc4Snapshot = snapshot;
	details.gfLow = 5;
	details.gfHigh = 50;
	ConfigureDiveComputerDialog dialog(testDialogPath("ostc4-gf-test.xml"));
	dialog.deviceDetailsReceived(details);
	QVERIFY(!dialog.ui.gfLowSpinBox_4->isEnabled());
	QVERIFY(!dialog.ui.gfHighSpinBox_4->isEnabled());
	QVERIFY(!dialog.deviceDetails.ostc4Editable[0x25 - 0x10]);
	QVERIFY(!dialog.deviceDetails.ostc4Editable[0x26 - 0x10]);

	// The populated details contain the clamped values, but cannot change either byte.
	dialog.populateDeviceDetails();
	device_data_t data;
	data.product = "OSTC 4";
	WriteSettingsThread worker(nullptr, &data);
	DeviceDetails edited = dialog.deviceDetails;
	edited.sp4 = setpoint(0, 0);
	edited.sp5 = setpoint(0, 0);
	worker.setDeviceDetails(edited);
	auto io = std::make_shared<Ostc4SettingsIO>();
	io->identify = [&](Transaction::Identity &out) { out = identity; return true; };
	io->read = [&](quint8 id, QByteArray &out) { out = device.at(id); return true; };
	int writes = 0;
	io->write = [&](quint8, const QByteArray &) { ++writes; return true; };
	worker.setIO(io);
	QSignalSpy results(&worker, &WriteSettingsThread::result);
	QSignalSpy errors(&worker, &WriteSettingsThread::error);
	worker.perform();
	QCOMPARE(errors.size(), 0);
	QCOMPARE(writes, 0);
	QVERIFY(!results.isEmpty());
	QVERIFY(results.first().first().toString().contains(QStringLiteral("No changes")));
}

void TestOstc4Dialog::settingsBlockedDuringFirmwareAndReset()
{
	ConfigureDiveComputer config;
	device_data_t data;
	DeviceDetails details;
	QSignalSpy errors(&config, &ConfigureDiveComputer::error);
	QSignalSpy states(&config, &ConfigureDiveComputer::stateChanged);
	int opens = 0;
	config.setReadTransportForTesting([&](device_data_t *) { ++opens; return QString(); },
		[](device_data_t *) { return DC_STATUS_SUCCESS; }, [](device_data_t *) { return true; });
	for (auto state : { ConfigureDiveComputer::READING, ConfigureDiveComputer::WRITING,
	                   ConfigureDiveComputer::FWUPDATE, ConfigureDiveComputer::RESETTING }) {
		config.currentState = state; // Model an active worker without touching hardware.
		config.readSettings(&data);
		config.saveDeviceDetails(details, &data);
		QVERIFY(!config.openForOtherOperation(&data).isEmpty());
		config.startFirmwareUpdate(QString(), &data, false);
		config.resetSettings(&data);
		QCOMPARE(config.currentState, state);
	}
	QCOMPARE(errors.size(), 16);
	QCOMPARE(states.size(), 0);
	QCOMPARE(opens, 0);
}

void TestOstc4Dialog::resetAndFirmwareIgnorePreviousError()
{
	device_data_t data;
	ConfigureDiveComputer config;
	config.lastError = QStringLiteral("previous settings error");
	config.currentState = ConfigureDiveComputer::READING;
	config.resetSettings(&data);
	config.startFirmwareUpdate(QString(), &data, false);
	QCOMPARE(config.currentState, ConfigureDiveComputer::READING);
	QCOMPARE(config.lastError, QStringLiteral("previous settings error"));

	// A null device is a no-op reset in the worker, without touching hardware.
	config.currentState = ConfigureDiveComputer::ERRORED;
	config.resetSettings(&data);
	QTRY_COMPARE(config.currentState, ConfigureDiveComputer::DONE);
	QVERIFY(config.lastError.isEmpty());

	// Firmware registration rejects the null device: only that new error remains.
	config.lastError = QStringLiteral("previous settings error");
	config.startFirmwareUpdate(QString(), &data, false);
	QTRY_COMPARE(config.currentState, ConfigureDiveComputer::ERRORED);
	QVERIFY(config.lastError.contains(QStringLiteral("event handler")));
	QVERIFY(!config.lastError.contains(QStringLiteral("previous settings error")));
}

void TestOstc4Dialog::resetInvalidatesOstc4Snapshot()
{
	ConfigureDiveComputerDialog dialog(testDialogPath("ostc4-reset-snapshot-test.xml"));
	dialog.ui.dcStackedWidget->setCurrentIndex(2);
	dialog.deviceDetails.ostc4Snapshot.valid = true;
	dialog.deviceDetails.ostc4Editable[0] = true;
	dialog.ui.page_ostc4->setEnabled(true);
	dialog.ui.saveSettingsPushButton->setEnabled(true);
	dialog.ui.backupButton->setEnabled(true);
	dialog.ui.restoreBackupButton->setEnabled(true);
	dialog.processStateChanged(ConfigureDiveComputer::DONE, ConfigureDiveComputer::RESETTING);
	QVERIFY(!dialog.deviceDetails.ostc4Snapshot.valid);
	QVERIFY(!dialog.deviceDetails.ostc4Editable[0]);
	QVERIFY(!dialog.ui.page_ostc4->isEnabled());
	QVERIFY(!dialog.ui.saveSettingsPushButton->isEnabled());
	QVERIFY(!dialog.ui.backupButton->isEnabled());
	QVERIFY(!dialog.ui.restoreBackupButton->isEnabled());
	dialog.processStateChanged(ConfigureDiveComputer::RESETTING, ConfigureDiveComputer::DONE);
	QVERIFY(!dialog.ui.page_ostc4->isEnabled());
	QVERIFY(!dialog.ui.saveSettingsPushButton->isEnabled());
	QVERIFY(!dialog.ui.backupButton->isEnabled());
	QVERIFY(!dialog.ui.retrieveDetails->isEnabled()); // No device was opened in this simulation.

	// The controller also rejects the old baseline after reset, independently of the UI.
	device_data_t data;
	data.product = "OSTC 4";
	ConfigureDiveComputer config;
	DeviceDetails details;
	details.ostc4Snapshot.valid = true;
	config.setReadDetailsForTesting(details);
	config.setReadTransportForTesting([](device_data_t *) { return QString(); },
		[](device_data_t *) { return DC_STATUS_SUCCESS; }, [](device_data_t *) { return true; });
	config.readSettings(&data);
	QTRY_COMPARE(config.currentState, ConfigureDiveComputer::DONE);
	config.saveDeviceDetails(details, &data);
	QVERIFY(config.lastError.contains(QStringLiteral("recognised device descriptor")));
	config.resetSettings(&data);
	QTRY_COMPARE(config.currentState, ConfigureDiveComputer::DONE);
	config.saveDeviceDetails(details, &data);
	QVERIFY(config.lastError.contains(QStringLiteral("fresh read")));
}

void TestOstc4Dialog::disconnectBlockedDuringDeviceOperations()
{
	ConfigureDiveComputerDialog dialog(testDialogPath("ostc4-active-disconnect-test.xml"));
	dialog.deviceDetails.ostc4Snapshot.valid = true;
	for (auto state : { ConfigureDiveComputer::READING, ConfigureDiveComputer::WRITING,
	                   ConfigureDiveComputer::FWUPDATE, ConfigureDiveComputer::RESETTING }) {
		dialog.config->currentState = state; // Model an active worker without hardware.
		dialog.dc_close();
		QCOMPARE(dialog.config->currentState, state);
		QVERIFY(dialog.deviceDetails.ostc4Snapshot.valid);
		QVERIFY(dialog.ui.progressBar->format().contains(QStringLiteral("finish before disconnecting")));
		QCloseEvent event;
		dialog.closeEvent(&event);
		QVERIFY(!event.isAccepted());
		QCOMPARE(dialog.config->currentState, state);
		QVERIFY(dialog.deviceDetails.ostc4Snapshot.valid);
	}
	dialog.config->currentState = ConfigureDiveComputer::INITIAL;
}

void TestOstc4Dialog::rejectPreservesOperationAndDisconnect()
{
	ConfigureDiveComputerDialog dialog(testDialogPath("ostc4-reject-test.xml"));
	dialog.show();
	QSignalSpy rejected(&dialog, &QDialog::rejected);
	for (auto state : { ConfigureDiveComputer::READING, ConfigureDiveComputer::WRITING,
	                   ConfigureDiveComputer::FWUPDATE, ConfigureDiveComputer::RESETTING }) {
		dialog.config->currentState = state; // Model an active worker without touching hardware.
		dialog.reject(); // Escape is delivered through reject(), not closeEvent().
		QVERIFY(dialog.isVisible());
		QCOMPARE(dialog.config->currentState, state);
		QCOMPARE(rejected.size(), 0);
	}
	dialog.config->currentState = ConfigureDiveComputer::DONE;
	dialog.deviceDetails.ostc4Snapshot.valid = true;
	dialog.reject();
	QVERIFY(!dialog.isVisible());
	QCOMPARE(rejected.size(), 1);
	QCOMPARE(dialog.config->currentState, ConfigureDiveComputer::INITIAL);
	QVERIFY(!dialog.deviceDetails.ostc4Snapshot.valid);
}

void TestOstc4Dialog::failedTeardownKeepsDialogOpen()
{
	for (bool escape : { true, false }) {
		ConfigureDiveComputerDialog dialog(testDialogPath("ostc4-reject-close-failure-test.xml"));
		for (int row = 0; row < dialog.ui.DiveComputerList->count(); ++row)
			if (dialog.ui.DiveComputerList->item(row)->text().contains(QStringLiteral("OSTC 4/5")))
				dialog.ui.DiveComputerList->setCurrentRow(row);
		int closes = 0;
		dialog.config->setReadTransportForTesting([](device_data_t *) { return QString(); },
			[&](device_data_t *) { ++closes; return DC_STATUS_IO; }, [](device_data_t *) { return true; });
		dialog.ui.connectButton->click();
		dialog.ui.retrieveDetails->click();
		QTRY_COMPARE(dialog.config->currentState, ConfigureDiveComputer::DONE);
		dialog.show();
		QSignalSpy rejected(&dialog, &QDialog::rejected);
		if (escape) {
			dialog.reject();
		} else {
			QCloseEvent event;
			dialog.closeEvent(&event);
			QVERIFY(!event.isAccepted());
		}
		QVERIFY(dialog.isVisible());
		QCOMPARE(rejected.size(), 0);
		QCOMPARE(closes, 1);
		QCOMPARE(dialog.config->currentState, ConfigureDiveComputer::ERRORED);
		QVERIFY(dialog.ui.progressBar->toolTip().contains(QStringLiteral("did not close cleanly")));
	}
}

void TestOstc4Dialog::openFailureWithoutCloseHook()
{
	device_data_t data;
	ReadSettingsThread read(nullptr, &data);
	read.setTransport([](device_data_t *) { return QStringLiteral("test open failure"); }, {});
	QSignalSpy readErrors(&read, &ReadSettingsThread::error);
	read.start();
	QVERIFY(read.wait(3000));
	QCOMPARE(readErrors.size(), 1);
	QVERIFY(readErrors.first().first().toString().contains(QStringLiteral("test open failure")));

	WriteSettingsThread write(nullptr, &data);
	write.setTransport([](device_data_t *) { return QStringLiteral("test open failure"); }, {});
	QSignalSpy writeErrors(&write, &WriteSettingsThread::error);
	write.start();
	QVERIFY(write.wait(3000));
	QCOMPARE(writeErrors.size(), 1);
	QVERIFY(writeErrors.first().first().toString().contains(QStringLiteral("test open failure")));
}

void TestOstc4Dialog::closeFailureBlocksNextOperation()
{
	for (bool reread : { true, false }) {
		ConfigureDiveComputerDialog dialog(testDialogPath("ostc4-close-failure-test.xml"));
		for (int row = 0; row < dialog.ui.DiveComputerList->count(); ++row)
			if (dialog.ui.DiveComputerList->item(row)->text().contains(QStringLiteral("OSTC 4/5")))
				dialog.ui.DiveComputerList->setCurrentRow(row);
		int opens = 0, closes = 0;
		bool failClose = true;
		DeviceDetails details;
		details.model = QStringLiteral("OSTC 4");
		details.ostc4Snapshot.valid = true;
		details.ostc4Snapshot.identity.model = 0x43;
		details.ostc4Snapshot.identity.serial = 428;
		dialog.config->setReadDetailsForTesting(details);
		dialog.config->setReadTransportForTesting([&](device_data_t *) { ++opens; return QString(); },
			[&](device_data_t *) { ++closes; return failClose ? DC_STATUS_IO : DC_STATUS_SUCCESS; },
			[](device_data_t *) { return true; });
		dialog.ui.connectButton->click();
		dialog.ui.retrieveDetails->click();
		QTRY_COMPARE(dialog.config->currentState, ConfigureDiveComputer::DONE);
		QCOMPARE(opens, 1);
		QVERIFY(dialog.deviceDetails.ostc4Snapshot.valid);
		QSignalSpy errors(dialog.config, &ConfigureDiveComputer::error);
		if (reread) {
			dialog.config->readSettings(&dialog.device_data);
			QCOMPARE(dialog.config->currentState, ConfigureDiveComputer::ERRORED);
			QCOMPARE(errors.size(), 1);
			QVERIFY(errors.first().first().toString().contains(QStringLiteral("did not close cleanly")));
			QVERIFY(!dialog.deviceDetails.ostc4Snapshot.valid);
			QVERIFY(!dialog.ui.page_ostc4->isEnabled());
			QVERIFY(!dialog.ui.retrieveDetails->isEnabled());
			QVERIFY(!dialog.ui.saveSettingsPushButton->isEnabled());
			QVERIFY(!dialog.ui.backupButton->isEnabled());
			QTemporaryDir directory;
			QVERIFY(directory.isValid());
			const QString path = directory.filePath(QStringLiteral("stale.json"));
			QString error;
			QVERIFY(!dialog.saveManualOstc4Backup(path, error));
			QVERIFY(error.contains(QStringLiteral("fresh read")));
			QVERIFY(!QFile::exists(path));
		} else {
			const QString result = dialog.config->openForOtherOperation(&dialog.device_data);
			QVERIFY(result.contains(QStringLiteral("did not close cleanly")));
			QCOMPARE(dialog.config->currentState, ConfigureDiveComputer::ERRORED);
			QVERIFY(!dialog.deviceDetails.ostc4Snapshot.valid);
			QVERIFY(!dialog.ui.page_ostc4->isEnabled());
			QVERIFY(!dialog.ui.retrieveDetails->isEnabled());
			QVERIFY(!dialog.ui.saveSettingsPushButton->isEnabled());
			QVERIFY(!dialog.ui.backupButton->isEnabled());
			QVERIFY(!dialog.ui.restoreBackupButton->isEnabled());
			QTemporaryDir directory;
			QVERIFY(directory.isValid());
			QString error;
			const QString path = directory.filePath(QStringLiteral("stale.json"));
			QVERIFY(!dialog.saveManualOstc4Backup(path, error));
			QVERIFY(error.contains(QStringLiteral("fresh read")));
			QVERIFY(!QFile::exists(path));
			dialog.config->saveDeviceDetails(details, &dialog.device_data);
			QVERIFY(dialog.config->lastError.contains(QStringLiteral("fresh read")));
		}
		QCOMPARE(closes, 1);
		QCOMPARE(opens, 1); // No new transport after a failed close.
		if (reread) {
			dialog.config->readSettings(&dialog.device_data);
			QCOMPARE(dialog.config->currentState, ConfigureDiveComputer::ERRORED);
			QCOMPARE(opens, 1); // An immediate Retrieve retry cannot bypass the failed close.
			QVERIFY(dialog.config->lastError.contains(QStringLiteral("Reconnect")));
			failClose = false;
			QVERIFY(dialog.dc_close());
			dialog.ui.connectButton->click();
			QVERIFY(dialog.ui.retrieveDetails->isEnabled());
			dialog.ui.retrieveDetails->click();
			QTRY_COMPARE(dialog.config->currentState, ConfigureDiveComputer::DONE);
			QCOMPARE(opens, 2);
			dialog.config->readSettings(&dialog.device_data);
			QTRY_COMPARE(dialog.config->currentState, ConfigureDiveComputer::DONE);
			QCOMPARE(opens, 3); // A successful reconnect cleared the guard.
		}
	}
}

void TestOstc4Dialog::closeFailureBlocksOstc3Save()
{
	ConfigureDiveComputerDialog dialog(testDialogPath("ostc3-save-close-failure-test.xml"));
	int row = -1;
	for (int i = 0; i < dialog.ui.DiveComputerList->count(); ++i)
		if (dialog.ui.DiveComputerList->item(i)->text().contains(QStringLiteral("OSTC 2/3/Sport/Plus/cR"))) { row = i; break; }
	QVERIFY(row >= 0);
	dialog.ui.DiveComputerList->setCurrentRow(row);
	int opens = 0, closes = 0;
	dialog.config->setReadTransportForTesting([&](device_data_t *) { ++opens; return QString(); },
		[&](device_data_t *) { ++closes; return DC_STATUS_IO; }, [](device_data_t *) { return true; });
	dialog.ui.connectButton->click();
	QVERIFY(dialog.device_data.descriptor);
	QCOMPARE(dc_descriptor_get_type(dialog.device_data.descriptor), DC_FAMILY_HW_OSTC3);
	dialog.ui.retrieveDetails->click();
	QTRY_COMPARE(dialog.config->currentState, ConfigureDiveComputer::DONE);
	QCOMPARE(opens, 1);
	QSignalSpy errors(dialog.config, &ConfigureDiveComputer::error);
	DeviceDetails details;
	dialog.config->saveDeviceDetails(details, &dialog.device_data);
	QCOMPARE(dialog.config->currentState, ConfigureDiveComputer::ERRORED);
	QCOMPARE(closes, 1);
	QCOMPARE(opens, 1);
	QCOMPARE(errors.size(), 1);
	QVERIFY(errors.first().first().toString().contains(QStringLiteral("did not close cleanly")));
	QVERIFY(dialog.config->findChildren<WriteSettingsThread *>().isEmpty());
}

void TestOstc4Dialog::writeCloseFailureReportsError()
{
	for (bool failWrite : { false, true }) {
		ConfigureDiveComputerDialog dialog(testDialogPath("ostc3-write-close-failure-test.xml"));
		int row = -1;
		for (int i = 0; i < dialog.ui.DiveComputerList->count(); ++i)
			if (dialog.ui.DiveComputerList->item(i)->text().contains(QStringLiteral("OSTC 2/3/Sport/Plus/cR"))) { row = i; break; }
		QVERIFY(row >= 0);
		dialog.ui.DiveComputerList->setCurrentRow(row);
		dialog.ui.connectButton->click();
		QCOMPARE(dc_descriptor_get_type(dialog.device_data.descriptor), DC_FAMILY_HW_OSTC3);
		int opens = 0, writes = 0, closes = 0;
		dialog.config->setWriteTransportForTesting([&](device_data_t *) { ++opens; return QString(); },
			[&](device_data_t *) { ++closes; return DC_STATUS_IO; },
			[&](device_data_t *) { ++writes; return !failWrite; });
		DeviceDetails details;
		dialog.config->saveDeviceDetails(details, &dialog.device_data);
		QTRY_COMPARE(dialog.config->currentState, ConfigureDiveComputer::ERRORED);
		QCOMPARE(opens, 1);
		QCOMPARE(writes, 1);
		QCOMPARE(closes, 1);
		QVERIFY(dialog.config->lastError.contains(QStringLiteral("did not close cleanly")));
		QVERIFY(!dialog.ui.retrieveDetails->isEnabled());
		QVERIFY(!dialog.ui.saveSettingsPushButton->isEnabled());
		QVERIFY(dialog.ui.connectButton->isEnabled());
		if (failWrite)
			QVERIFY(dialog.config->lastError.contains(QStringLiteral("Settings operation failed")));
		else
			QVERIFY(!dialog.config->lastError.contains(QStringLiteral("Settings operation failed")));
	}
}

void TestOstc4Dialog::precloseFailureBlocksRead()
{
	ConfigureDiveComputerDialog dialog(testDialogPath("ostc3-preclose-failure-test.xml"));
	int row = -1;
	for (int i = 0; i < dialog.ui.DiveComputerList->count(); ++i)
		if (dialog.ui.DiveComputerList->item(i)->text().contains(QStringLiteral("OSTC 2/3/Sport/Plus/cR"))) { row = i; break; }
	QVERIFY(row >= 0);
	dialog.ui.DiveComputerList->setCurrentRow(row);
	dialog.ui.connectButton->click();
	QVERIFY(dialog.device_data.descriptor);
	QCOMPARE(dc_descriptor_get_type(dialog.device_data.descriptor), DC_FAMILY_HW_OSTC3);
	QCOMPARE(dc_context_new(&dialog.device_data.context), DC_STATUS_SUCCESS); // Simulate a GUI-owned session.
	int opens = 0, closes = 0;
	dialog.config->setReadTransportForTesting([&](device_data_t *) { ++opens; return QString(); },
		[&](device_data_t *data) {
			Q_ASSERT(QThread::currentThread() == qApp->thread());
			++closes;
			dc_context_free(data->context);
			data->context = nullptr;
			return DC_STATUS_IO;
		}, [](device_data_t *) { return true; });
	QSignalSpy errors(dialog.config, &ConfigureDiveComputer::error);
	dialog.ui.retrieveDetails->click();
	QCOMPARE(dialog.config->currentState, ConfigureDiveComputer::ERRORED);
	QCOMPARE(closes, 1);
	QCOMPARE(opens, 0);
	QCOMPARE(errors.size(), 1);
	QVERIFY(errors.first().first().toString().contains(QStringLiteral("did not close cleanly")));
	QVERIFY(dialog.config->findChildren<ReadSettingsThread *>().isEmpty());
}

void TestOstc4Dialog::disconnectReportsCloseFailure()
{
	ConfigureDiveComputerDialog dialog(testDialogPath("ostc4-disconnect-failure-test.xml"));
	for (int row = 0; row < dialog.ui.DiveComputerList->count(); ++row)
		if (dialog.ui.DiveComputerList->item(row)->text().contains(QStringLiteral("OSTC 4/5")))
			dialog.ui.DiveComputerList->setCurrentRow(row);
	int opens = 0, closes = 0;
	dialog.config->setReadTransportForTesting([&](device_data_t *) { ++opens; return QString(); },
		[&](device_data_t *) { ++closes; return DC_STATUS_IO; }, [](device_data_t *) { return true; });
	dialog.ui.connectButton->click();
	dialog.ui.retrieveDetails->click();
	QTRY_COMPARE(dialog.config->currentState, ConfigureDiveComputer::DONE);
	dialog.dc_close();
	QCOMPARE(closes, 1);
	QCOMPARE(opens, 1);
	QCOMPARE(dialog.config->currentState, ConfigureDiveComputer::ERRORED);
	QVERIFY(dialog.config->lastError.contains(QStringLiteral("did not close cleanly")));
	QVERIFY(dialog.ui.progressBar->format().contains(QStringLiteral("Error")));
	QVERIFY(dialog.ui.progressBar->toolTip().contains(QStringLiteral("did not close cleanly")));
	QVERIFY(!dialog.ui.saveSettingsPushButton->isEnabled());
	QVERIFY(!dialog.ui.backupButton->isEnabled());
}

void TestOstc4Dialog::readAndCloseErrorsRemainVisible()
{
	ConfigureDiveComputerDialog dialog(testDialogPath("ostc4-read-close-failure-test.xml"));
	for (int row = 0; row < dialog.ui.DiveComputerList->count(); ++row)
		if (dialog.ui.DiveComputerList->item(row)->text().contains(QStringLiteral("OSTC 4/5")))
			dialog.ui.DiveComputerList->setCurrentRow(row);
	int closes = 0;
	dialog.config->setReadTransportForTesting([](device_data_t *) { return QString(); },
		[&](device_data_t *) { ++closes; return DC_STATUS_IO; }, [](device_data_t *) { return false; });
	dialog.ui.connectButton->click();
	dialog.ui.retrieveDetails->click();
	QTRY_COMPARE(dialog.config->currentState, ConfigureDiveComputer::ERRORED);
	QCOMPARE(closes, 1);
	QVERIFY(dialog.config->lastError.contains(QStringLiteral("Reading settings failed")));
	QVERIFY(dialog.config->lastError.contains(QStringLiteral("did not close cleanly")));
}

void TestOstc4Dialog::modelPageFollowsLatestRead()
{
	ConfigureDiveComputerDialog dialog(testDialogPath("ostc4-model-page-test.xml"));
	dialog.device_data.product = "OSTC Plus";
	DeviceDetails ostc4;
	ostc4.model = QStringLiteral("OSTC 4");
	ostc4.ostc4Snapshot.valid = true;
	ostc4.ostc4Snapshot.identity.model = 0x43;
	dialog.deviceDetailsReceived(ostc4);
	QCOMPARE(dialog.ui.dcStackedWidget->currentIndex(), 2);
	DeviceDetails ostc3;
	ostc3.model = QStringLiteral("OSTC Plus");
	dialog.deviceDetailsReceived(ostc3);
	QCOMPARE(dialog.ui.dcStackedWidget->currentIndex(), 1);
	QVERIFY(!dialog.deviceDetails.ostc4Snapshot.valid);
}

void TestOstc4Dialog::dialogGuardsAndManualBackupError()
{
	ConfigureDiveComputerDialog dialog(testDialogPath("ostc4-guard-test.xml"));
	dialog.on_DiveComputerList_currentRowChanged(-1);
	QVERIFY(dialog.ui.openOstc4BackupFolderButton->isHidden());
	QVERIFY(!dialog.ui.connectBluetoothButton->isEnabled());
	dialog.on_DiveComputerList_currentRowChanged(dialog.ui.DiveComputerList->count());
	QVERIFY(dialog.ui.openOstc4BackupFolderButton->isHidden());
	dialog.ui.DiveComputerList->setCurrentRow(-1);
	dialog.dc_open();
	QVERIFY(dialog.ui.progressBar->format().contains(QStringLiteral("Select a dive computer")));

	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString path = directory.filePath(QStringLiteral("already.json"));
	QFile existing(path);
	QVERIFY(existing.open(QIODevice::WriteOnly));
	QCOMPARE(existing.write("original"), 8);
	existing.close();
	QString error;
	QVERIFY(!dialog.saveManualOstc4Backup(path, error));
	QVERIFY(error.contains(QStringLiteral("File already exists")) && error.contains(path));
	QVERIFY(existing.open(QIODevice::ReadOnly));
	QCOMPARE(existing.readAll(), QByteArray("original"));
	const QString invalidPath = directory.filePath(QStringLiteral("invalid.json"));
	QVERIFY(!dialog.saveManualOstc4Backup(invalidPath, error));
	QVERIFY(error.contains(QStringLiteral("fresh read")));
	QVERIFY(!QFile::exists(invalidPath));

	dialog.ui.dcStackedWidget->setCurrentIndex(2);
	dialog.deviceDetails.ostc4Snapshot.identity = { 0x43, 428, static_cast<quint16>((1 << 11) | (7 << 6) | (5 << 1)) };
	dialog.ui.dateTimeSyncCheckBox->setEnabled(true);
	dialog.ui.dateTimeSyncCheckBox->setChecked(true);
	dialog.ui.dateTimeSyncCheckBox->setChecked(false);
	QVERIFY(dialog.ui.dateTimeSyncCheckBox->isEnabled());
	QVERIFY(!dialog.device_data.sync_time);
	dialog.deviceDetails.ostc4Snapshot.identity.firmware++;
	dialog.ui.dateTimeSyncCheckBox->setChecked(true);
	QVERIFY(!dialog.ui.dateTimeSyncCheckBox->isEnabled());
}

void TestOstc4Dialog::retrieveUsesOneWorkerConnection()
{
	// Exercise the actual connect button, controller and Retrieve details slot.
	// The selected OSTC3 descriptor also covers an OSTC4 detected over Bluetooth.
	for (int outcome : { 0, 1, 2 }) {
		ConfigureDiveComputerDialog dialog(testDialogPath("ostc4-retrieve-test.xml"));
		auto *list = dialog.ui.DiveComputerList;
		int row = -1;
		for (int i = 0; i < list->count(); ++i)
			if (list->item(i)->text().contains(QStringLiteral("OSTC 4/5"))) { row = i; break; }
		QVERIFY(row >= 0);
		list->setCurrentRow(row);
		QObject *transport = nullptr;
		int opens = 0, closes = 0, operations = 0;
		dialog.config->setReadTransportForTesting([&](device_data_t *) {
			Q_ASSERT(QThread::currentThread() != QCoreApplication::instance()->thread());
			Q_ASSERT(!transport);
			transport = new QTimer;
			new QTimer(transport);
			++opens;
			if (outcome == 1) {
				// Model a no-event connection deadline, not a successful read.
				QEventLoop loop;
				QTimer::singleShot(30, &loop, &QEventLoop::quit);
				loop.exec();
			}
			return outcome == 0 ? QStringLiteral("terminal profile failure") :
				(outcome == 1 ? QStringLiteral("connection timed out") : QString());
		}, [&](device_data_t *) {
			Q_ASSERT(transport && transport->thread() == QThread::currentThread());
			delete transport;
			transport = nullptr;
			++closes;
			return DC_STATUS_SUCCESS;
		}, [&](device_data_t *) { ++operations; return true; });
		dialog.ui.connectButton->click();
		QCOMPARE(dialog.config->currentState, ConfigureDiveComputer::OPEN);
		QCOMPARE(opens, 0); // Connect only arms worker-owned Retrieve.
		QVERIFY(dialog.ui.retrieveDetails->isEnabled());
		QElapsedTimer elapsed;
		elapsed.start();
		dialog.ui.retrieveDetails->click();
		QTRY_COMPARE_WITH_TIMEOUT(dialog.config->currentState,
			outcome == 2 ? ConfigureDiveComputer::DONE : ConfigureDiveComputer::ERRORED, 3000);
		QCOMPARE(opens, 1);
		QCOMPARE(closes, outcome == 2 ? 0 : 1);
		QCOMPARE(operations, outcome == 2 ? 1 : 0);
		QCOMPARE(bool(transport), outcome == 2);
		QVERIFY(elapsed.elapsed() < 3000);
		if (outcome != 2) {
			QVERIFY(dialog.config->lastError.contains(outcome == 0 ?
				QStringLiteral("terminal profile failure") : QStringLiteral("connection timed out")));
			QVERIFY(dialog.ui.progressBar->format().contains(QStringLiteral("Error")));
		} else {
			QVERIFY(dialog.config->lastError.isEmpty());
		}
		QVERIFY(dialog.ui.retrieveDetails->isEnabled());
		QVERIFY(dialog.ui.disconnectButton->isEnabled());
		QVERIFY(!dialog.ui.saveSettingsPushButton->isEnabled());
		dialog.ui.disconnectButton->click();
		QCOMPARE(closes, 1);
		QCOMPARE(dialog.config->currentState, ConfigureDiveComputer::INITIAL);
	}
}

void TestOstc4Dialog::readWorkerCleansPartialOpen()
{
	for (int failure : { 1, 2 }) {
		device_data_t data;
		ReadSettingsThread worker(nullptr, &data);
		QObject *transport = nullptr;
		int operations = 0, closes = 0;
		worker.setTransport([&](device_data_t *) {
			Q_ASSERT(QThread::currentThread() != QCoreApplication::instance()->thread());
			transport = new QTimer;
			new QTimer(transport);
			return failure == 1 ? QStringLiteral("fake read connection error") : QString();
		}, [&](device_data_t *) {
			Q_ASSERT(transport && transport->thread() == QThread::currentThread());
			delete transport;
			transport = nullptr;
			++closes;
			return DC_STATUS_SUCCESS;
		});
		worker.setTestOperation([&](device_data_t *) {
			Q_ASSERT(transport && transport->thread() == QThread::currentThread());
			++operations;
			return false;
		});
		QSignalSpy finished(&worker, &QThread::finished);
		QSignalSpy errors(&worker, &ReadSettingsThread::error);
		worker.start();
		if (finished.isEmpty())
			QVERIFY(finished.wait(10000));
		worker.wait();
		QCOMPARE(closes, 1);
		QCOMPARE(operations, failure == 1 ? 0 : 1);
		QVERIFY(!transport);
		QCOMPARE(errors.size(), 1);
		QVERIFY(errors.first().first().toString().contains(failure == 1 ?
			QStringLiteral("fake read connection error") : QStringLiteral("Reading settings failed")));
	}
}

void TestOstc4Dialog::readThenWriteWorkersOwnTransports()
{
	device_data_t data;
	QObject *transport = nullptr;
	int opens = 0, closes = 0, operations = 0;
	auto open = [&](device_data_t *actual) {
		Q_ASSERT(actual == &data);
		Q_ASSERT(QThread::currentThread() != QCoreApplication::instance()->thread());
		Q_ASSERT(!transport);
		transport = new QTimer;
		new QTimer(transport);
		++opens;
		return QString();
	};
	auto close = [&](device_data_t *actual) {
		Q_ASSERT(actual == &data);
		Q_ASSERT(transport && transport->thread() == QThread::currentThread());
		delete transport;
		transport = nullptr;
		++closes;
		return DC_STATUS_SUCCESS;
	};
	auto operation = [&](device_data_t *actual) {
		Q_ASSERT(actual == &data);
		Q_ASSERT(transport && transport->thread() == QThread::currentThread());
		++operations;
		return true;
	};
	ReadSettingsThread read(nullptr, &data);
	read.setTransport(open, close);
	read.setTestOperation(operation);
	QSignalSpy readFinished(&read, &QThread::finished);
	QSignalSpy readErrors(&read, &ReadSettingsThread::error);
	read.start();
	if (readFinished.isEmpty())
		QVERIFY(readFinished.wait(10000));
	read.wait();
	QCOMPARE(readErrors.size(), 0);
	QCOMPARE(opens, 1);
	QCOMPARE(closes, 1);
	QCOMPARE(operations, 1);
	QVERIFY(!transport);

	WriteSettingsThread write(nullptr, &data);
	write.setTransport(open, close);
	write.setTestOperation(operation);
	QSignalSpy writeFinished(&write, &QThread::finished);
	QSignalSpy writeErrors(&write, &WriteSettingsThread::error);
	write.start();
	if (writeFinished.isEmpty())
		QVERIFY(writeFinished.wait(10000));
	write.wait();
	QCOMPARE(writeErrors.size(), 0);
	QCOMPARE(opens, 2);
	QCOMPARE(closes, 2);
	QCOMPARE(operations, 2);
	QVERIFY(!transport);
}

void TestOstc4Dialog::writeWorkerOwnsTransport()
{
	// Exercise the real QThread::run lifecycle without Bluetooth or device writes.
	// Even a partially opened transport must be destroyed on its creator thread.
	for (int failure : { 0, 1, 2 }) {
		device_data_t data;
		WriteSettingsThread worker(nullptr, &data);
		QSignalSpy finished(&worker, &QThread::finished);
		QSignalSpy errors(&worker, &WriteSettingsThread::error);
		QObject *transport = nullptr;
		int opens = 0, operations = 0, closes = 0;
		worker.setTransport([&](device_data_t *actual) {
			Q_ASSERT(actual == &data);
			Q_ASSERT(QThread::currentThread() != QCoreApplication::instance()->thread());
			++opens;
			transport = new QTimer; // Qt child and event machinery belong to this thread.
			new QTimer(transport);
			return failure == 1 ? QStringLiteral("fake open error") : QString();
		}, [&](device_data_t *actual) {
			Q_ASSERT(actual == &data);
			Q_ASSERT(transport && transport->thread() == QThread::currentThread());
			++closes;
			delete transport;
			transport = nullptr;
			return DC_STATUS_SUCCESS;
		});
		worker.setTestOperation([&](device_data_t *) {
			Q_ASSERT(transport && transport->thread() == QThread::currentThread());
			++operations;
			return failure != 2;
		});
		worker.start();
		if (finished.isEmpty())
			QVERIFY(finished.wait(10000));
		worker.wait();
		QCOMPARE(opens, 1);
		QCOMPARE(operations, failure == 1 ? 0 : 1);
		QCOMPARE(closes, 1);
		QCOMPARE(transport, nullptr);
		QCOMPARE(errors.size(), failure ? 1 : 0);
		if (failure == 1)
			QVERIFY(errors.first().first().toString().contains(QStringLiteral("Connection failed before settings")));
		if (failure == 2)
			QVERIFY(errors.first().first().toString().contains(QStringLiteral("may have taken effect")));
	}
}

void TestOstc4Dialog::continuousSaveSession()
{
	for (int scenario = 0; scenario < 4; ++scenario) {
		const bool failWrite = scenario == 1 || scenario == 3;
		const bool failClose = scenario >= 2;
		ConfigureDiveComputerDialog dialog(testDialogPath("ostc4-session-test.xml"));
		int row = -1;
		for (int i = 0; i < dialog.ui.DiveComputerList->count(); ++i)
			if (dialog.ui.DiveComputerList->item(i)->text().contains(QStringLiteral("OSTC 4/5"))) { row = i; break; }
		QVERIFY(row >= 0);
		dialog.ui.DiveComputerList->setCurrentRow(row);
		QObject *transport = nullptr;
		QThread *owner = nullptr;
		int opens = 0, closes = 0, reads = 0, writes = 0;
		bool completionBeforeClose = false;
		QObject::connect(dialog.config, &ConfigureDiveComputer::stateChanged, &dialog,
			[&](ConfigureDiveComputer::states oldState, ConfigureDiveComputer::states newState) {
				if (oldState == ConfigureDiveComputer::WRITING &&
				    (newState == ConfigureDiveComputer::DONE || newState == ConfigureDiveComputer::ERRORED) && closes == 0)
					completionBeforeClose = true;
			});
		std::atomic<int> idleEvents { 0 };
		DeviceDetails details;
		details.model = QStringLiteral("OSTC 4");
		details.serialNo = QStringLiteral("428");
		details.ostc4Snapshot.valid = true;
		details.ostc4Snapshot.identity.model = 0x43;
		details.ostc4Snapshot.identity.serial = 428;
		dialog.config->setReadDetailsForTesting(details);
		dialog.config->setReadTransportForTesting([&](device_data_t *) {
			Q_ASSERT(!transport && QThread::currentThread() != qApp->thread());
			owner = QThread::currentThread();
			transport = new QTimer;
			auto *idleTimer = new QTimer(transport);
			idleTimer->setSingleShot(true);
			QObject::connect(idleTimer, &QTimer::timeout, idleTimer, [&]() { ++idleEvents; });
			idleTimer->start(10);
			++opens;
			return QString();
		}, [&](device_data_t *) {
			Q_ASSERT(transport && QThread::currentThread() == owner && transport->thread() == owner);
			if (writes == 1 && closes == 0) {
				// Model a deferred socket-disconnect callback on its owning thread.
				QEventLoop disconnectLoop;
				QTimer::singleShot(20, &disconnectLoop, &QEventLoop::quit);
				disconnectLoop.exec();
			}
			delete transport;
			transport = nullptr;
			++closes;
			return failClose && closes == 1 ? DC_STATUS_IO : DC_STATUS_SUCCESS;
		}, [&](device_data_t *) {
			Q_ASSERT(transport && transport->thread() == QThread::currentThread());
			if (reads == 0) { ++reads; return true; }
			if (writes == 0) { ++writes; return !failWrite; }
			++reads;
			return true;
		});
		dialog.ui.connectButton->click();
		dialog.ui.retrieveDetails->click();
		QTRY_COMPARE(dialog.config->currentState, ConfigureDiveComputer::DONE);
		QCOMPARE(opens, 1);
		QCOMPARE(closes, 0);
		QVERIFY(transport);
		QTRY_COMPARE(idleEvents.load(), 1); // Qt socket events must run while the editor is open.
		// Exercise controller save (the UI's file chooser is deliberately not opened).
		// The same transport must remain live through the edit interval and save operation.
		dialog.config->saveDeviceDetails(details, &dialog.device_data);
		QTRY_COMPARE(dialog.config->currentState, failWrite || failClose ? ConfigureDiveComputer::ERRORED : ConfigureDiveComputer::DONE);
		QCOMPARE(opens, 1);
		QCOMPARE(writes, 1);
		QCOMPARE(closes, 1);
		QVERIFY(!completionBeforeClose);
		QVERIFY(!transport);
		if (failWrite)
			QVERIFY(dialog.config->lastError.contains(QStringLiteral("may have taken effect")));
		if (failClose)
			QVERIFY(dialog.config->lastError.contains(QStringLiteral("did not close cleanly")));
		if (!failWrite && !failClose)
			QVERIFY(dialog.config->lastError.isEmpty());
		QVERIFY(!dialog.ui.retrieveDetails->isEnabled());
		QVERIFY(!dialog.ui.disconnectButton->isEnabled());
		QVERIFY(!dialog.ui.page_ostc4->isEnabled());
		QVERIFY(!dialog.ui.saveSettingsPushButton->isEnabled());
		QVERIFY(!dialog.ui.resetButton_4->isEnabled());
		QVERIFY(!dialog.ui.backupButton->isEnabled());
		QVERIFY(!dialog.ui.restoreBackupButton->isEnabled());
		QVERIFY(!dialog.ui.dateTimeSyncCheckBox->isEnabled());
		QVERIFY(dialog.ui.connectButton->isEnabled());
		QVERIFY(dialog.ui.DiveComputerList->isEnabled());
		QVERIFY(dialog.ui.progressBar->toolTip().contains(QStringLiteral("Re-enable Bluetooth")));
		QVERIFY(dialog.ui.progressBar->toolTip().contains(QStringLiteral("Connect and Retrieve")));
		if (failWrite || failClose)
			QVERIFY(dialog.ui.progressBar->format().contains(QStringLiteral("Error")));
		// Reconnecting explicitly arms a new worker session, without reopening
		// the closed worker's transport from the GUI thread.
		dialog.ui.connectButton->click();
		QCOMPARE(dialog.config->currentState, ConfigureDiveComputer::OPEN);
		QCOMPARE(opens, 1);
		QVERIFY(dialog.ostc4Status.isEmpty());
		QVERIFY(dialog.ui.progressBar->toolTip().isEmpty());
		QVERIFY(dialog.ui.progressBar->accessibleDescription().isEmpty());
		dialog.ui.progressBar->resize(75, dialog.ui.progressBar->height());
		QCOMPARE(dialog.ui.progressBar->format(), QStringLiteral("Ready to retrieve details"));
		QVERIFY(dialog.ui.retrieveDetails->isEnabled());
		QVERIFY(!dialog.ui.page_ostc4->isEnabled());
		dialog.ui.retrieveDetails->click();
		if (failClose) {
			QCOMPARE(dialog.config->currentState, ConfigureDiveComputer::ERRORED);
			QCOMPARE(opens, 1); // Do not open another session after a failed close.
			QVERIFY(dialog.config->lastError.contains(QStringLiteral("did not close cleanly")));
			QVERIFY(!dialog.ui.retrieveDetails->isEnabled());
			QVERIFY(dialog.dc_close());
			dialog.ui.connectButton->click();
			dialog.ui.retrieveDetails->click(); // A fresh logical connection is required.
		}
		QTRY_COMPARE(dialog.config->currentState, ConfigureDiveComputer::DONE);
		QCOMPARE(opens, 2);
		QCOMPARE(reads, 2);
		QCOMPARE(closes, 1);
		QVERIFY(transport && owner == transport->thread());
		// Repeated reads close the previous worker before opening the next.
		dialog.ui.retrieveDetails->click();
		QTRY_COMPARE(dialog.config->currentState, ConfigureDiveComputer::DONE);
		QCOMPARE(opens, 3);
		QCOMPARE(reads, 3);
		QCOMPARE(closes, 2);
		dialog.dc_close();
		QCOMPARE(closes, 3);
		QVERIFY(dialog.ostc4Status.isEmpty());
		QVERIFY(dialog.ui.progressBar->toolTip().isEmpty());
		QVERIFY(dialog.ui.progressBar->accessibleDescription().isEmpty());
		dialog.ui.progressBar->resize(100, dialog.ui.progressBar->height());
		QCOMPARE(dialog.ui.progressBar->format(), QStringLiteral("Disconnected from device"));
	}
}

void TestOstc4Dialog::transactionThroughSession()
{
	using Transaction = Ostc4SettingsTransaction;
	for (int scenario = 0; scenario < 7; ++scenario) {
		// No-op, verified write, read-back mismatch, stale baseline, existing file,
		// identity change, or automatic backup directory failure.
		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		const QString backupPath = directory.filePath(QStringLiteral("original.json"));
		const QString autoDir = directory.filePath(QStringLiteral("ostc4-backups"));
		Transaction::Identity identity { 0x43, 428, static_cast<quint16>((1 << 11) | (7 << 6) | (6 << 1) | 1) };
		std::map<quint8, QByteArray> device;
		for (int id = 0x10; id <= 0x44; ++id)
			device[id] = QByteArray::fromHex("00000000");
		for (int id = 0x1a; id <= 0x1e; ++id)
			device[id] = QByteArray::fromHex("784400a5");
		device[0x25] = QByteArray::fromHex("1e000000");
		device[0x26] = QByteArray::fromHex("55000000");
		Transaction snapshot;
		QVERIFY(snapshot.capture([&](Transaction::Identity &out) { out = identity; return true; },
			[&](quint8 id, QByteArray &out) { out = device.at(id); return true; }));
		QVERIFY(snapshot.valid);
		DeviceDetails details;
		details.model = QStringLiteral("OSTC 4");
		details.serialNo = QStringLiteral("428");
		details.firmwareVersion = Transaction::firmwareString(identity.firmware);
		details.ostc4Snapshot = snapshot;
		details.sp1.sp = details.sp2.sp = details.sp3.sp = details.sp4.sp = details.sp5.sp = 120;
		details.sp1.depth = details.sp2.depth = details.sp3.depth = details.sp4.depth = details.sp5.depth = 68;
		details.gfLow = 30;
		details.gfHigh = 85;
		int opens = 0, closes = 0, reads = 0, writes = 0, identifies = 0;
		QThread *owner = nullptr;
		QObject *transport = nullptr;
		bool written = false;
		bool backupValidatedBeforeWrite = false;
		const QByteArray original = device[0x1d];
		auto io = std::make_shared<Ostc4SettingsIO>();
		io->identify = [&](Transaction::Identity &out) {
			Q_ASSERT(transport && QThread::currentThread() == owner);
			++identifies;
			out = identity;
			return true;
		};
		io->read = [&](quint8 id, QByteArray &out) {
			Q_ASSERT(transport && QThread::currentThread() == owner);
			++reads;
			out = scenario == 2 && written && id == 0x1d ? original : device.at(id);
			return true;
		};
		io->write = [&](quint8 id, const QByteArray &value) {
			Q_ASSERT(transport && QThread::currentThread() == owner);
			QString description;
			const QString actualPath = scenario == 1 ?
				QDir(autoDir).entryInfoList(QStringList { QStringLiteral("*.json") }, QDir::Files).first().absoluteFilePath() : backupPath;
			backupValidatedBeforeWrite = QFile::exists(actualPath) &&
				Transaction::validateBackup(actualPath, snapshot.identity, description);
			if (!backupValidatedBeforeWrite)
				return false;
			++writes;
			written = true;
			device[id] = value;
			return true;
		};
		ConfigureDiveComputerDialog dialog(testDialogPath("ostc4-transaction-test.xml"));
		int row = -1;
		for (int i = 0; i < dialog.ui.DiveComputerList->count(); ++i)
			if (dialog.ui.DiveComputerList->item(i)->text().contains(QStringLiteral("OSTC 4/5"))) { row = i; break; }
		QVERIFY(row >= 0);
		dialog.ui.DiveComputerList->setCurrentRow(row);
		if (scenario == 1) {
			QCOMPARE(QFileInfo(dialog.newOstc4BackupPath()).absolutePath(),
				QDir(QString::fromStdString(system_default_directory())).filePath(QStringLiteral("ostc4-backups")));
		}
		dialog.ostc4BackupDirectory = autoDir;
		if (scenario == 1)
			QVERIFY(dialog.newOstc4BackupPath() != dialog.newOstc4BackupPath());
		QVERIFY(!dialog.ui.openOstc4BackupFolderButton->isHidden());
		dialog.config->setReadDetailsForTesting(details);
		dialog.config->setOstc4IOForTesting(io);
		dialog.config->setReadTransportForTesting([&](device_data_t *) {
			Q_ASSERT(!transport && QThread::currentThread() != qApp->thread());
			owner = QThread::currentThread();
			transport = new QTimer;
			++opens;
			return QString();
		}, [&](device_data_t *) {
			Q_ASSERT(transport && transport->thread() == QThread::currentThread() && owner == QThread::currentThread());
			delete transport;
			transport = nullptr;
			++closes;
			return DC_STATUS_SUCCESS;
		}, [&](device_data_t *) { return true; }); // Mock read only; save runs writer.perform().
		dialog.ui.connectButton->click();
		dialog.ui.retrieveDetails->click();
		QTRY_COMPARE(dialog.config->currentState, ConfigureDiveComputer::DONE);
		QCOMPARE(opens, 1);
		QCOMPARE(closes, 0);
		DeviceDetails edited = details;
		if (scenario != 0)
			edited.sp4.sp = 121;
		if (scenario == 3)
			device[0x1d][0] = 119;
		if (scenario == 5)
			identity.serial++;
		if (scenario == 4) {
			QFile existing(backupPath);
			QVERIFY(existing.open(QIODevice::WriteOnly));
			QVERIFY(existing.write("existing") == 8);
		}
		if (scenario == 6) {
			QFile blocked(autoDir);
			QVERIFY(blocked.open(QIODevice::WriteOnly));
		}
		QSignalSpy messages(dialog.config, &ConfigureDiveComputer::message);
		QSignalSpy errors(dialog.config, &ConfigureDiveComputer::error);
		if (scenario == 0 || scenario == 1 || scenario == 6) {
			if (scenario != 0)
				dialog.ui.ostc4SetPointTable->item(3, 1)->setText(QStringLiteral("121"));
			dialog.ui.saveSettingsPushButton->click(); // No modal file picker.
		} else {
			dialog.config->setOstc4BackupPath(backupPath);
			dialog.config->saveDeviceDetails(edited, &dialog.device_data);
		}
		QTRY_COMPARE(dialog.config->currentState,
			(scenario <= 1 ? ConfigureDiveComputer::DONE : ConfigureDiveComputer::ERRORED));
		QCOMPARE(opens, 1);
		QCOMPARE(closes, 1);
		QVERIFY(!transport);
		QCOMPARE(identifies, 1);
		QCOMPARE(reads, scenario == 5 ? 0 : (scenario == 3 ? 4 : 7 + (scenario == 1 || scenario == 2 ? 1 : 0)));
		QCOMPARE(writes, scenario == 1 || scenario == 2 ? 1 : 0);
		QCOMPARE(backupValidatedBeforeWrite, scenario == 1 || scenario == 2);
		QVERIFY(!dialog.ui.retrieveDetails->isEnabled());
		QVERIFY(!dialog.ui.page_ostc4->isEnabled());
		QVERIFY(dialog.ui.connectButton->isEnabled());
		QVERIFY(dialog.ui.progressBar->toolTip().contains(QStringLiteral("Re-enable Bluetooth")));
		if (scenario == 0) {
			QVERIFY(!QFile::exists(backupPath));
			QVERIFY(!messages.isEmpty() && messages.last().first().toString().contains(QStringLiteral("zero writes")));
			QVERIFY(dialog.ui.progressBar->toolTip().contains(QStringLiteral("zero writes")));
		} else if (scenario == 1) {
			const QFileInfoList backups = QDir(autoDir).entryInfoList(QStringList { QStringLiteral("OSTC4-settings-*.json") }, QDir::Files);
			QCOMPARE(backups.size(), 1);
			const QString savedPath = backups.first().absoluteFilePath();
			QVERIFY(!QFile::exists(backupPath));
			QVERIFY(messages.last().first().toString().contains(savedPath));
			QVERIFY(dialog.ui.progressBar->toolTip().contains(savedPath));
			QCOMPARE(dialog.ui.progressBar->accessibleDescription(), dialog.ui.progressBar->toolTip());
			QVERIFY(dialog.ui.progressBar->format().contains(QChar(0x2026)));
			QVERIFY(!dialog.ui.progressBar->format().contains(savedPath));
			QCOMPARE(dialog.ui.openOstc4BackupFolderButton->text(), QStringLiteral("Open OSTC 4/5 backup folder"));
			openedUrl = QUrl();
			{
				FileUrlHandlerGuard handler(this);
				dialog.ui.openOstc4BackupFolderButton->click();
			}
			QCOMPARE(openedUrl, QUrl::fromLocalFile(autoDir));
#ifndef Q_OS_WIN
			QCOMPARE(QFileInfo(autoDir).permissions() & QFileDevice::ReadGroup, QFileDevice::Permissions());
			QCOMPARE(backups.first().permissions() & QFileDevice::ReadGroup, QFileDevice::Permissions());
			QCOMPARE(backups.first().permissions() & QFileDevice::ReadOther, QFileDevice::Permissions());
#endif
			QCOMPARE(device[0x1d], QByteArray::fromHex("794400a5"));
			QVERIFY(!messages.isEmpty() && messages.last().first().toString().contains(QStringLiteral("verified by read-back")));
			QVERIFY(dialog.ui.progressBar->toolTip().contains(QStringLiteral("verified by read-back")));
		} else {
			QVERIFY(!errors.isEmpty());
			QVERIFY(dialog.config->lastError.contains(scenario == 2 ? QStringLiteral("Read-back") :
				(scenario == 3 ? QStringLiteral("changed since") :
				(scenario == 4 || scenario == 6 ? QStringLiteral("Backup cancelled or failed") : QStringLiteral("identity/firmware differs")))));
			if (scenario == 6) {
				QVERIFY(dialog.config->lastError.contains(autoDir));
				QVERIFY(dialog.config->lastError.contains(QStringLiteral("Could not create")));
			}
			QCOMPARE(QFile::exists(backupPath), scenario == 2 || scenario == 4);
			if (scenario == 2)
				QVERIFY(dialog.config->lastError.contains(backupPath));
			QVERIFY(dialog.ui.progressBar->toolTip().contains(QStringLiteral("Error")));
			if (scenario == 4) {
				QFile existing(backupPath);
				QVERIFY(existing.open(QIODevice::ReadOnly));
				QCOMPARE(existing.readAll(), QByteArray("existing"));
			}
		}
		dialog.dc_close();
		QCOMPARE(closes, 1);
	}
}

void TestOstc4Dialog::liveSessionDestructor()
{
	QObject *transport = nullptr;
	QThread *owner = nullptr;
	int opens = 0, closes = 0;
	QElapsedTimer timer;
	{
		ConfigureDiveComputerDialog dialog(testDialogPath("ostc4-destruction-test.xml"));
		int row = -1;
		for (int i = 0; i < dialog.ui.DiveComputerList->count(); ++i)
			if (dialog.ui.DiveComputerList->item(i)->text().contains(QStringLiteral("OSTC 4/5"))) { row = i; break; }
		QVERIFY(row >= 0);
		dialog.ui.DiveComputerList->setCurrentRow(row);
		dialog.config->setReadTransportForTesting([&](device_data_t *) {
			Q_ASSERT(!transport && QThread::currentThread() != qApp->thread());
			owner = QThread::currentThread();
			transport = new QTimer;
			++opens;
			return QString();
		}, [&](device_data_t *) {
			Q_ASSERT(transport && transport->thread() == owner && QThread::currentThread() == owner);
			delete transport;
			transport = nullptr;
			++closes;
			return DC_STATUS_SUCCESS;
		}, [&](device_data_t *) { return true; });
		dialog.ui.connectButton->click();
		dialog.ui.retrieveDetails->click();
		QTRY_COMPARE(dialog.config->currentState, ConfigureDiveComputer::DONE);
		QCOMPARE(opens, 1);
		QCOMPARE(closes, 0);
		QVERIFY(transport);
		timer.start();
	} // Destruct a live idle session without clicking Disconnect.
	QCOMPARE(closes, 1);
	QVERIFY(!transport);
	QVERIFY(timer.elapsed() < 3000);
}

void TestOstc4Dialog::hiddenTabsAndReread()
{
	// 0x60 hardware 003b000043, 0x69 serial 428 and stable 1.7.5.0.
	QByteArray hardware = QByteArray::fromHex("003b000043");
	QByteArray version(64, '\0');
	version[0] = static_cast<char>(428 & 0xff);
	version[1] = static_cast<char>(428 >> 8);
	const quint16 firmware = (1 << 11) | (7 << 6) | (5 << 1);
	version[2] = static_cast<char>(firmware & 0xff);
	version[3] = static_cast<char>(firmware >> 8);
	Ostc4SettingsTransaction::Identity identity;
	QVERIFY(Ostc4SettingsTransaction::decodeIdentity(hardware, version, identity));
	DeviceDetails details;
	details.model = QStringLiteral("OSTC 4");
	details.serialNo = QStringLiteral("428");
	details.firmwareVersion = Ostc4SettingsTransaction::firmwareString(identity.firmware);
	// 0x72 returns a four-byte payload for every setting except the simulated
	// failed 0x20/0x10 replies; completion alone does not make them editable.
	auto capture = [&](bool fail) {
		return details.ostc4Snapshot.capture([&](auto &out) { out = identity; return true; },
			[&](quint8 id, QByteArray &out) {
				if (fail && (id == 0x20 || id == 0x10)) return false;
				out = QByteArray::fromHex("00000000");
				if (id == 0x2d) out[0] = 1;
				if (id == 0x1a || id == 0x1b || id == 0x1c || id == 0x1d || id == 0x1e)
					out[0] = 120;
				return true;
			});
	};
	QVERIFY(capture(false));
	details.ostc4Snapshot.originalCustomText = QByteArray(60, '\0');
	details.brightness = 1;
	details.sp1.sp = details.sp2.sp = details.sp3.sp = details.sp4.sp = details.sp5.sp = 120;
	ConfigureDiveComputerDialog dialog(testDialogPath("ostc4-dialog-test.xml"));
	auto *tabs = dialog.findChild<QTabWidget *>(QStringLiteral("tabWidget4"));
	auto *stack = dialog.findChild<QStackedWidget *>(QStringLiteral("dcStackedWidget"));
	auto *brightness = dialog.findChild<QComboBox *>(QStringLiteral("brightnessComboBox_4"));
	auto *diveMode = dialog.findChild<QComboBox *>(QStringLiteral("diveModeComboBox_4"));
	auto *setpoints = dialog.findChild<QTableWidget *>(QStringLiteral("ostc4SetPointTable"));
	auto *gases = dialog.findChild<QTableWidget *>(QStringLiteral("ostc4GasTable"));
	auto *progress = dialog.findChild<QProgressBar *>(QStringLiteral("progressBar"));
	auto *customText = dialog.findChild<QLineEdit *>(QStringLiteral("customTextLlineEdit_4"));
	auto *clock = dialog.findChild<QCheckBox *>(QStringLiteral("dateTimeSyncCheckBox"));
	QVERIFY(tabs && stack && brightness && diveMode && setpoints && gases && progress && customText && clock);
	QCOMPARE(stack->currentIndex(), 0);
	QVERIFY(!brightness->isVisible()); // neither the dialog nor this tab is visible at completion
	dialog.deviceDetailsReceived(details);
	QCOMPARE(stack->currentIndex(), 2);
	QVERIFY(!brightness->isHidden());
	QVERIFY(brightness->isEnabled());
	QVERIFY(dialog.deviceDetails.ostc4Editable[0x2d - 0x10]);
	QVERIFY(dialog.deviceDetails.ostc4Editable[0x20 - 0x10]);
	QVERIFY(customText->isEnabled() && clock->isEnabled());
	QVERIFY(!(dialog.deviceDetails.ostc4Editable[0x36 - 0x10])); // unsafe legacy encoding
	QVERIFY(progress->toolTip().startsWith(QStringLiteral("OSTC 4 1.7.5: settings read; available controls can be edited.")));
	QVERIFY(!progress->format().contains(QStringLiteral("compatibility")));
	QCOMPARE(progress->accessibleDescription(), progress->toolTip());
	QVERIFY(progress->toolTip().contains(QStringLiteral("Unsafe legacy encodings remain disabled")));
	QVERIFY(progress->toolTip().contains(QStringLiteral("clock sync is not backed up or read-back verified")));
	QVERIFY(progress->toolTip().contains(QStringLiteral("do not verify persistence")));
	dialog.show();
	QCoreApplication::processEvents();
	progress->resize(75, progress->height());
	QVERIFY(progress->format().contains(QChar(0x2026)));
	QVERIFY(progress->fontMetrics().horizontalAdvance(progress->format()) <= progress->width());
	QVERIFY(progress->toolTip().contains(QStringLiteral("available controls can be edited")));
	for (QWidget *page = brightness; page && page != tabs; page = page->parentWidget())
		if (tabs->indexOf(page) >= 0) tabs->setCurrentWidget(page);
	QVERIFY(brightness->isVisible() && brightness->isEnabled());
	QVERIFY(dialog.deviceDetails.ostc4Editable[0x2d - 0x10]);
	for (QWidget *page = diveMode; page && page != tabs; page = page->parentWidget())
		if (tabs->indexOf(page) >= 0) tabs->setCurrentWidget(page);
	QVERIFY(diveMode->isVisible() && diveMode->isEnabled());
	QVERIFY(dialog.deviceDetails.ostc4Editable[0x20 - 0x10]);
	QVERIFY(setpoints->item(0, 1)->flags() & Qt::ItemIsEditable);

	QVERIFY(capture(true));
	dialog.deviceDetailsReceived(details);
	QVERIFY(!dialog.deviceDetails.ostc4Editable[0x20 - 0x10]);
	QVERIFY(!diveMode->isEnabled());
	QVERIFY(!dialog.deviceDetails.ostc4Editable[0]);
	QVERIFY(!gases->item(0, 1)->flags().testFlag(Qt::ItemIsEditable));
	QVERIFY(gases->item(1, 1)->flags().testFlag(Qt::ItemIsEditable));
	QVERIFY(!customText->isEnabled()); // No new 60-byte version reply.
	QVERIFY(dialog.deviceDetails.ostc4Editable[0x2d - 0x10]);
	dialog.processStateChanged(ConfigureDiveComputer::DONE, ConfigureDiveComputer::READING);
	QVERIFY(!dialog.deviceDetails.ostc4Snapshot.valid);
	QVERIFY(!dialog.deviceDetails.ostc4Editable[0x2d - 0x10]);
	QVERIFY(!dialog.ui.page_ostc4->isEnabled());
	dialog.deviceDetailsReceived(details); // Successful completion re-enables only the new snapshot.
	QVERIFY(dialog.ui.page_ostc4->isEnabled());
	// Gas-only replies must still produce a successful-read status.
	QVERIFY(details.ostc4Snapshot.capture([&](auto &out) { out = identity; return true; },
		[](quint8 id, QByteArray &out) {
			if (id != 0x10) return false;
			out = QByteArray::fromHex("15000000");
			return true;
		}));
	dialog.deviceDetailsReceived(details);
	QVERIFY(dialog.deviceDetails.ostc4Editable[0]);
	QVERIFY(progress->toolTip().startsWith(QStringLiteral("OSTC 4 1.7.5: settings read; available controls can be edited.")));
	// A failed or unsupported subsequent read may not retain the previous map.
	identity.firmware = 0;
	QVERIFY(capture(false));
	dialog.deviceDetailsReceived(details);
	QVERIFY(!dialog.deviceDetails.ostc4Editable[0x2d - 0x10]);
	QVERIFY(!brightness->isEnabled());
	QVERIFY(!setpoints->item(0, 1)->flags().testFlag(Qt::ItemIsEditable));
	QVERIFY(progress->toolTip().contains(QStringLiteral("unsupported or unverified firmware; settings disabled")));
	dialog.processStateChanged(ConfigureDiveComputer::DONE, ConfigureDiveComputer::WRITING);
	QVERIFY(!dialog.ui.page_ostc4->isEnabled());
	dialog.processStateChanged(ConfigureDiveComputer::WRITING, ConfigureDiveComputer::DONE);
	QVERIFY(!dialog.deviceDetails.ostc4Snapshot.valid);
	QVERIFY(!dialog.ui.page_ostc4->isEnabled());
	QVERIFY(!dialog.ui.retrieveDetails->isEnabled()); // Connect is required after save teardown.
	QVERIFY(dialog.ui.connectButton->isEnabled());
	dialog.configError(QStringLiteral("Read failed"));
	QVERIFY(progress->toolTip().contains(QStringLiteral("Read failed")));
	QCOMPARE(progress->accessibleDescription(), progress->toolTip());
}

void TestOstc4Dialog::writeCompletionWithoutSelection()
{
	ConfigureDiveComputerDialog dialog(testDialogPath("ostc4-write-no-selection-test.xml"));
	dialog.ui.dcStackedWidget->setCurrentIndex(2);
	dialog.ui.DiveComputerList->setCurrentRow(-1);
	QCOMPARE(dialog.ui.DiveComputerList->currentRow(), -1);
	dialog.processStateChanged(ConfigureDiveComputer::WRITING, ConfigureDiveComputer::DONE);
	QVERIFY(dialog.ui.connectButton->isEnabled());
	QVERIFY(!dialog.ui.connectBluetoothButton->isEnabled());
}

QTEST_MAIN(TestOstc4Dialog)
