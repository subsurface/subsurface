// SPDX-License-Identifier: GPL-2.0
#include <errno.h>

#include <QtBluetooth/QBluetoothAddress>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QThread>
#include <QTimer>
#include <QDebug>

#include <libdivecomputer/version.h>

#include "libdivecomputer.h"
#include "core/qt-ble.h"

#if defined(SSRF_CUSTOM_IO)

#include <libdivecomputer/custom_io.h>

#define BLE_TIMEOUT 12000 // 12 seconds seems like a very long time to wait

extern "C" {

static int device_is_hw(dc_user_device_t *device);

void waitFor(int ms) {
	Q_ASSERT(QCoreApplication::instance());
	Q_ASSERT(QThread::currentThread());

	QElapsedTimer timer;
	timer.start();

	do {
		QCoreApplication::processEvents(QEventLoop::AllEvents, ms);
		QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
		QThread::msleep(10);
	} while (timer.elapsed() < ms);
}

void BLEObject::serviceStateChanged(QLowEnergyService::ServiceState s)
{
	Q_UNUSED(s)

	QList<QLowEnergyCharacteristic> list;

	auto service = qobject_cast<QLowEnergyService*>(sender());
	if (service)
		list = service->characteristics();

	Q_FOREACH(QLowEnergyCharacteristic c, list) {
		qDebug() << "   " << c.uuid().toString();
	}
}

void BLEObject::characteristcStateChanged(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
	Q_UNUSED(c)

	receivedPackets.append(value);
	waitForPacket.exit();
}

void BLEObject::writeCompleted(const QLowEnergyDescriptor &d, const QByteArray &value)
{
	Q_UNUSED(d)
	Q_UNUSED(value)

	qDebug() << "BLE write completed";
}

void BLEObject::addService(const QBluetoothUuid &newService)
{
	qDebug() << "Found service" << newService;
	bool isStandardUuid = false;
	newService.toUInt16(&isStandardUuid);
	if (device_is_hw(device)) {
		/* The HW BT/BLE piece or hardware uses, what we
		 * call here, "a Standard UUID. It is standard because the Telit/Stollmann
		 * manufacturer applied for an own UUID for its product, and this was granted
		 * by the Bluetooth SIG.
		 */
		if (newService != QUuid("{0000fefb-0000-1000-8000-00805f9b34fb}"))
			return; // skip all services except the right one
	} else
	if (isStandardUuid) {
		qDebug () << " .. ignoring standard service";
		return;
	}

	auto service = controller->createServiceObject(newService, this);
	qDebug() << " .. created service object" << service;
	if (service) {
		services.append(service);
		connect(service, &QLowEnergyService::stateChanged, this, &BLEObject::serviceStateChanged);
		connect(service, &QLowEnergyService::characteristicChanged, this, &BLEObject::characteristcStateChanged);
		connect(service, &QLowEnergyService::descriptorWritten, this, &BLEObject::writeCompleted);
		service->discoverDetails();
	}
}

BLEObject::BLEObject(QLowEnergyController *c, dc_user_device_t *d)
{
	controller = c;
	device = d;
}

BLEObject::~BLEObject()
{
	qDebug() << "Deleting BLE object";
}

/* Yeah, I could do the C++ inline member thing */
static int device_is_shearwater(dc_user_device_t *device)
{
	return !strcmp(device->vendor, "Shearwater");
}

static int device_is_hw(dc_user_device_t *device)
{
	return !strcmp(device->vendor, "Heinrichs Weikamp");
}

dc_status_t BLEObject::write(const void *data, size_t size, size_t *actual)
{
	Q_UNUSED(actual) // that seems like it might cause problems

	QList<QLowEnergyCharacteristic> list = preferredService()->characteristics();
	QByteArray bytes((const char *)data, (int) size);

	if (!list.isEmpty()) {
		const QLowEnergyCharacteristic &c = list.constFirst();
		QLowEnergyService::WriteMode mode;

		mode = (c.properties() & QLowEnergyCharacteristic::WriteNoResponse) ?
			QLowEnergyService::WriteWithoutResponse :
			QLowEnergyService::WriteWithResponse;

		if (device_is_shearwater(device))
			bytes.prepend("\1\0", 2);

		preferredService()->writeCharacteristic(c, bytes, mode);
		return DC_STATUS_SUCCESS;
	}

	return DC_STATUS_IO;
}

dc_status_t BLEObject::read(void *data, size_t size, size_t *actual)
{
	if (receivedPackets.isEmpty()) {
		QList<QLowEnergyCharacteristic> list = preferredService()->characteristics();
		if (list.isEmpty())
			return DC_STATUS_IO;

		QTimer timer;
		int msec = BLE_TIMEOUT;
		timer.setSingleShot(true);

		waitForPacket.connect(&timer, SIGNAL(timeout()), SLOT(quit()));
		timer.start(msec);
		waitForPacket.exec();
	}

	// Still no packet?
	if (receivedPackets.isEmpty())
		return DC_STATUS_IO;

	QByteArray packet = receivedPackets.takeFirst();

	if (device_is_shearwater(device))
		packet.remove(0,2);

	if (size > (size_t)packet.size())
		size = packet.size();
	memcpy(data, packet.data(), size);
	*actual = size;
	return DC_STATUS_SUCCESS;
}

int BLEObject::setupHwTerminalIo(QList<QLowEnergyCharacteristic> allC)
{	/* This initalizes the Terminal I/O client as described in
	 * http://www.telit.com/fileadmin/user_upload/products/Downloads/sr-rf/BlueMod/TIO_Implementation_Guide_r04.pdf
	 * Referenced section numbers below are from that document.
	 *
	 * This is for all HW computers, that use referenced BT/BLE hardware module from Telit
	 * (formerly Stollmann). The 16 bit UUID 0xFEFB (or a derived 128 bit UUID starting with
	 * 0x0000FEFB is a clear indication that the OSTC is equipped with this BT/BLE hardware.
	 */

	if (allC.length() != 4) {
		qDebug() << "This should not happen. HW/OSTC BT/BLE device without 4 Characteristics";
		return DC_STATUS_IO;
	}

	/* The Terminal I/O client subscribes to indications of the UART credits TX
	 * characteristic (see 6.4).
	 *
	 * Notice that indications are subscribed to by writing 0x0200 to its descriptor. This
	 * can be understood by looking for Client Characteristic Configuration, Assigned
	 * Number: 0x2902. Enabling/Disabeling is setting the proper bit, and they
	 * differ for indications and notifications.
	 */
	QLowEnergyDescriptor d = allC[HW_OSTC_BLE_CREDITS_TX].descriptors().first();
	preferredService()->writeDescriptor(d, QByteArray::fromHex("0200"));

	/* The Terminal I/O client subscribes to notifications of the UART data TX
	 * characteristic (see 6.2).
	 */
	d = allC[HW_OSTC_BLE_DATA_TX].descriptors().first();
	preferredService()->writeDescriptor(d, QByteArray::fromHex("0100"));

	/* The Terminal I/O client transmits initial UART credits to the server (see 6.5).
	 *
	 * Notice that we have to write to the characteristic here, and not to its
	 * descriptor as for the enabeling of notifications or indications.
	 */
	isCharacteristicWritten = false;
	preferredService()->writeCharacteristic(allC[HW_OSTC_BLE_CREDITS_RX],
						QByteArray(1, 255),
						QLowEnergyService::WriteWithResponse);

	/* And give to OSTC some time to get initialized */
	int msec = 5000;
	while (msec > 0 && !isCharacteristicWritten) {
		waitFor(100);
		msec -= 100;
	};
	if (!isCharacteristicWritten)
		return DC_STATUS_TIMEOUT;

	return DC_STATUS_SUCCESS;
}

dc_status_t qt_ble_open(dc_custom_io_t *io, dc_context_t *context, const char *devaddr)
{
	Q_UNUSED(context)
	/*
	 * LE-only devices get the "LE:" prepended by the scanning
	 * code, so that the rfcomm code can see they only do LE.
	 *
	 * We just skip that prefix (and it doesn't always exist,
	 * since the device may support both legacy BT and LE).
	 */
	if (!strncmp(devaddr, "LE:", 3))
		devaddr += 3;

	QBluetoothAddress remoteDeviceAddress(devaddr);

	// HACK ALERT! Qt 5.9 needs this for proper Bluez operation
	qputenv("QT_DEFAULT_CENTRAL_SERVICES", "1");

	QLowEnergyController *controller = new QLowEnergyController(remoteDeviceAddress);

	qDebug() << "qt_ble_open(" << devaddr << ")";

	if (device_is_shearwater(io->user_device))
		controller->setRemoteAddressType(QLowEnergyController::RandomAddress);

	// Try to connect to the device
	controller->connectToDevice();

	// Create a timer. If the connection doesn't succeed after five seconds or no error occurs then stop the opening step
	int msec = BLE_TIMEOUT;
	while (msec > 0 && controller->state() == QLowEnergyController::ConnectingState) {
		waitFor(100);
		msec -= 100;
	};

	switch (controller->state()) {
	case QLowEnergyController::ConnectedState:
		qDebug() << "connected to the controller for device" << devaddr;
		break;
	default:
		qDebug() << "failed to connect to the controller " << devaddr << "with error" << controller->errorString();
		report_error("Failed to connect to %s: '%s'", devaddr, controller->errorString().toUtf8().data());
		controller->disconnectFromDevice();
		delete controller;
		return DC_STATUS_IO;
	}

	/* We need to discover services etc here! */
	BLEObject *ble = new BLEObject(controller, io->user_device);
	ble->connect(controller, SIGNAL(serviceDiscovered(QBluetoothUuid)), SLOT(addService(QBluetoothUuid)));

	qDebug() << "  .. discovering services";

	controller->discoverServices();

	msec = BLE_TIMEOUT;
	while (msec > 0 && controller->state() == QLowEnergyController::DiscoveringState) {
		waitFor(100);
		msec -= 100;
	};

	qDebug() << " .. done discovering services";
	if (ble->preferredService() == nullptr) {
		qDebug() << "failed to find suitable service on" << devaddr;
		report_error("Failed to find suitable service on '%s'", devaddr);
		controller->disconnectFromDevice();
		delete controller;
		return DC_STATUS_IO;
	}

	qDebug() << " .. discovering details";
	msec = BLE_TIMEOUT;
	while (msec > 0 && ble->preferredService()->state() == QLowEnergyService::DiscoveringServices) {
		waitFor(100);
		msec -= 100;
	};

	if (ble->preferredService()->state() != QLowEnergyService::ServiceDiscovered) {
		qDebug() << "failed to find suitable service on" << devaddr;
		report_error("Failed to find suitable service on '%s'", devaddr);
		controller->disconnectFromDevice();
		delete controller;
		return DC_STATUS_IO;
	}


	qDebug() << " .. enabling notifications";

	/* Enable notifications */
	QList<QLowEnergyCharacteristic> list = ble->preferredService()->characteristics();

	if (!list.isEmpty()) {
		const QLowEnergyCharacteristic &c = list.constLast();

		if (device_is_hw(io->user_device)) {
			ble->setupHwTerminalIo(list);
		} else {
			QList<QLowEnergyDescriptor> l = c.descriptors();

			qDebug() << "Descriptor list with" << l.length() << "elements";

			QLowEnergyDescriptor d;
			foreach(d, l)
				qDebug() << "Descriptor:" << d.name() << "uuid:" << d.uuid().toString();

			if (!l.isEmpty()) {
				d = l.first();
				qDebug() << "now writing \"0x0100\" to the first descriptor";

				ble->preferredService()->writeDescriptor(d, QByteArray::fromHex("0100"));
			}
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
