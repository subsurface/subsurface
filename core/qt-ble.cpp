#include <errno.h>

#include <QtBluetooth/QBluetoothAddress>
#include <QLowEnergyController>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

#include <libdivecomputer/version.h>

#include "libdivecomputer.h"
#include "core/qt-ble.h"

#if defined(SSRF_CUSTOM_IO)

#include <libdivecomputer/custom_io.h>

extern "C" {

void BLEObject::serviceStateChanged(QLowEnergyService::ServiceState s)
{
	QList<QLowEnergyCharacteristic> list;

	list = service->characteristics();

	Q_FOREACH(QLowEnergyCharacteristic c, list) {
		fprintf(stderr, "   %s\n", c.uuid().toString().toUtf8().data());
	}
}

void BLEObject::characteristcStateChanged(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
	receivedPackets.append(value);
	waitForPacket.exit();
}

void BLEObject::writeCompleted(const QLowEnergyDescriptor &d, const QByteArray &value)
{
	fprintf(stderr, "Write completed\n");
}

void BLEObject::addService(const QBluetoothUuid &newService)
{
	const char *uuid = newService.toString().toUtf8().data();

	fprintf(stderr, "Found service %s\n", uuid);
	if (uuid[1] == '0') {
		fprintf(stderr, " .. ignoring\n");
		return;
	}
	service = controller->createServiceObject(newService, this);
	fprintf(stderr, " .. created service object %p\n", service);
	if (service) {
		connect(service, &QLowEnergyService::stateChanged, this, &BLEObject::serviceStateChanged);
		connect(service, &QLowEnergyService::characteristicChanged, this, &BLEObject::characteristcStateChanged);
		connect(service, &QLowEnergyService::descriptorWritten, this, &BLEObject::writeCompleted);
		service->discoverDetails();
	}
}

BLEObject::BLEObject(QLowEnergyController *c)
{
	controller = c;
}

BLEObject::~BLEObject()
{
fprintf(stderr, "Deleting BLE object\n");
}

dc_status_t BLEObject::write(const void* data, size_t size, size_t *actual)
{
	QList<QLowEnergyCharacteristic> list = service->characteristics();
	QByteArray bytes((const char *)data, (int) size);

	if (!list.isEmpty()) {
		const QLowEnergyCharacteristic &c = list.constFirst();
		QLowEnergyService::WriteMode mode;

		mode = (c.properties() & QLowEnergyCharacteristic::WriteNoResponse) ?
			QLowEnergyService::WriteWithoutResponse :
			QLowEnergyService::WriteWithResponse;

		service->writeCharacteristic(c, bytes, mode);
		return DC_STATUS_SUCCESS;
	}

	return DC_STATUS_IO;
}

dc_status_t BLEObject::read(void* data, size_t size, size_t *actual)
{
	if (receivedPackets.isEmpty()) {
		QList<QLowEnergyCharacteristic> list = service->characteristics();
		if (list.isEmpty())
			return DC_STATUS_IO;

		const QLowEnergyCharacteristic &c = list.constLast();

		QTimer timer;
		int msec = 5000;
		timer.setSingleShot(true);

		waitForPacket.connect(&timer, SIGNAL(timeout()), SLOT(quit()));
		timer.start(msec);
		waitForPacket.exec();
	}

	// Still no packet?
	if (receivedPackets.isEmpty())
		return DC_STATUS_IO;

	QByteArray packet = receivedPackets.takeFirst();
	if (size > packet.size())
		size = packet.size();
	memcpy(data, packet.data(), size);
	*actual = size;
	return DC_STATUS_SUCCESS;
}

dc_status_t qt_ble_open(dc_custom_io_t *io, dc_context_t *context, const char *devaddr)
{
	QBluetoothAddress remoteDeviceAddress(devaddr);

	// HACK ALERT! Qt 5.9 needs this for proper Bluez operation
	qputenv("QT_DEFAULT_CENTRAL_SERVICES", "1");

	QLowEnergyController *controller = new QLowEnergyController(remoteDeviceAddress);

fprintf(stderr, "qt_ble_open(%s)\n", devaddr);

	// Wait until the connection succeeds or until an error occurs
	QEventLoop loop;
	loop.connect(controller, SIGNAL(connected()), SLOT(quit()));
	loop.connect(controller, SIGNAL(error(QLowEnergyController::Error)), SLOT(quit()));

	// Create a timer. If the connection doesn't succeed after five seconds or no error occurs then stop the opening step
	QTimer timer;
	int msec = 5000;
	timer.setSingleShot(true);
	loop.connect(&timer, SIGNAL(timeout()), SLOT(quit()));

	// Try to connect to the device
	controller->connectToDevice();
	timer.start(msec);
	loop.exec();

	switch (controller->state()) {
	case QLowEnergyController::ConnectedState:
		break;
	default:
		report_error("Failed to connect to %s: '%s'", devaddr, controller->errorString().toUtf8().data());
		controller->disconnectFromDevice();
		delete controller;
		return DC_STATUS_IO;
	}

fprintf(stderr, "Connected to device %s\n", devaddr);

	/* We need to discover services etc here! */
	BLEObject *ble = new BLEObject(controller);
	loop.connect(controller, SIGNAL(discoveryFinished()), SLOT(quit()));
	ble->connect(controller, SIGNAL(serviceDiscovered(QBluetoothUuid)), SLOT(addService(QBluetoothUuid)));

fprintf(stderr, "  .. discovering services\n");

	controller->discoverServices();
	timer.start(msec);
	loop.exec();

fprintf(stderr, " .. done discovering services\n");

fprintf(stderr, " .. discovering details\n");

	timer.start(msec);
	loop.exec();

fprintf(stderr, " .. done waiting\n");

fprintf(stderr, " .. enabling notifications\n");

	/* Enable notifications */
	QList<QLowEnergyCharacteristic> list = ble->service->characteristics();

	if (!list.isEmpty()) {
		const QLowEnergyCharacteristic &c = list.constLast();
		QList<QLowEnergyDescriptor> l = c.descriptors();

fprintf(stderr, "Descriptor list (%p, %d)\n", l, l.length());

		if (!l.isEmpty()) {
			QLowEnergyDescriptor d = l.first();

fprintf(stderr, "Descriptor: %s uuid: %s\n", d.name().toUtf8().data(), d.uuid().toString().toUtf8().data());

			ble->service->writeDescriptor(d, QByteArray::fromHex("0100"));
		}
	}

	// Fill in info
	io->userdata = (void *)ble;
	return DC_STATUS_SUCCESS;
}

dc_status_t qt_ble_close(dc_custom_io_t *io)
{
	BLEObject *ble = (BLEObject *) io->userdata;

	io->userdata = NULL;
	delete ble;

	return DC_STATUS_SUCCESS;
}

dc_status_t qt_ble_read(dc_custom_io_t *io, void* data, size_t size, size_t *actual)
{
	BLEObject *ble = (BLEObject *) io->userdata;
	return ble->read(data, size, actual);
}

dc_status_t qt_ble_write(dc_custom_io_t *io, const void* data, size_t size, size_t *actual)
{
	BLEObject *ble = (BLEObject *) io->userdata;
	return ble->write(data, size, actual);
}

} /* extern "C" */
#endif
