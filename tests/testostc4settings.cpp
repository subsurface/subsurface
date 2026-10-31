// SPDX-License-Identifier: GPL-2.0
#include "testostc4settings.h"
#include "core/ostc4settingstransaction.h"
#include "core/devicedetails.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTextStream>
#include <cstring>
#include <map>
#include <algorithm>

using Transaction = Ostc4SettingsTransaction;

void TestOstc4Settings::gate()
{
	const quint16 stable175 = (1 << 11) | (7 << 6) | (5 << 1);
	const quint16 beta176 = (1 << 11) | (7 << 6) | (6 << 1) | 1;
	const quint16 beta177 = (1 << 11) | (7 << 6) | (7 << 1) | 1;
	Transaction::Identity id { 0x43, 123, stable175 };
	for (quint8 model : { quint8(0x43), quint8(0x44) }) {
		id.model = model;
		for (quint16 firmware : { stable175, beta176, beta177 }) {
			id.firmware = firmware;
			QCOMPARE(Transaction::supported(id), model == 0x43 || firmware == stable175);
		}
		for (quint16 firmware : { quint16(0), quint16(stable175 | 1),
			quint16(beta176 & ~1), quint16(beta177 & ~1),
			quint16((1 << 11) | (7 << 6) | (8 << 1) | 1) }) {
			id.firmware = firmware;
			QVERIFY(!Transaction::supported(id));
		}
		id.firmware = stable175;
		id.serial = 0;
		QVERIFY(!Transaction::supported(id));
		id.serial = 123;
	}
	id.model = 0x44;
	id.firmware = beta176;
	int betaReads = 0;
	Transaction ostc5Beta;
	QVERIFY(ostc5Beta.capture([&](Transaction::Identity &out) { out = id; return true; },
		[&](quint8, QByteArray &) { ++betaReads; return true; }));
	QVERIFY(!ostc5Beta.valid);
	QCOMPARE(betaReads, 0);
	id.model = 0; // unknown model
	QVERIFY(!Transaction::supported(id));
	id.model = 0x0a; // OSTC3 model (libdc hw_ostc3.c)
	QVERIFY(!Transaction::supported(id));
	id.model = 0x42;
	QVERIFY(!Transaction::supported(id));
	Transaction blocked;
	auto identify = [&id](Transaction::Identity &out) { out = id; return true; };
	int reads = 0;
	QVERIFY(blocked.capture(identify, [&](quint8, QByteArray &) { ++reads; return false; }));
	QCOMPARE(blocked.records[0].availability, Transaction::Record::Unknown);
	QVERIFY(!blocked.valid);
	QCOMPARE(reads, 0);
	QVERIFY(blocked.displayRecords[0].isEmpty());
	id.model = 0x43;
	id.firmware = quint16(stable175 | 1); // beta 1.7.5 is not stable 1.7.5
	QVERIFY(blocked.capture(identify, [&](quint8, QByteArray &) { ++reads; return false; }));
	QCOMPARE(blocked.records[5].availability, Transaction::Record::Unknown);
	QVERIFY(!blocked.valid);
	QCOMPARE(reads, 0);
	id.firmware = beta177;
	Transaction failedRead;
	QVERIFY(failedRead.capture(identify, [](quint8, QByteArray &) { return false; }));
	QCOMPARE(failedRead.records[0].availability, Transaction::Record::Unknown);
	QVERIFY(failedRead.valid);
	QVERIFY(failedRead.displayRecords[0].isEmpty());
}

void TestOstc4Settings::supportedTransactions()
{
	const quint16 stable175 = (1 << 11) | (7 << 6) | (5 << 1);
	const quint16 beta176 = (1 << 11) | (7 << 6) | (6 << 1) | 1;
	const quint16 beta177 = (1 << 11) | (7 << 6) | (7 << 1) | 1;
	for (quint8 model : { quint8(0x43), quint8(0x44) }) {
		for (quint16 firmware : { stable175, beta176, beta177 }) {
			if (model != 0x43 || firmware == stable175)
				continue; // checked source-backed path is only OSTC4 beta
			Transaction::Identity id { model, 123, firmware };
			std::map<quint8, QByteArray> device;
			for (quint8 key : Transaction::ids)
				device[key] = QByteArray::fromHex(key < 0x25 ? "784400a5" :
					(key == 0x25 ? "1e000000" : "55000000"));
			int reads = 0, writes = 0, backups = 0;
			auto identify = [&](Transaction::Identity &out) { out = id; return true; };
			auto read = [&](quint8 key, QByteArray &out) {
				++reads;
				out = device.count(key) ? device.at(key) : QByteArray::fromHex("15000000");
				return true;
			};
			auto write = [&](quint8 key, const QByteArray &value) { ++writes; device[key] = value; return true; };
			auto backup = [&](const Transaction &) { ++backups; return true; };
			Transaction snapshot;
			QVERIFY(snapshot.capture(identify, read));
			QVERIFY(snapshot.valid);
			QCOMPARE(reads, 0x35);
			QCOMPARE(snapshot.displayRecords[0].size(), 4);
			QVERIFY(snapshot.displayRecords[0x1d - 0x10].isEmpty()); // writable baseline is separate
			// A missing display-only reply does not block checked writes or
			// masquerade as a zero-valued device setting.
			QVERIFY(snapshot.capture(identify, [&](quint8 key, QByteArray &out) {
				if (key == 0x10) return false;
				return read(key, out);
			}));
			QVERIFY(snapshot.displayRecords[0].isEmpty());
			QVERIFY(snapshot.records[3].availability == Transaction::Record::Readable);
			std::array<QByteArray, 7> desired;
			for (size_t n = 0; n < desired.size(); ++n) desired[n] = snapshot.records[n].original;
			desired[3][0] = 121;
			QString report;
			auto swapped = id;
			swapped.model = model == 0x43 ? 0x44 : 0x43;
			id = swapped;
			QVERIFY(!snapshot.apply(identify, read, write, backup, desired, report));
			QCOMPARE(writes, 0);
			QCOMPARE(backups, 0);
			id.model = model;
			QVERIFY(snapshot.apply(identify, read, write, backup, desired, report));
			QCOMPARE(writes, 1);
			QCOMPARE(backups, 1);
			QCOMPARE(device[0x1d], desired[3]);
		}
	}
}

void TestOstc4Settings::bootloaderIdentity()
{
	// tComm.c BOOTLOADER_STANDALONE case 0x69 returns zero bytes 0..3;
	// its 0x77 reply still acknowledges without writing a setting.
	QByteArray hardware(5, '\0');
	hardware[1] = 0x3b;
	hardware[4] = 0x43;
	QByteArray version(64, '\0');
	Transaction::Identity identity;
	QVERIFY(!Transaction::decodeIdentity(hardware, version, identity));
	version[0] = 123;
	QVERIFY(!Transaction::decodeIdentity(hardware, version, identity));
	version[2] = static_cast<char>(((1 << 11) | (7 << 6) | (6 << 1) | 1) & 0xff);
	version[3] = static_cast<char>(((1 << 11) | (7 << 6) | (6 << 1) | 1) >> 8);
	QVERIFY(Transaction::decodeIdentity(hardware, version, identity));
	QVERIFY(Transaction::supported(identity));
	Transaction snapshot;
	auto bootloader = [&](Transaction::Identity &out) {
		QByteArray zeros(64, '\0');
		return Transaction::decodeIdentity(hardware, zeros, out);
	};
	int reads = 0, writes = 0, backups = 0;
	QVERIFY(!snapshot.capture(bootloader, [&](quint8, QByteArray &) { ++reads; return true; }));
	QCOMPARE(reads, 0);
	std::array<QByteArray, 7> desired {};
	QString report;
	QVERIFY(!snapshot.apply(bootloader, [&](quint8, QByteArray &) { ++reads; return true; },
		[&](quint8, const QByteArray &) { ++writes; return true; },
		[&](const Transaction &) { ++backups; return true; }, desired, report));
	QCOMPARE(writes, 0);
	QCOMPARE(backups, 0);
	hardware[1] = 0x1a; // OSTC3 hardware descriptor
	QVERIFY(!Transaction::decodeIdentity(hardware, version, identity));
}

void TestOstc4Settings::hardwareReplyBoundary()
{
	// libdc hw_ostc3_device_hardware accepts only 1 or 5 bytes, before init/I/O.
	// The five-byte 0x60 reply contains the model at offset 4, not offset 5.
	QByteArray hardware = QByteArray::fromHex("003b000043");
	QByteArray version(64, '\0');
	version[0] = 123;
	const quint16 stable175 = (1 << 11) | (7 << 6) | (5 << 1);
	version[2] = static_cast<char>(stable175 & 0xff);
	version[3] = static_cast<char>(stable175 >> 8);
	int hardwareReads = 0, versionReads = 0;
	auto readHardware = [&](unsigned char *out, unsigned int size) {
		++hardwareReads;
		if (size != 1 && size != 5) return false;
		memcpy(out, hardware.constData(), size);
		return true;
	};
	auto readVersion = [&](unsigned char *out, unsigned int size) {
		++versionReads;
		if (size != 64) return false;
		memcpy(out, version.constData(), size);
		return true;
	};
	Transaction::Identity identity;
	for (quint8 model : { quint8(0x43), quint8(0x44) }) {
		hardware[4] = static_cast<char>(model);
		QVERIFY(Transaction::isOstc4Hardware(hardware)); // selected OSTC3 must detect actual OSTC4/5
		QVERIFY(Transaction::readIdentity(readHardware, readVersion, identity));
		QCOMPARE(identity.model, model);
		QCOMPARE(identity.serial, quint16(123));
		QCOMPARE(identity.firmware, stable175);
		QVERIFY(Transaction::supported(identity));
	}
	QCOMPARE(hardwareReads, 2);
	QCOMPARE(versionReads, 2);
	hardware = QByteArray::fromHex("001a00000a"); // OSTC3 is not an OSTC4/5
	QVERIFY(!Transaction::isOstc4Hardware(hardware));
	QVERIFY(!Transaction::readIdentity(readHardware, readVersion, identity));
	QCOMPARE(identity.model, quint8(0x44)); // failed decoding cannot replace a valid identity
	QVERIFY(!readHardware(nullptr, 4)); // previously used detection size was rejected
	QVERIFY(!readHardware(nullptr, 6)); // previously used identity size was rejected
	QVERIFY(!Transaction::decodeIdentity(QByteArray::fromHex("003b00004300"), version, identity));
}

void TestOstc4Settings::gfRequiresReadablePartner()
{
	Transaction::Identity identity { 0x43, 123, static_cast<quint16>((1 << 11) | (7 << 6) | (6 << 1) | 1) };
	auto identify = [&](Transaction::Identity &out) { out = identity; return true; };
	for (const quint8 missing : { quint8(0x25), quint8(0x26) }) {
		Transaction snapshot;
		auto read = [&](quint8 key, QByteArray &out) {
			if (key == missing) return false;
			out = QByteArray::fromHex(key < 0x25 ? "784400a5" : (key == 0x25 ? "1e000000" : "55000000"));
			return true;
		};
		QVERIFY(snapshot.capture(identify, read));
		const size_t missingIndex = missing == 0x25 ? 5 : 6;
		const size_t editedIndex = missingIndex == 5 ? 6 : 5;
		QCOMPARE(snapshot.records[missingIndex].availability, Transaction::Record::Unknown);
		QCOMPARE(snapshot.records[missingIndex].original.size(), 0);
		QCOMPARE(snapshot.records[editedIndex].availability, Transaction::Record::Readable);
		std::array<QByteArray, 7> desired;
		for (size_t n = 0; n < desired.size(); ++n) desired[n] = snapshot.records[n].original;
		desired[editedIndex][0] = editedIndex == 5 ? 31 : 86;
		int reads = 0, backups = 0, writes = 0;
		QString report;
		QVERIFY(!snapshot.apply(identify, [&](quint8, QByteArray &) { ++reads; return true; },
			[&](quint8, const QByteArray &) { ++writes; return true; },
			[&](const Transaction &) { ++backups; return true; }, desired, report));
		QVERIFY(report.contains("Both gradient factors"));
		QCOMPARE(reads, 0);
		QCOMPARE(backups, 0);
		QCOMPARE(writes, 0);
	}
}

void TestOstc4Settings::transaction()
{
	Transaction::Identity id { 0x43, 123, static_cast<quint16>((1 << 11) | (7 << 6) | (7 << 1) | 1) };
	std::map<quint8, QByteArray> device;
	for (quint8 setting : Transaction::ids)
		device[setting] = QByteArray::fromHex(setting < 0x25 ? "784400a5" : (setting == 0x25 ? "1e000000" : "55000000"));
	bool failRead = false, failWrite = false;
	int writes = 0, backups = 0;
	auto identify = [&](Transaction::Identity &out) { out = id; return true; };
	auto read = [&](quint8 key, QByteArray &out) {
		if (failRead && key == 0x26) return false;
		out = device[key];
		return true;
	};
	auto write = [&](quint8 key, const QByteArray &value) {
		++writes;
		if (failWrite && key == 0x26) return false;
		device[key] = value;
		return true;
	};
	auto backup = [&](const Transaction &) { ++backups; return true; };
	Transaction base;
	QVERIFY(base.capture(identify, read));
	std::array<QByteArray, 7> desired;
	for (size_t n = 0; n < desired.size(); ++n)
		desired[n] = base.records[n].original;
	QString report;
	QVERIFY(base.apply(identify, read, write, backup, desired, report));
	QCOMPARE(writes, 0);
	QCOMPARE(backups, 0);
	desired[3][1] = 70;
	QVERIFY(!base.apply(identify, read, write, [](const Transaction &) { return false; }, desired, report));
	QCOMPARE(writes, 0); // cancel/open/write/commit failures all abort before first write
	id.serial++;
	QVERIFY(!base.apply(identify, read, write, backup, desired, report));
	id.serial--;
	id.firmware++;
	QVERIFY(!base.apply(identify, read, write, backup, desired, report));
	id.firmware--;
	id.model = 0x44;
	QVERIFY(!base.apply(identify, read, write, backup, desired, report));
	id.model = 0x43;
	device[0x1d][0] = 80;
	QVERIFY(!base.apply(identify, read, write, backup, desired, report));
	device[0x1d] = base.records[3].original;
	QCOMPARE(writes, 0);
	QVERIFY(base.apply(identify, read, write, backup, desired, report));
	QCOMPARE(writes, 1);
	QCOMPARE(static_cast<unsigned char>(device[0x1d][2]), 0);
	QCOMPARE(static_cast<unsigned char>(device[0x1d][3]), 0xa5); // compound trailing bytes
	// Fresh capture is required after every transaction.
	QVERIFY(base.capture(identify, read));
	for (size_t n = 0; n < desired.size(); ++n) desired[n] = base.records[n].original;
	desired[3][0] = 121;
	device[0x1d][0] = 122;
	QVERIFY(!base.apply(identify, read, write, backup, desired, report)); // preflight mismatch
	device[0x1d] = base.records[3].original;
	QCOMPARE(device[0x1d], base.records[3].original);
	bool afterWrite = false;
	auto readMismatch = [&](quint8 key, QByteArray &out) {
		if (!read(key, out)) return false;
		if (afterWrite && key == 0x1d) out[0] = 122;
		return true;
	};
	auto writeMismatch = [&](quint8 key, const QByteArray &value) { afterWrite = true; return write(key, value); };
	QVERIFY(!base.apply(identify, readMismatch, writeMismatch, backup, desired, report));
	QVERIFY2(report.contains("unverified"), qPrintable(report));
	// A timeout after an acknowledged write leaves the device state unknown.
	QVERIFY(base.capture(identify, read));
	for (size_t n = 0; n < desired.size(); ++n) desired[n] = base.records[n].original;
	desired[3][0] = 122;
	afterWrite = false;
	auto readTimeout = [&](quint8 key, QByteArray &out) {
		return !(afterWrite && key == 0x1d) && read(key, out);
	};
	QVERIFY(!base.apply(identify, readTimeout, writeMismatch, backup, desired, report));
	QVERIFY(report.contains("unverified"));
	QVERIFY(base.capture(identify, read));
	for (size_t n = 0; n < desired.size(); ++n) desired[n] = base.records[n].original;
	desired[3][0] = 123;
	desired[6][0] = 90;
	failWrite = true;
	const int before = writes;
	QVERIFY(!base.apply(identify, read, write, backup, desired, report));
	QCOMPARE(writes - before, 2);
	QVERIFY(report.contains("1 verified"));
	QVERIFY(report.contains("uncertain"));
	failWrite = false;
	QVERIFY(base.capture(identify, read));
	for (size_t n = 0; n < desired.size(); ++n) desired[n] = base.records[n].original;
	desired[6][0] = 91;
	failRead = true;
	QVERIFY(!base.apply(identify, read, write, backup, desired, report));
	failRead = false;
	QVERIFY(base.capture(identify, read));
	desired[0] = base.records[0].original;
	desired[0][0] = 125; // auto-normalised setpoint must remain non-writable
	QVERIFY(!base.apply(identify, read, write, backup, desired, report));
}

void TestOstc4Settings::recovery()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	Transaction base;
	base.identity = { 0x43, 123, static_cast<quint16>((1 << 11) | (7 << 6) | (6 << 1) | 1) };
	base.records[0] = { 0x1a, QByteArray::fromHex("504000a5"), Transaction::Record::Readable };
	for (size_t n = 1; n < Transaction::ids.size(); ++n)
		base.records[n] = { Transaction::ids[n], {}, Transaction::Record::Unknown };
	QString error, description;
	QVERIFY(!base.saveBackup(dir.path(), error)); // open failure
	const QString path = dir.filePath("partial.json");
	QVERIFY(base.saveBackup(path, error));
	QVERIFY(Transaction::validateBackup(path, base.identity, description));
	QFile current(path);
	QVERIFY(current.open(QIODevice::ReadOnly));
	QJsonObject older = QJsonDocument::fromJson(current.readAll()).object();
	current.close();
	older.insert(QStringLiteral("version"), 1);
	older.remove(QStringLiteral("legacyRecords"));
	older.remove(QStringLiteral("customTextRaw"));
	QJsonArray excluded;
	for (int id = 0x10; id <= 0x44; ++id)
		if (std::find(Transaction::ids.begin(), Transaction::ids.end(), id) == Transaction::ids.end())
			excluded.append(id);
	older.insert(QStringLiteral("notCaptured"), excluded);
	QFile previous(dir.filePath("previous.json"));
	QVERIFY(previous.open(QIODevice::WriteOnly));
	previous.write(QJsonDocument(older).toJson());
	previous.close();
	QVERIFY(Transaction::validateBackup(previous.fileName(), base.identity, description));
	QVERIFY(description.contains("504000a5"));
	QVERIFY(!description.contains("1b:")); // unavailable fields not synthesised
	auto wrong = base.identity;
	wrong.serial++;
	QVERIFY(!Transaction::validateBackup(path, wrong, description));
	wrong = base.identity;
	wrong.firmware++;
	QVERIFY(!Transaction::validateBackup(path, wrong, description));
	wrong = base.identity;
	wrong.model = 0x44;
	QVERIFY(!Transaction::validateBackup(path, wrong, description));
	QFile malformed(dir.filePath("malformed.json"));
	QVERIFY(malformed.open(QIODevice::WriteOnly));
	QVERIFY(malformed.write("{bad") > 0);
	malformed.close();
	QVERIFY(!Transaction::validateBackup(malformed.fileName(), base.identity, description));
	QFile original(path);
	QVERIFY(original.open(QIODevice::ReadOnly));
	QByteArray damaged = original.readAll().replace("504000a5", "504000a");
	original.close();
	QFile truncated(dir.filePath("truncated.json"));
	QVERIFY(truncated.open(QIODevice::WriteOnly));
	QCOMPARE(truncated.write(damaged), damaged.size());
	truncated.close();
	QVERIFY(!Transaction::validateBackup(truncated.fileName(), base.identity, description));
}

void TestOstc4Settings::backupFailures()
{
	Transaction::Identity id { 0x43, 123, static_cast<quint16>((1 << 11) | (7 << 6) | (6 << 1) | 1) };
	Transaction snapshot;
	auto identify = [&](Transaction::Identity &out) { out = id; return true; };
	auto read = [](quint8 key, QByteArray &out) {
		out = QByteArray::fromHex(key < 0x25 ? "784400a5" : (key == 0x25 ? "1e000000" : "55000000"));
		return true;
	};
	QVERIFY(snapshot.capture(identify, read));
	std::array<QByteArray, 7> desired;
	for (size_t n = 0; n < desired.size(); ++n) desired[n] = snapshot.records[n].original;
	desired[3][1] = 70;
	int writes = 0;
	auto write = [&](quint8, const QByteArray &) { ++writes; return true; };
	QString report;
	// Simulated cancellation, open failure, short write and commit failure.
	// A real QSaveFile open failure is also exercised in recovery().
	for (const char *failure : { "cancel", "open", "write", "commit" }) {
		bool attempted = false;
		auto backup = [&](const Transaction &) { attempted = true; Q_UNUSED(failure); return false; };
		QVERIFY(!snapshot.apply(identify, read, write, backup, desired, report));
		QVERIFY(attempted);
		QCOMPARE(writes, 0);
	}
}

void TestOstc4Settings::legacyParity()
{
	Transaction::Identity id { 0x44, 123, static_cast<quint16>((1 << 11) | (7 << 6) | (5 << 1)) };
	std::map<quint8, QByteArray> device;
	for (int key = 0x10; key <= 0x44; ++key)
		device[key] = QByteArray::fromHex("154400a5");
	auto identify = [&](Transaction::Identity &out) { out = id; return true; };
	auto read = [&](quint8 key, QByteArray &out) { out = device[key]; return true; };
	int writes = 0, backups = 0;
	auto write = [&](quint8 key, const QByteArray &value) { ++writes; device[key] = value; return true; };
	auto backup = [&](const Transaction &) { ++backups; return true; };
	Transaction snapshot;
	QVERIFY(snapshot.capture(identify, read));
	std::array<QByteArray, 0x35> desired = snapshot.displayRecords;
	DeviceDetails fields;
	fields.ostc4Snapshot = snapshot;
	fields.gas2 = gas(22, 68, 0, 165);
	fields.ostc4Editable[1] = true;
	QCOMPARE(fields.ostc4LegacyEdits()[1], QByteArray::fromHex("164400a5"));
	QCOMPARE(fields.ostc4LegacyEdits()[0], snapshot.displayRecords[0]); // hidden/default UI values cannot write
	fields.ostc4Editable[1] = false;
	QCOMPARE(fields.ostc4LegacyEdits()[1], snapshot.displayRecords[1]);
	for (size_t n = 0; n < snapshot.records.size(); ++n)
		desired[Transaction::ids[n] - 0x10] = snapshot.records[n].original;
	QString report;
	QVERIFY(snapshot.applyLegacy(identify, read, write, backup, desired, report));
	QCOMPARE(writes, 0);
	QCOMPARE(backups, 0);
	desired[1][0] = 22; // gas 2, trailing bytes intact
	QVERIFY(!snapshot.applyLegacy(identify, read, write, [](const Transaction &) { return false; }, desired, report));
	QCOMPARE(writes, 0);
	id.model = 0x43;
	QVERIFY(!snapshot.applyLegacy(identify, read, write, backup, desired, report));
	id.model = 0x44;
	QVERIFY(snapshot.applyLegacy(identify, read, write, backup, desired, report));
	QCOMPARE(writes, 1);
	QCOMPARE(device[0x11], QByteArray::fromHex("164400a5"));
	QCOMPARE(device[0x10], QByteArray::fromHex("154400a5"));
	QVERIFY(snapshot.capture(identify, read));
	desired = snapshot.displayRecords;
	for (size_t n = 0; n < snapshot.records.size(); ++n)
		desired[Transaction::ids[n] - 0x10] = snapshot.records[n].original;
	desired[0x36 - 0x10][0] = 1;
	QVERIFY(!snapshot.applyLegacy(identify, read, write, backup, desired, report));
	QVERIFY(report.contains("36"));
	QCOMPARE(writes, 1);
	desired[0x36 - 0x10] = snapshot.displayRecords[0x36 - 0x10];
	desired[0x35 - 0x10][0] = static_cast<char>(-5);
	QVERIFY(!snapshot.applyLegacy(identify, read, write, backup, desired, report));
	QVERIFY(report.contains("35"));
	QCOMPARE(writes, 1);
	QVERIFY(snapshot.capture(identify, read));
	desired = snapshot.displayRecords;
	for (size_t n = 0; n < snapshot.records.size(); ++n)
		desired[Transaction::ids[n] - 0x10] = snapshot.records[n].original;
	desired[0][0] = 22;
	desired[2][0] = 23;
	const int previous = writes;
	QVERIFY(!snapshot.applyLegacy(identify, read,
		[&](quint8 key, const QByteArray &value) { ++writes; if (key == 0x12) return false; device[key] = value; return true; },
		backup, desired, report));
	QCOMPARE(writes - previous, 2);
	QVERIFY(report.contains("1 read-back matched"));
	QVERIFY(report.contains("uncertain"));
	QVERIFY(snapshot.capture(identify, read));
	desired = snapshot.displayRecords;
	for (size_t n = 0; n < snapshot.records.size(); ++n)
		desired[Transaction::ids[n] - 0x10] = snapshot.records[n].original;
	desired[0][0] = 23;
	bool afterWrite = false;
	QVERIFY(!snapshot.applyLegacy(identify, [&](quint8 key, QByteArray &out) {
		if (afterWrite && key == 0x10) return false;
		return read(key, out);
	}, [&](quint8 key, const QByteArray &value) { afterWrite = true; return write(key, value); }, backup, desired, report));
	QVERIFY(report.contains("unverified"));
	id.firmware |= 1;
	QVERIFY(!snapshot.applyLegacy(identify, read, write, backup, desired, report));
	id.firmware &= ~quint16(1);
	QVERIFY(snapshot.capture(identify, read));
	QByteArray custom = QByteArray("OLD").leftJustified(60, ' ');
	snapshot.originalCustomText = custom;
	desired = snapshot.displayRecords;
	for (size_t n = 0; n < snapshot.records.size(); ++n)
		desired[Transaction::ids[n] - 0x10] = snapshot.records[n].original;
	int textWrites = 0;
	auto readText = [&](QByteArray &out) { out = custom; return true; };
	auto writeText = [&](const QByteArray &value) { ++textWrites; custom = value.leftJustified(60, ' '); return true; };
	QVERIFY(snapshot.applyLegacy(identify, read, write, backup, desired, report, QStringLiteral("NEW"), readText, writeText));
	QCOMPARE(textWrites, 1);
	QCOMPARE(custom, QByteArray("NEW").leftJustified(60, ' '));
	QVERIFY(snapshot.capture(identify, read));
	desired = snapshot.displayRecords;
	for (size_t n = 0; n < snapshot.records.size(); ++n)
		desired[Transaction::ids[n] - 0x10] = snapshot.records[n].original;
	int clockWrites = 0;
	const int beforeClock = backups;
	const int beforeClockSettings = writes;
	QVERIFY(snapshot.applyLegacy(identify, read, write, backup, desired, report, {}, {}, {}, true,
		[&]() { ++clockWrites; return true; }));
	QCOMPARE(writes, beforeClockSettings); // no setting write for a clock-only action
	QCOMPARE(backups, beforeClock + 1);
	QCOMPARE(clockWrites, 1);
	QVERIFY(report.contains("not read-back verified"));
	QVERIFY(!snapshot.applyLegacy(identify, read, write, [](const Transaction &) { return false; },
		desired, report, {}, {}, {}, true, [&]() { ++clockWrites; return true; }));
	QCOMPARE(clockWrites, 1); // no clock command without successful backup
	QVERIFY(!snapshot.applyLegacy(identify, read, write, backup, desired, report, {}, {}, {}, true,
		[&]() { ++clockWrites; return false; }));
	QCOMPARE(clockWrites, 2);
	QCOMPARE(writes, beforeClockSettings);
	QCOMPARE(backups, beforeClock + 2);
	QVERIFY(report.contains("clock state uncertain"));
	id.model = 0x43;
	QVERIFY(snapshot.capture(identify, read));
	desired = snapshot.displayRecords;
	for (size_t n = 0; n < snapshot.records.size(); ++n)
		desired[Transaction::ids[n] - 0x10] = snapshot.records[n].original;
	QVERIFY(snapshot.applyLegacy(identify, read, write, backup, desired, report));
	std::array<QByteArray, 7> sourceDesired;
	for (size_t n = 0; n < sourceDesired.size(); ++n)
		sourceDesired[n] = snapshot.records[n].original;
	QVERIFY(!snapshot.apply(identify, read, write, backup, sourceDesired, report));
	QVERIFY(report.contains("Source-backed transaction unavailable"));
}

QTEST_GUILESS_MAIN(TestOstc4Settings)
