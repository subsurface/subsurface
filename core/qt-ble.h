// SPDX-License-Identifier: GPL-2.0
#ifndef QT_BLE_H
#define QT_BLE_H

#include <QLowEnergyController>
#include <QEventLoop>

class BLEObject : public QObject
{
	Q_OBJECT

public:
	BLEObject(QLowEnergyController *c);
	~BLEObject();
	dc_status_t write(const void* data, size_t size, size_t *actual);
	dc_status_t read(void* data, size_t size, size_t *actual);
	QLowEnergyService *service;

public slots:
	void addService(const QBluetoothUuid &newService);
	void serviceStateChanged(QLowEnergyService::ServiceState s);
	void characteristcStateChanged(const QLowEnergyCharacteristic &c, const QByteArray &value);
	void writeCompleted(const QLowEnergyDescriptor &d, const QByteArray &value);

private:
	QLowEnergyController *controller;
	QList<QByteArray> receivedPackets;
	QEventLoop waitForPacket;
};


extern "C" {
dc_status_t qt_ble_open(dc_custom_io_t *io, dc_context_t *context, const char *name);
dc_status_t qt_ble_read(dc_custom_io_t *io, void* data, size_t size, size_t *actual);
dc_status_t qt_ble_write(dc_custom_io_t *io, const void* data, size_t size, size_t *actual);
dc_status_t qt_ble_close(dc_custom_io_t *io);
}

#endif
