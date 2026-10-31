// SPDX-License-Identifier: GPL-2.0
#include "testostc4dialog.h"
#include "desktop-widgets/configuredivecomputerdialog.h"
#include "core/qtserialbluetooth.h"
#include <QBluetoothSocket>
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
#include <map>
#include <atomic>

void TestOstc4Dialog::rfcommDisconnectWait()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
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
	ConfigureDiveComputerDialog dialog(QStringLiteral("/tmp/ostc4-gf-test.xml"));
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

void TestOstc4Dialog::retrieveUsesOneWorkerConnection()
{
	// Exercise the actual connect button, controller and Retrieve details slot.
	// The selected OSTC3 descriptor also covers an OSTC4 detected over Bluetooth.
	for (int outcome : { 0, 1, 2 }) {
		ConfigureDiveComputerDialog dialog(QStringLiteral("/tmp/ostc4-retrieve-test.xml"));
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
		ConfigureDiveComputerDialog dialog(QStringLiteral("/tmp/ostc4-session-test.xml"));
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
		QVERIFY(dialog.ui.progressBar->format().contains(QStringLiteral("Re-enable Bluetooth")));
		QVERIFY(dialog.ui.progressBar->format().contains(QStringLiteral("Connect and Retrieve")));
		if (failWrite || failClose)
			QVERIFY(dialog.ui.progressBar->format().contains(QStringLiteral("Error")));
		// Reconnecting explicitly arms a new worker session, without reopening
		// the closed worker's transport from the GUI thread.
		dialog.ui.connectButton->click();
		QCOMPARE(dialog.config->currentState, ConfigureDiveComputer::OPEN);
		QCOMPARE(opens, 1);
		QVERIFY(dialog.ui.retrieveDetails->isEnabled());
		QVERIFY(!dialog.ui.page_ostc4->isEnabled());
		dialog.ui.retrieveDetails->click();
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
	}
}

void TestOstc4Dialog::transactionThroughSession()
{
	using Transaction = Ostc4SettingsTransaction;
	for (int scenario = 0; scenario < 6; ++scenario) {
		// No-op, verified write, read-back mismatch, stale baseline, backup failure, identity change.
		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		const QString backupPath = directory.filePath(QStringLiteral("original.json"));
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
			backupValidatedBeforeWrite = QFile::exists(backupPath) &&
				Transaction::validateBackup(backupPath, snapshot.identity, description);
			if (!backupValidatedBeforeWrite)
				return false;
			++writes;
			written = true;
			device[id] = value;
			return true;
		};
		ConfigureDiveComputerDialog dialog(QStringLiteral("/tmp/ostc4-transaction-test.xml"));
		int row = -1;
		for (int i = 0; i < dialog.ui.DiveComputerList->count(); ++i)
			if (dialog.ui.DiveComputerList->item(i)->text().contains(QStringLiteral("OSTC 4/5"))) { row = i; break; }
		QVERIFY(row >= 0);
		dialog.ui.DiveComputerList->setCurrentRow(row);
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
		dialog.config->setOstc4BackupPath(backupPath);
		QSignalSpy messages(dialog.config, &ConfigureDiveComputer::message);
		QSignalSpy errors(dialog.config, &ConfigureDiveComputer::error);
		dialog.config->saveDeviceDetails(edited, &dialog.device_data);
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
		QVERIFY(dialog.ui.progressBar->format().contains(QStringLiteral("Re-enable Bluetooth")));
		if (scenario == 0) {
			QVERIFY(!QFile::exists(backupPath));
			QVERIFY(!messages.isEmpty() && messages.last().first().toString().contains(QStringLiteral("zero writes")));
			QVERIFY(dialog.ui.progressBar->format().contains(QStringLiteral("zero writes")));
		} else if (scenario == 1) {
			QVERIFY(QFile::exists(backupPath));
			QCOMPARE(device[0x1d], QByteArray::fromHex("794400a5"));
			QVERIFY(!messages.isEmpty() && messages.last().first().toString().contains(QStringLiteral("verified by read-back")));
			QVERIFY(dialog.ui.progressBar->format().contains(QStringLiteral("verified by read-back")));
		} else {
			QVERIFY(!errors.isEmpty());
			QVERIFY(dialog.config->lastError.contains(scenario == 2 ? QStringLiteral("Read-back") :
				(scenario == 3 ? QStringLiteral("changed since") :
				(scenario == 4 ? QStringLiteral("Backup cancelled or failed") : QStringLiteral("identity/firmware differs")))));
			QCOMPARE(QFile::exists(backupPath), scenario == 2 || scenario == 4);
			if (scenario == 2)
				QVERIFY(dialog.config->lastError.contains(backupPath));
			QVERIFY(dialog.ui.progressBar->format().contains(QStringLiteral("Error")));
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
		ConfigureDiveComputerDialog dialog(QStringLiteral("/tmp/ostc4-destruction-test.xml"));
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
	ConfigureDiveComputerDialog dialog(QStringLiteral("/tmp/ostc4-dialog-test.xml"));
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
	QCOMPARE(progress->format(), QStringLiteral("OSTC 4 1.7.5: settings read; available controls can be edited."));
	QVERIFY(!progress->format().contains(QStringLiteral("compatibility")));
	QVERIFY(progress->toolTip().contains(QStringLiteral("Unsafe legacy encodings remain disabled")));
	QVERIFY(progress->toolTip().contains(QStringLiteral("clock sync is not backed up or read-back verified")));
	QVERIFY(progress->toolTip().contains(QStringLiteral("do not verify persistence")));
	dialog.show();
	QCoreApplication::processEvents();
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
	QCOMPARE(progress->format(), QStringLiteral("OSTC 4 1.7.5: settings read; available controls can be edited."));
	// A failed or unsupported subsequent read may not retain the previous map.
	identity.firmware = 0;
	QVERIFY(capture(false));
	dialog.deviceDetailsReceived(details);
	QVERIFY(!dialog.deviceDetails.ostc4Editable[0x2d - 0x10]);
	QVERIFY(!brightness->isEnabled());
	QVERIFY(!setpoints->item(0, 1)->flags().testFlag(Qt::ItemIsEditable));
	QVERIFY(progress->format().contains(QStringLiteral("unsupported or unverified firmware; settings disabled")));
	dialog.processStateChanged(ConfigureDiveComputer::DONE, ConfigureDiveComputer::WRITING);
	QVERIFY(!dialog.ui.page_ostc4->isEnabled());
	dialog.processStateChanged(ConfigureDiveComputer::WRITING, ConfigureDiveComputer::DONE);
	QVERIFY(!dialog.deviceDetails.ostc4Snapshot.valid);
	QVERIFY(!dialog.ui.page_ostc4->isEnabled());
	QVERIFY(!dialog.ui.retrieveDetails->isEnabled()); // Connect is required after save teardown.
	QVERIFY(dialog.ui.connectButton->isEnabled());
	dialog.configError(QStringLiteral("Read failed"));
	QVERIFY(progress->format().contains(QStringLiteral("Read failed")));
	QCOMPARE(progress->toolTip(), progress->format());
}

QTEST_MAIN(TestOstc4Dialog)
