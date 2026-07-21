// SPDX-License-Identifier: GPL-2.0
#ifndef BLUETOOTHADDRESS_H
#define BLUETOOTHADDRESS_H

#include <QString>
#include <utility>

bool isBluetoothAddress(const QString &address);
bool isBluetoothLowEnergyAddress(const QString &address);
bool isBluetoothClassicAddress(const QString &address);
QString bluetoothAddressWithoutPrefix(const QString &address);
QString extractBluetoothAddress(const QString &address);
std::pair<QString, QString> extractBluetoothNameAddress(const QString &address);

#endif
