// SPDX-License-Identifier: GPL-2.0
#ifndef QT_SERIAL_BLUETOOTH_H
#define QT_SERIAL_BLUETOOTH_H

#include <QtGlobal>

class QBluetoothSocket;

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
// Wait for Qt/BlueZ to finish disconnecting and unregister its client profile.
bool waitForRfcommDisconnect(QBluetoothSocket *socket, int deadlineMs);
#endif

#endif
