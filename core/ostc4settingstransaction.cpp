// SPDX-License-Identifier: GPL-2.0
#include "ostc4settingstransaction.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QObject>
#include <algorithm>

// Source evidence: OSTC4 Discovery/Src/settings.c (Icon_Integration da58cfad,
// development settings.c blob f1279424): writeData, readData, setSetpoint,
// readDataLimits, check_and_correct_settings. tComm.c select_mode and
// tComm_exit: bootloader replies to 0x77 without writing; exit can normalise.
// Released 1.7.5 OSTC4/5 uses the pre-existing Subsurface editor's
// compatibility path, not a proven match to a particular beta build.
// The beta tuples are source-backed only for OSTC4. No OSTC5 beta source or
// released-binary provenance, hardware-write or persistence test is available.
bool Ostc4SettingsTransaction::supported(const Identity &i)
{
	const quint16 stable175 = (1 << 11) | (7 << 6) | (5 << 1);
	const quint16 beta176 = (1 << 11) | (7 << 6) | (6 << 1) | 1;
	const quint16 beta177 = (1 << 11) | (7 << 6) | (7 << 1) | 1;
	return i.serial != 0 &&
		(((i.model == 0x43 || i.model == 0x44) && i.firmware == stable175) ||
		 (i.model == 0x43 && (i.firmware == beta176 || i.firmware == beta177)));
}

bool Ostc4SettingsTransaction::legacyCompatible(const Identity &i)
{
	return supported(i) && i.firmware == ((1 << 11) | (7 << 6) | (5 << 1));
}

bool Ostc4SettingsTransaction::readIdentity(const ReadReply &hardware, const ReadReply &version, Identity &identity)
{
	unsigned char hardwareReply[5] = {};
	unsigned char versionReply[64] = {};
	return hardware(hardwareReply, sizeof(hardwareReply)) && version(versionReply, sizeof(versionReply)) &&
		decodeIdentity(QByteArray(reinterpret_cast<const char *>(hardwareReply), sizeof(hardwareReply)),
			QByteArray(reinterpret_cast<const char *>(versionReply), sizeof(versionReply)), identity);
}

bool Ostc4SettingsTransaction::isOstc4Hardware(const QByteArray &hardware)
{
	return hardware.size() == 5 && static_cast<quint8>(hardware[0]) == 0 &&
		static_cast<quint8>(hardware[1]) == 0x3b;
}

bool Ostc4SettingsTransaction::decodeIdentity(const QByteArray &hardware, const QByteArray &version, Identity &identity)
{
	// tComm.c (BOOTLOADER_STANDALONE) returns zero serial/version for 0x69
	// and acknowledges 0x77 without writing. Reject that identity before
	// capture and again on reconnect; an acknowledgement cannot prove mode.
	if (!isOstc4Hardware(hardware) || version.size() != 64)
		return false;
	const auto serial = static_cast<quint8>(version[0]) | (static_cast<quint8>(version[1]) << 8);
	const auto firmware = static_cast<quint8>(version[2]) | (static_cast<quint8>(version[3]) << 8);
	if (!serial || !firmware)
		return false;
	identity = { static_cast<quint8>(hardware[4]), static_cast<quint16>(serial), static_cast<quint16>(firmware) };
	return true;
}

QString Ostc4SettingsTransaction::firmwareString(quint16 v)
{
	return QStringLiteral("%1.%2.%3%4").arg(v >> 11).arg((v >> 6) & 31)
		.arg((v >> 1) & 31).arg((v & 1) ? QStringLiteral(" beta") : QString());
}

QString Ostc4SettingsTransaction::customText(const QByteArray &raw)
{
	const int end = raw.indexOf('\0');
	return QString::fromUtf8(raw.constData(), end < 0 ? raw.size() : end).trimmed();
}

bool Ostc4SettingsTransaction::capture(const Identify &identify, const Read &read)
{
	valid = false;
	originalCustomText.clear();
	for (auto &value : displayRecords)
		value.clear();
	for (size_t n = 0; n < ids.size(); ++n)
		records[n] = { ids[n], {}, Record::Unknown };
	if (!identify(identity))
		return false;
	if (!supported(identity)) {
		// No exact-version evidence is not a firmware rejection of these IDs.
		// No read has been attempted, so their availability remains unknown.
		return true;
	}
	valid = true;
	for (auto &record : records) {
		QByteArray value;
		if (read(record.id, value) && value.size() == 4) {
			record.original = value;
			record.availability = Record::Readable;
		} else {
			record.availability = Record::Unknown;
		}
	}
	// readData() in Discovery/Src/settings.c exposes 0x10..0x44. Keep
	// unsuccessful reads absent rather than displaying invented zero values.
	for (int id = 0x10; id <= 0x44; ++id) {
		if (std::find(ids.begin(), ids.end(), id) != ids.end())
			continue;
		QByteArray value;
		if (read(id, value) && value.size() == 4)
			displayRecords[id - 0x10] = value;
	}
	return true;
}

static bool validEdit(size_t n, const QByteArray &value, const std::array<QByteArray, 7> &desired)
{
	if (value.size() != 4)
		return false;
	const int v = static_cast<unsigned char>(value[0]);
	if (n < 5) {
		const int depth = static_cast<unsigned char>(value[1]);
		// Auto-setpoint correction can alter indices 1..3 on exit; its
		// operating mode is not exposed here.
		return n >= 3 && v >= 50 && v <= 160 && depth <= 250;
	}
	if (n == 5) {
		if (desired[6].size() != 4)
			return false;
		const int high = static_cast<unsigned char>(desired[6][0]);
		return v >= 10 && v <= 99 && v <= high;
	}
	if (desired[5].size() != 4)
		return false;
	const int low = static_cast<unsigned char>(desired[5][0]);
	return v >= 45 && v <= 99 && low <= v;
}

bool Ostc4SettingsTransaction::apply(const Identify &identify, const Read &read, const Write &write,
	const Backup &backup, const std::array<QByteArray, 7> &desired, QString &report) const
{
	if (!valid || !supported(identity) || legacyCompatible(identity)) {
		report = QObject::tr("Source-backed transaction unavailable for this model/firmware; reload required.");
		return false;
	}
	std::array<bool, 7> changed {};
	int total = 0;
	for (size_t n = 0; n < records.size(); ++n) {
		const Record &r = records[n];
		if (r.availability != Record::Readable) {
			if (desired[n] != r.original) {
				report = QObject::tr("Unavailable setting edited; reload required.");
				return false;
			}
			continue;
		}
		if (desired[n].size() != 4 ||
		    (n < 5 && desired[n].mid(2) != r.original.mid(2)) ||
		    (n >= 5 && desired[n].mid(1) != r.original.mid(1))) {
			report = QObject::tr("Invalid edit or unrepresented bytes changed; reload required.");
			return false;
		}
		changed[n] = desired[n] != r.original;
		if (changed[n] && n >= 5 &&
		    (records[11 - n].availability != Record::Readable || records[11 - n].original.size() != 4 ||
		     desired[11 - n].size() != 4)) {
			report = QObject::tr("Both gradient factors must be readable before editing either; reload required.");
			return false;
		}
		if (changed[n] && !validEdit(n, desired[n], desired)) {
			report = QObject::tr("Setting outside firmware limits; reload required.");
			return false;
		}
		total += changed[n];
	}
	Identity current;
	if (!identify(current)) {
		report = QObject::tr("Could not read device identity/firmware; zero writes. Check the connection and read settings again.");
		return false;
	}
	if (!(current == identity) || !supported(current)) {
		report = QObject::tr("Device identity/firmware differs from the original read; zero writes. Reload required.");
		return false;
	}
	for (const Record &r : records) {
		if (r.availability != Record::Readable)
			continue;
		QByteArray now;
		if (!read(r.id, now)) {
			report = QObject::tr("Device setting %1 cannot be read; zero writes. Reload required.").arg(r.id, 2, 16, QLatin1Char('0'));
			return false;
		}
		if (now != r.original) {
			report = QObject::tr("Device setting %1 changed since the original read; zero writes. Reload required.").arg(r.id, 2, 16, QLatin1Char('0'));
			return false;
		}
	}
	if (!total) {
		report = QObject::tr("No changes; zero writes.");
		return true;
	}
	if (!backup(*this)) {
		report = QObject::tr("Backup cancelled or failed; zero writes. Reload required.");
		return false;
	}
	int verified = 0;
	for (size_t n = 0; n < records.size(); ++n) {
		if (!changed[n])
			continue;
		if (!write(records[n].id, desired[n])) {
			report = QObject::tr("Write %1 failed: %2 verified, current write uncertain, %3 not attempted. Reload required; consult backup.")
				.arg(records[n].id, 2, 16, QLatin1Char('0')).arg(verified).arg(total - verified - 1);
			return false;
		}
		QByteArray actual;
		if (!read(records[n].id, actual) || actual != desired[n]) {
			report = QObject::tr("Read-back %1 failed/mismatched: %2 verified, current write unverified, %3 not attempted. Reload required; consult backup.")
				.arg(records[n].id, 2, 16, QLatin1Char('0')).arg(verified).arg(total - verified - 1);
			return false;
		}
		++verified;
	}
	report = QObject::tr("%1 setting(s) verified by read-back; reload before another edit. Persistence after disconnect/power cycle not verified.").arg(verified);
	return true;
}

// Compatibility with the released 1.7.5 editor. Unlike the source-backed
// subset, these records retain the legacy UI encoding and are only claimed as
// read-back matched, not as validated firmware semantics or persistence.
bool Ostc4SettingsTransaction::applyLegacy(const Identify &identify, const Read &read, const Write &write,
	const Backup &backup, const std::array<QByteArray, 0x35> &desired, QString &report,
	const QString &text, const std::function<bool(QByteArray &)> &readText,
	const std::function<bool(const QByteArray &)> &writeText,
	bool syncClock, const std::function<bool()> &setClock) const
{
	if (!valid || !legacyCompatible(identity)) {
		report = QObject::tr("Legacy editor unavailable for this model/firmware; reload required.");
		return false;
	}
	std::array<bool, 0x35> changed {};
	// Whitelist the original UI fields, not every readable 0x72 reply.
	static constexpr std::array<quint8, 30> scalarIds {{
		0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
		0x29, 0x2c, 0x2d, 0x2e, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
		0x36, 0x38, 0x39, 0x3a, 0x3d, 0x3e, 0x41, 0x42, 0x43, 0x44
	}};
	int total = 0;
	const QByteArray desiredText = text.toUtf8();
	const bool textChanged = originalCustomText.size() == 60 &&
		customText(originalCustomText) != text;
	if (textChanged && (desiredText.size() >= 60 || desiredText.contains('\0') || !readText || !writeText)) {
		report = QObject::tr("Invalid or unavailable custom text; zero writes.");
		return false;
	}
	if (syncClock && !setClock) {
		report = QObject::tr("Clock synchronisation unavailable; zero writes.");
		return false;
	}
	for (int id = 0x10; id <= 0x44; ++id) {
		const auto found = std::find(ids.begin(), ids.end(), id);
		const QByteArray &original = found == ids.end() ? displayRecords[id - 0x10] : records[found - ids.begin()].original;
		const QByteArray &next = desired[id - 0x10];
		if (original.size() != 4) {
			if (!next.isEmpty()) {
				report = QObject::tr("Setting %1 unavailable; reload required.").arg(id, 2, 16, QLatin1Char('0'));
				return false;
			}
			continue;
		}
		if (next.size() != 4) {
			report = QObject::tr("Invalid edit for setting %1; reload required.").arg(id, 2, 16, QLatin1Char('0'));
			return false;
		}
		changed[id - 0x10] = next != original;
		if (changed[id - 0x10]) {
			if (id >= 0x1f && std::find(scalarIds.begin(), scalarIds.end(), id) == scalarIds.end()) {
				report = QObject::tr("Setting %1 is not editable; zero writes.").arg(id, 2, 16, QLatin1Char('0'));
				return false;
			}
			const int value = static_cast<quint8>(next[0]);
			const int depth = static_cast<quint8>(next[1]);
			if ((id <= 0x19 && (value < 4 || value > 100 || static_cast<quint8>(next[3]) > 250)) ||
			    (id >= 0x1a && id <= 0x1e && (value < 50 || value > 160 || depth > 250)) ||
			    ((id == 0x25 || id == 0x27) && (value < 10 || value > 99)) ||
			    ((id == 0x26 || id == 0x28) && (value < 45 || value > 99)) ||
			    (id == 0x36) || // firmware readData uses stop depth, writeData changes duration
			    ((id == 0x35 || id == 0x42) && value >= 128) || // inspected source negates, not two's complement
			    (id == 0x43 && value != 255 && (value < 60 || value > 240 || value % 60 != 0)) ||
			    (id == 0x44 && (value < 30 || value > 60 || value % 10 != 0))) {
				report = QObject::tr("Setting %1 has an unsafe or non-round-tripping legacy encoding; no writes. Reload required.")
					.arg(id, 2, 16, QLatin1Char('0'));
				return false;
			}
			if ((id == 0x25 || id == 0x26) &&
			    (records[id == 0x25 ? 6 : 5].original.size() != 4)) {
				report = QObject::tr("Both gradient factors must be readable before editing either; reload required.");
				return false;
			}
		}
		if (changed[id - 0x10] && id >= 0x1a && id <= 0x1e && next.mid(2) != original.mid(2)) {
			report = QObject::tr("Unrepresented setpoint bytes changed; reload required.");
			return false;
		}
		total += changed[id - 0x10];
	}
	Identity current;
	if (!identify(current)) {
		report = QObject::tr("Could not read device identity/firmware; zero writes. Check the connection and read settings again.");
		return false;
	}
	if (!(current == identity)) {
		report = QObject::tr("Device identity/firmware differs from the original read; zero writes. Reload required.");
		return false;
	}
	for (int id = 0x10; id <= 0x44; ++id) {
		const auto found = std::find(ids.begin(), ids.end(), id);
		const QByteArray &original = found == ids.end() ? displayRecords[id - 0x10] : records[found - ids.begin()].original;
		if (original.size() != 4)
			continue;
		QByteArray now;
		if (!read(id, now)) {
			report = QObject::tr("Setting %1 unreadable; zero writes. Reload required.").arg(id, 2, 16, QLatin1Char('0'));
			return false;
		}
		if (now != original) {
			report = QObject::tr("Setting %1 changed since the original read; zero writes. Reload required.").arg(id, 2, 16, QLatin1Char('0'));
			return false;
		}
	}
	if (originalCustomText.size() == 60) {
		QByteArray now;
		if (!readText || !readText(now) || now != originalCustomText) {
			report = QObject::tr("Custom text changed or unreadable; reload required.");
			return false;
		}
	}
	total += textChanged + syncClock;
	if (!total) {
		report = QObject::tr("No changes; zero writes.");
		return true;
	}
	if (!backup(*this)) {
		report = QObject::tr("Backup cancelled or failed; zero writes. Reload required.");
		return false;
	}
	int matched = 0;
	for (int id = 0x10; id <= 0x44; ++id) {
		if (!changed[id - 0x10])
			continue;
		if (!write(id, desired[id - 0x10])) {
			report = QObject::tr("Legacy write %1 failed: %2 read-back matched, current write uncertain, %3 not attempted. Reload required; consult backup.")
				.arg(id, 2, 16, QLatin1Char('0')).arg(matched).arg(total - matched - 1);
			return false;
		}
		QByteArray now;
		if (!read(id, now) || now != desired[id - 0x10]) {
			report = QObject::tr("Legacy read-back %1 failed/mismatched: %2 matched, current write unverified, %3 not attempted. Reload required; consult backup.")
				.arg(id, 2, 16, QLatin1Char('0')).arg(matched).arg(total - matched - 1);
			return false;
		}
		++matched;
	}
	if (textChanged) {
		if (!writeText(desiredText)) {
			report = QObject::tr("Custom text write failed: %1 records matched, text state uncertain. Reload required; consult backup.").arg(matched);
			return false;
		}
		QByteArray actual;
		if (!readText(actual) || customText(actual) != text) {
			report = QObject::tr("Custom text read-back failed/mismatched: %1 records matched, text unverified. Reload required; consult backup.").arg(matched);
			return false;
		}
		++matched;
	}
	if (syncClock && !setClock()) {
		report = QObject::tr("Clock sync failed: %1 settings matched immediate read-back, clock state uncertain. Reload required; consult backup (clock not included).").arg(matched);
		return false;
	}
	report = QObject::tr("%1 legacy setting(s) matched immediate read-back. Firmware semantics and persistence are not hardware verified; reload before another edit.").arg(matched);
	if (syncClock)
		report += QObject::tr(" Clock sync acknowledged, not read-back verified; original clock is not in the settings backup.");
	return true;
}

static QJsonObject identityObject(const Ostc4SettingsTransaction::Identity &i)
{
	return { { QStringLiteral("model"), i.model }, { QStringLiteral("serial"), i.serial },
		 { QStringLiteral("firmware"), i.firmware } };
}

bool Ostc4SettingsTransaction::saveBackup(const QString &path, QString &error) const
{
	QJsonArray entries;
	for (size_t n = 0; n < records.size(); ++n) {
		const Record &r = records[n];
		entries.append(QJsonObject { { QStringLiteral("id"), r.id },
			{ QStringLiteral("status"), r.availability == Record::Readable ? QStringLiteral("readable") :
			  r.availability == Record::Unsupported ? QStringLiteral("unsupported") : QStringLiteral("unknown") },
			{ QStringLiteral("writable"), valid && (n >= 3 || legacyCompatible(identity)) && r.availability == Record::Readable },
			{ QStringLiteral("raw"), r.availability == Record::Readable ? QString::fromLatin1(r.original.toHex()) : QString() } });
	}
	QJsonArray notCaptured;
	QJsonArray legacyRecords;
	for (int id = 0x10; id <= 0x44; ++id) {
		if (std::find(ids.begin(), ids.end(), id) != ids.end())
			continue;
		const QByteArray &raw = displayRecords[id - 0x10];
		legacyRecords.append(QJsonObject { { QStringLiteral("id"), id },
			{ QStringLiteral("raw"), raw.size() == 4 ? QString::fromLatin1(raw.toHex()) : QString() } });
		if (raw.size() != 4)
			notCaptured.append(id);
	}
	QJsonObject root { { QStringLiteral("format"), QStringLiteral("Subsurface OSTC4 partial settings backup") },
		{ QStringLiteral("version"), 2 }, { QStringLiteral("identity"), identityObject(identity) },
		{ QStringLiteral("records"), entries },
		{ QStringLiteral("legacyRecords"), legacyRecords },
		{ QStringLiteral("customTextRaw"), QString::fromLatin1(originalCustomText.toHex()) },
		{ QStringLiteral("notCaptured"), notCaptured },
		{ QStringLiteral("coverage"), QStringLiteral("Partial: only listed readable records and available custom text; time and unavailable settings not backed up. Manual recovery only after fresh device read and identity validation.") } };
	QSaveFile file(path);
	if (!file.open(QIODevice::WriteOnly)) {
		error = file.errorString();
		return false;
	}
	const QByteArray bytes = QJsonDocument(root).toJson();
	if (file.write(bytes) != bytes.size() || !file.commit()) {
		error = file.errorString();
		return false;
	}
	return true;
}

bool Ostc4SettingsTransaction::validateBackup(const QString &path, const Identity &expected, QString &description)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
		return false;
	QJsonParseError parse;
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parse);
	const QJsonObject root = doc.object();
	if (parse.error != QJsonParseError::NoError || !doc.isObject() ||
	    root.value(QStringLiteral("format")).toString() != QStringLiteral("Subsurface OSTC4 partial settings backup") ||
	    (root.value(QStringLiteral("version")).toInt() != 1 && root.value(QStringLiteral("version")).toInt() != 2) ||
	    root.value(QStringLiteral("identity")).toObject() != identityObject(expected) ||
	    !root.value(QStringLiteral("records")).isArray())
		return false;
	const QJsonArray entries = root.value(QStringLiteral("records")).toArray();
	const bool version2 = root.value(QStringLiteral("version")).toInt() == 2;
	const QString customHex = root.value(QStringLiteral("customTextRaw")).toString();
	if ((version2 && !root.value(QStringLiteral("customTextRaw")).isString()) ||
	    (!customHex.isEmpty() && (customHex.size() != 120 || QByteArray::fromHex(customHex.toLatin1()).toHex() != customHex.toLatin1())))
		return false;
	description.clear();
	if (!customHex.isEmpty())
		description += QObject::tr("Custom text raw: %1\n").arg(customHex);
	const QJsonArray legacy = root.value(QStringLiteral("legacyRecords")).toArray();
	const QJsonArray notCaptured = root.value(QStringLiteral("notCaptured")).toArray();
	int skipped = 0;
	int legacyIndex = 0;
	for (int id = 0x10; id <= 0x44; ++id) {
		if (std::find(ids.begin(), ids.end(), id) == ids.end()) {
			if (!version2) {
				if (skipped >= notCaptured.size() || notCaptured[skipped++].toInt(-1) != id)
					return false;
				continue;
			}
			if (legacyIndex >= legacy.size() || !legacy[legacyIndex].isObject())
				return false;
			const QJsonObject record = legacy[legacyIndex++].toObject();
			const QString hex = record.value(QStringLiteral("raw")).toString();
			if (record.value(QStringLiteral("id")).toInt(-1) != id ||
			    (!hex.isEmpty() && (hex.size() != 8 || QByteArray::fromHex(hex.toLatin1()).toHex() != hex.toLatin1())))
				return false;
			if (hex.isEmpty()) {
				if (skipped >= notCaptured.size() || notCaptured[skipped++].toInt(-1) != id)
					return false;
			} else {
				description += QStringLiteral("%1: %2\n").arg(id, 2, 16, QLatin1Char('0')).arg(hex);
			}
		}
	}
	if ((version2 && legacyIndex != legacy.size()) || skipped != notCaptured.size() ||
	    entries.size() != static_cast<int>(ids.size()))
		return false;
	for (int n = 0; n < entries.size(); ++n) {
		const QJsonObject r = entries[n].toObject();
		if (r.value(QStringLiteral("id")).toInt(-1) != ids[n])
			return false;
		const QString status = r.value(QStringLiteral("status")).toString();
		const QString hex = r.value(QStringLiteral("raw")).toString();
		if (!r.value(QStringLiteral("writable")).isBool() ||
		    (r.value(QStringLiteral("writable")).toBool() && (status != QStringLiteral("readable") ||
		     (n < 3 && !legacyCompatible(expected)))))
			return false;
		if (status == QStringLiteral("readable")) {
			if (hex.size() != 8 || QByteArray::fromHex(hex.toLatin1()).toHex() != hex.toLatin1())
				return false;
			description += QStringLiteral("%1: %2").arg(ids[n], 2, 16, QLatin1Char('0')).arg(hex);
			const QByteArray raw = QByteArray::fromHex(hex.toLatin1());
			if (n == 3 || n == 4)
				description += QObject::tr(" (setpoint %1: %2 cbar, %3 m)")
					.arg(n + 1).arg(static_cast<unsigned char>(raw[0]))
					.arg(static_cast<unsigned char>(raw[1]));
			if (n == 5 || n == 6)
				description += QObject::tr(" (GF %1: %2%)")
					.arg(n == 5 ? QObject::tr("low") : QObject::tr("high"))
					.arg(static_cast<unsigned char>(raw[0]));
			description += QLatin1Char('\n');
		} else if ((status != QStringLiteral("unsupported") && status != QStringLiteral("unknown")) || !hex.isEmpty()) {
			return false;
		}
	}
	return true;
}
