// SPDX-License-Identifier: GPL-2.0
#ifndef QT_BLE_H
#define QT_BLE_H

#include <stddef.h>
#include "core/libdivecomputer.h"
#include <QVector>
#include <QLowEnergyController>
#include <QEventLoop>

#define HW_OSTC_BLE_DATA_RX	0
#define HW_OSTC_BLE_DATA_TX	1
#define HW_OSTC_BLE_CREDITS_RX	2
#define HW_OSTC_BLE_CREDITS_TX	3

class BLEObject : public QObject
{
	Q_OBJECT

public:
	BLEObject(QLowEnergyController *c, dc_user_device_t *);
	~BLEObject();
	dc_status_t write(const void* data, size_t size, size_t *actual);
	dc_status_t read(void* data, size_t size, size_t *actual);

	//TODO: need better mode of selecting the desired service than below
	inline QLowEnergyService *preferredService()
				{ return services.isEmpty() ? nullptr : services[0]; }

public slots:
	void addService(const QBluetoothUuid &newService);
	void serviceStateChanged(QLowEnergyService::ServiceState s);
	void characteristcStateChanged(const QLowEnergyCharacteristic &c, const QByteArray &value);
	void characteristicWritten(const QLowEnergyCharacteristic &c, const QByteArray &value);
	void writeCompleted(const QLowEnergyDescriptor &d, const QByteArray &value);
	dc_status_t setupHwTerminalIo(QList<QLowEnergyCharacteristic>);
	dc_status_t setHwCredit(unsigned int c);
private:
	QVector<QLowEnergyService *> services;

	QLowEnergyController *controller = nullptr;
	QList<QByteArray> receivedPackets;
	bool isCharacteristicWritten;
	dc_user_device_t *device;
	unsigned int hw_credit = 0;

	QList<QUuid> hwAllCharacteristics = {
		"{00000001-0000-1000-8000-008025000000}", // HW_OSTC_BLE_DATA_RX
		"{00000002-0000-1000-8000-008025000000}", // HW_OSTC_BLE_DATA_TX
		"{00000003-0000-1000-8000-008025000000}", // HW_OSTC_BLE_CREDITS_RX
		"{00000004-0000-1000-8000-008025000000}"  // HW_OSTC_BLE_CREDITS_TX
	};
};


extern "C" {
dc_status_t qt_ble_open(void **io, dc_context_t *context, const char *devaddr, dc_user_device_t *user_device);
dc_status_t qt_ble_read(void *io, void* data, size_t size, size_t *actual);
dc_status_t qt_ble_write(void *io, const void* data, size_t size, size_t *actual);
dc_status_t qt_ble_close(void *io);
}

#endif
