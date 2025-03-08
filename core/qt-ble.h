// SPDX-License-Identifier: GPL-2.0
#ifndef QT_BLE_H
#define QT_BLE_H

#include <stddef.h>
#include "core/libdivecomputer.h"
#include <QVector>
#include <QLowEnergyController>
#include <QEventLoop>

#define TELIT_DATA_RX    0
#define TELIT_DATA_TX    1
#define TELIT_CREDITS_RX 2
#define TELIT_CREDITS_TX 3

#define UBLOX_DATA_RX    0
#define UBLOX_DATA_TX    0
#define UBLOX_CREDITS_RX 1
#define UBLOX_CREDITS_TX 1

class BLEObject : public QObject
{
	Q_OBJECT
public:
	BLEObject(QLowEnergyController *c, device_data_t &);
	~BLEObject();
	inline void set_timeout(int value) { timeout = value; }
	dc_status_t write(const void* data, size_t size, size_t *actual);
	dc_status_t read(void* data, size_t size, size_t *actual);
	dc_status_t get_name(char *res, size_t size);
	dc_status_t read_characteristic(const QBluetoothUuid &uuid, char *res, size_t size);
	dc_status_t poll(int timeout);

	inline QLowEnergyService *preferredService() { return preferred; }
	inline int descriptorWritten() { return desc_written; }
	dc_status_t select_preferred_service();

public slots:
	void addService(const QBluetoothUuid &newService);
	void serviceStateChanged(QLowEnergyService::ServiceState s);
	void characteristcStateChanged(const QLowEnergyCharacteristic &c, const QByteArray &value);
	void characteristicWritten(const QLowEnergyCharacteristic &c, const QByteArray &value);
	void writeCompleted(const QLowEnergyDescriptor &d, const QByteArray &value);
	dc_status_t setupHwTerminalIo(const QList<QLowEnergyCharacteristic> &allC);
	dc_status_t setHwCredit(unsigned int c);
private:
	QVector<QLowEnergyService *> services;

	QLowEnergyController *controller;
	QLowEnergyService *preferred = nullptr;
	QList<QByteArray> receivedPackets;
	bool isCharacteristicWritten;
	device_data_t &device;
	unsigned int hw_credit = 0;
	unsigned int desc_written = 0;
	int timeout;

	QList<QBluetoothUuid> telit = {
		QBluetoothUuid(QUuid("{00000001-0000-1000-8000-008025000000}")), // TELIT_DATA_RX
		QBluetoothUuid(QUuid("{00000002-0000-1000-8000-008025000000}")), // TELIT_DATA_TX
		QBluetoothUuid(QUuid("{00000003-0000-1000-8000-008025000000}")), // TELIT_CREDITS_RX
		QBluetoothUuid(QUuid("{00000004-0000-1000-8000-008025000000}"))  // TELIT_CREDITS_TX
	};

	QList<QBluetoothUuid> ublox = {
		QBluetoothUuid(QUuid("{2456e1b9-26e2-8f83-e744-f34f01e9d703}")), // UBLOX_DATA_RX, UBLOX_DATA_TX
		QBluetoothUuid(QUuid("{2456e1b9-26e2-8f83-e744-f34f01e9d704}"))  // UBLOX_CREDITS_RX, UBLOX_CREDITS_TX
	};
};

dc_status_t qt_ble_open(void **io, dc_context_t *context, const char *devaddr, device_data_t *user_device);
dc_status_t qt_ble_set_timeout(void *io, int timeout);
dc_status_t qt_ble_poll(void *io, int timeout);
dc_status_t qt_ble_read(void *io, void* data, size_t size, size_t *actual);
dc_status_t qt_ble_write(void *io, const void* data, size_t size, size_t *actual);
dc_status_t qt_ble_ioctl(void *io, unsigned int request, void *data, size_t size);
dc_status_t qt_ble_close(void *io);

#endif
