// SPDX-License-Identifier: GPL-2.0
#ifndef OSTC4SETTINGSTRANSACTION_H
#define OSTC4SETTINGSTRANSACTION_H

#include <QByteArray>
#include <QString>
#include <array>
#include <functional>

// The source-backed editor deliberately supports only these records. The other
// settings are not inferred from a successful 0x72 response.
struct Ostc4SettingsTransaction {
	struct Identity {
		quint8 model = 0;
		quint16 serial = 0;
		quint16 firmware = 0;
		bool operator==(const Identity &other) const
		{
			return model == other.model && serial == other.serial && firmware == other.firmware;
		}
	};
	struct Record {
		quint8 id;
		QByteArray original;
		enum Availability { Unknown, Unsupported, Readable } availability = Unknown;
	};
	using Read = std::function<bool(quint8, QByteArray &)>;
	using Write = std::function<bool(quint8, const QByteArray &)>;
	using Identify = std::function<bool(Identity &)>;
	using Backup = std::function<bool(const Ostc4SettingsTransaction &)>;
	using ReadReply = std::function<bool(unsigned char *, unsigned int)>;
	static constexpr std::array<quint8, 7> ids {{ 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x25, 0x26 }};
	Identity identity;
	std::array<Record, 7> records;
	// Additional 0x72 replies for display only. Never used as a write baseline
	// or represented by the partial raw backup.
	std::array<QByteArray, 0x35> displayRecords {};
	QByteArray originalCustomText; // 60 bytes of the 0x69 reply, when available
	bool valid = false;

	static bool supported(const Identity &identity);
	static bool legacyCompatible(const Identity &identity);
	// Request the libdc hardware2 (5 bytes) and version (64 bytes) replies.
	static bool readIdentity(const ReadReply &hardware, const ReadReply &version, Identity &identity);
	static bool isOstc4Hardware(const QByteArray &hardware);
	// Decode the actual 0x60 hardware2 and 0x69 identity replies. A bootloader
	// reply has a zero serial and firmware, even though it acknowledges writes.
	static bool decodeIdentity(const QByteArray &hardware, const QByteArray &version, Identity &identity);
	static QString firmwareString(quint16 firmware);
	static QString customText(const QByteArray &raw);
	bool capture(const Identify &identify, const Read &read);
	// desired is indexed by ids; untouched bytes must match original. Every
	// edited record is prevalidated before the backup or first device write.
	bool apply(const Identify &identify, const Read &read, const Write &write,
		   const Backup &backup, const std::array<QByteArray, 7> &desired, QString &report) const;
	bool applyLegacy(const Identify &identify, const Read &read, const Write &write,
		const Backup &backup, const std::array<QByteArray, 0x35> &desired, QString &report,
		const QString &text = {}, const std::function<bool(QByteArray &)> &readText = {},
		const std::function<bool(const QByteArray &)> &writeText = {},
		bool syncClock = false, const std::function<bool()> &setClock = {}) const;
	bool saveBackup(const QString &path, QString &error) const;
	static bool validateBackup(const QString &path, const Identity &expected, QString &description);
};

#endif
