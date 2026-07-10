// SPDX-License-Identifier: GPL-2.0
#include "bluetoothaddress.h"

#include "errorhelper.h"
#include <QRegularExpression>

bool isBluetoothAddress(const QString &address)
{
	return !extractBluetoothAddress(address).isEmpty();
}

bool isBluetoothLowEnergyAddress(const QString &address)
{
	return address.startsWith("LE:", Qt::CaseInsensitive) && extractBluetoothAddress(address) == address.trimmed();
}

bool isBluetoothClassicAddress(const QString &address)
{
	return address.startsWith("BT:", Qt::CaseInsensitive) && extractBluetoothAddress(address) == address.trimmed();
}

QString bluetoothAddressWithoutPrefix(const QString &address)
{
	return isBluetoothLowEnergyAddress(address) || isBluetoothClassicAddress(address) ? address.mid(3) : address;
}

QString extractBluetoothAddress(const QString &address)
{
	// UUIDs identify BLE devices on Apple platforms and cannot be forced to Classic.
	if (address.contains("BT:{", Qt::CaseInsensitive))
		return {};

	QRegularExpression compactMac("^(?:(?:LE|BT):)?[0-9A-F]{12}$", QRegularExpression::CaseInsensitiveOption);
	if (compactMac.match(address.trimmed()).hasMatch())
		return address.trimmed();

	static const QString macAddress = "(?:(?:[0-9A-F]{2}:){5}[0-9A-F]{2}|(?:[0-9A-F]{2}-){5}[0-9A-F]{2})";
	QRegularExpression re(QString("(?:(?:LE|BT):)?%1|(?:LE:)?{[0-9A-F]{8}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{12}}").arg(macAddress),
			      QRegularExpression::CaseInsensitiveOption);
	return re.match(address).captured(0);
}

std::pair<QString, QString> extractBluetoothNameAddress(const QString &address)
{
	QString extractedAddress = extractBluetoothAddress(address);
	if (extractedAddress == address.trimmed())
		return { extractedAddress, QString() };
	if (extractedAddress.isEmpty()) {
		report_info("can't parse address %s", qPrintable(address));
		return { QString(), QString() };
	}

	QRegularExpression re("^([^()]+)\\(([^)]*\\))$");
	QRegularExpressionMatch m = re.match(address);
	if (m.hasMatch())
		return { extractedAddress, m.captured(1).trimmed() };
	report_info("can't parse address %s", qPrintable(address));
	return { QString(), QString() };
}
