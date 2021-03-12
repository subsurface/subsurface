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
#include <QTime>
#include <QDebug>
#include <QLoggingCategory>

#include <libdivecomputer/version.h>
#include <libdivecomputer/ble.h>

#include "libdivecomputer.h"
#include "core/qt-ble.h"
#include "core/btdiscovery.h"
#include "core/errorhelper.h"
#include "core/subsurface-string.h"

#define BLE_TIMEOUT 12000 // 12 seconds seems like a very long time to wait
#define DEBUG_THRESHOLD 50
static int debugCounter;

#define IS_HW(_d) same_string((_d)->vendor, "Heinrichs Weikamp")
#define IS_SHEARWATER(_d) same_string((_d)->vendor, "Shearwater")
#define IS_GARMIN(_d) same_string((_d)->vendor, "Garmin")

#define MAXIMAL_HW_CREDIT	255
#define MINIMAL_HW_CREDIT	32

#define WAITFOR(expression, ms) do {					\
	Q_ASSERT(QCoreApplication::instance());				\
	Q_ASSERT(QThread::currentThread());				\
									\
	if (expression)							\
		break;							\
	QElapsedTimer timer;						\
	timer.start();							\
									\
	do {								\
		QCoreApplication::processEvents(QEventLoop::AllEvents, ms); \
		if (expression)						\
			break;						\
		QThread::msleep(10);					\
	} while (timer.elapsed() < (ms));				\
} while (0)

extern "C" {

void BLEObject::serviceStateChanged(QLowEnergyService::ServiceState newState)
{
	if (verbose > 2 || debugCounter < DEBUG_THRESHOLD)
		qDebug() << "serviceStateChanged";
	auto service = qobject_cast<QLowEnergyService*>(sender());
	if (service)
		if (verbose > 2 || debugCounter < DEBUG_THRESHOLD)
			qDebug() << service->serviceUuid() << newState;
}

void BLEObject::characteristcStateChanged(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
	if (verbose > 2 || debugCounter < DEBUG_THRESHOLD)
		qDebug() << QTime::currentTime() << "packet RECV" << value.toHex();
	if (IS_HW(device)) {
		if (c.uuid() == hwAllCharacteristics[HW_OSTC_BLE_DATA_TX]) {
			hw_credit--;
			receivedPackets.append(value);
			if (hw_credit == MINIMAL_HW_CREDIT)
				setHwCredit(MAXIMAL_HW_CREDIT - MINIMAL_HW_CREDIT);
		} else {
			qDebug() << "ignore packet from" << c.uuid() << value.toHex();
		}
	} else {
		receivedPackets.append(value);
	}
}

void BLEObject::characteristicWritten(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
	if (IS_HW(device)) {
		if (c.uuid() == hwAllCharacteristics[HW_OSTC_BLE_CREDITS_RX]) {
			bool ok;
			hw_credit += value.toHex().toInt(&ok, 16);
			isCharacteristicWritten = true;
		}
	} else {
		if (verbose > 2 || debugCounter < DEBUG_THRESHOLD)
			qDebug() << "BLEObject::characteristicWritten";
	}
}

void BLEObject::writeCompleted(const QLowEnergyDescriptor&, const QByteArray&)
{
	if (verbose > 2 || debugCounter < DEBUG_THRESHOLD)
		qDebug() << "BLE write completed";
	desc_written++;
}

struct uuid_match {
	const char *uuid, *details;
};

static const char *match_uuid_list(const QBluetoothUuid &match, const struct uuid_match *array)
{
	const char *uuid;

	while ((uuid = array->uuid) != NULL) {
		if (match == QUuid(uuid))
			return array->details;
		array++;
	}
	return NULL;
}

//
// Known BLE GATT service UUID's that we should prefer for the serial
// emulation.
//
// The Bluetooth SIG is a disgrace, and never standardized any serial
// communication over BLE. They should just have specified a standard
// UUID for serial service, but preferred their idiotic model of
// "vendor specific" garbage instead. So everybody has made their own
// serial protocol over BLE GATT, which all look fairly similar, but
// with pointless and stupid differences just because the BLE SIG
// couldn't be arsed to do their job properly.
//
// Am I bitter? Just a bit. I know that "standards bodies" is just a
// fancy way of saying "incompetent tech politics", but still.. It's
// not like legacy BT didn't have a standard serial encapsulation.
// Oh. It did, didn't it?
//
static const struct uuid_match serial_service_uuids[] = {
	{ "0000fefb-0000-1000-8000-00805f9b34fb", "Heinrichs-Weikamp" },
	{ "544e326b-5b72-c6b0-1c46-41c1bc448118", "Mares BlueLink Pro" },
	{ "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "Nordic Semi UART" },
	{ "98ae7120-e62e-11e3-badd-0002a5d5c51b", "Suunto (EON Steel/Core, G5)" },
	{ "cb3c4555-d670-4670-bc20-b61dbc851e9a", "Pelagic (i770R, i200C, Pro Plus X, Geo 4.0)" },
	{ "fdcdeaaa-295d-470e-bf15-04217b7aa0a0", "ScubaPro G2"},
	{ "fe25c237-0ece-443c-b0aa-e02033e7029d", "Shearwater (Perdix/Teric/Peregrine)" },
	{ NULL, }
};

//
// Sometimes we don't know which is the good service, but we can tell
// that a service is NOT a serial service because we've seen that
// people use it for firmware upgrades.
//
static const struct uuid_match upgrade_service_uuids[] = {
	{ "00001530-1212-efde-1523-785feabcd123", "Nordic Upgrade" },
	{ "9e5d1e47-5c13-43a0-8635-82ad38a1386f", "Broadcom Upgrade #1" },
	{ "a86abc2d-d44c-442e-99f7-80059a873e36", "Broadcom Upgrade #2" },
	{ NULL, }
};

static const char *is_known_serial_service(const QBluetoothUuid &service)
{
	return match_uuid_list(service, serial_service_uuids);
}

static const char *is_known_bad_service(const QBluetoothUuid &service)
{
	return match_uuid_list(service, upgrade_service_uuids);
}

void BLEObject::addService(const QBluetoothUuid &newService)
{
	const char *details;

	qDebug() << "Found service" << newService;

	//
	// Known bad service that we should ignore?
	// (typically firmware update service).
	//
	details = is_known_bad_service(newService);
	if (details) {
		qDebug () << " .. ignoring service" << details;
		return;
	}

	//
	// If it's a known serial service, clear any other previous
	// services we've found - we'll use this one.
	//
	// Note that if it's not _known_ to be good, we'll ignore
	// any standard services. They are usually things like battery
	// status or device name services.
	//
	// But Heinrich-Weicamp actually has a standard service ID in the
	// known good category, because Telit/Stollmann (the manufacturer)
	// applied for a UUID for its product.
	//
	// If it's not a known service, and not a standard one, we'll just
	// add it to the list and then we'll try our heuristics on that
	// list.
	//
	details = is_known_serial_service(newService);
	if (details) {
		qDebug () << " .. recognized service" << details;
		services.clear();
	} else {
		bool isStandardUuid = false;

		newService.toUInt16(&isStandardUuid);
		if (isStandardUuid) {
			qDebug () << " .. ignoring standard service";
			return;
		}
	}

	auto service = controller->createServiceObject(newService, this);
	if (service) {
		// provide some visibility into what's happening in the log
		service->connect(service, &QLowEnergyService::stateChanged,[=](QLowEnergyService::ServiceState newState) {
			qDebug() << "   .. service state changed to" << newState;
		});
		service->connect(service, QOverload<QLowEnergyService::ServiceError>::of(&QLowEnergyService::error),
				 [=](QLowEnergyService::ServiceError newError) {
			qDebug() << "error discovering service details" << newError;
		});
		services.append(service);
		qDebug() << "starting service characteristics discovery";
		service->discoverDetails();
	}
}

BLEObject::BLEObject(QLowEnergyController *c, device_data_t *d)
{
	controller = c;
	device = d;
	debugCounter = 0;
	isCharacteristicWritten = false;
	timeout = BLE_TIMEOUT;
}

BLEObject::~BLEObject()
{
	qDebug() << "Deleting BLE object";

	qDeleteAll(services);

	delete controller;
}

/*
 * The McLean Extreme has just one vendor service, but then inside that
 * service it has several characteristics, and it's not obvious which
 * ones are the read/write ones.
 *
 * So just make sure to skip the ones we don't want.
 *
 * The proper ones are:
 *
 *   Microchip service UUID:  49535343-fe7d-4ae5-8fa9-9fafd205e455
 *            TX characteristic: 49535343-1e4d-4bd9-ba61-23c647249616
 *            RX characteristic: 49535343-8841-43f4-a8d4-ecbe34729bb3
 */
static const struct uuid_match skip_characteristics[] = {
	{ "49535343-6daa-4d02-abf6-19569aca69fe", "McLean Extreme Avoid" },
	{ "49535343-aca3-481c-91ec-d85e28a60318", "McLean Extreme Avoid" },
	{ "49535343-026e-3a9b-954c-97daef17e26e", "McLean Extreme Avoid" },
	{ "49535343-4c8a-39b3-2f49-511cff073b7e", "McLean Extreme Avoid" },
	{ NULL, }
};

// a write characteristic needs Write or WriteNoResponse
static bool is_write_characteristic(const QLowEnergyCharacteristic &c)
{
	if (match_uuid_list(c.uuid(), skip_characteristics))
		return false;
	return c.properties() &
		 (QLowEnergyCharacteristic::Write |
		  QLowEnergyCharacteristic::WriteNoResponse);
}

// We need a Notify or Indicate for the reading side, and
// a descriptor to enable it
static bool is_read_characteristic(const QLowEnergyCharacteristic &c)
{
	if (match_uuid_list(c.uuid(), skip_characteristics))
		return false;
	return !c.descriptors().empty() &&
		(c.properties() &
		  (QLowEnergyCharacteristic::Notify |
		   QLowEnergyCharacteristic::Indicate));
}

dc_status_t BLEObject::write(const void *data, size_t size, size_t *actual)
{
	if (actual) *actual = 0;

	if (!receivedPackets.isEmpty()) {
		qDebug() << ".. write HIT with still incoming packets in queue";
		do {
			receivedPackets.takeFirst();
		} while (!receivedPackets.isEmpty());
	}

	foreach (const QLowEnergyCharacteristic &c, preferredService()->characteristics()) {
		if (!is_write_characteristic(c))
			continue;

		QByteArray bytes((const char *)data, (int) size);
		if (verbose > 2 || debugCounter < DEBUG_THRESHOLD)
			qDebug() << QTime::currentTime() << "packet SEND" << bytes.toHex();

		QLowEnergyService::WriteMode mode;
		mode = (c.properties() & QLowEnergyCharacteristic::WriteNoResponse) ?
				QLowEnergyService::WriteWithoutResponse :
				QLowEnergyService::WriteWithResponse;

		preferredService()->writeCharacteristic(c, bytes, mode);
		if (actual) *actual = size;
		return DC_STATUS_SUCCESS;
	}

	return DC_STATUS_IO;
}

dc_status_t BLEObject::poll(int timeout)
{
	if (receivedPackets.isEmpty()) {
		QList<QLowEnergyCharacteristic> list = preferredService()->characteristics();
		if (list.isEmpty())
			return DC_STATUS_IO;

		if (verbose > 2 || debugCounter < DEBUG_THRESHOLD)
			qDebug() << QTime::currentTime() << "packet WAIT";

		WAITFOR(!receivedPackets.isEmpty(), timeout);
		if (receivedPackets.isEmpty())
			return DC_STATUS_TIMEOUT;
	}

	return DC_STATUS_SUCCESS;
}

dc_status_t BLEObject::read(void *data, size_t size, size_t *actual)
{
	dc_status_t rc;

	if (actual)
		*actual = 0;

	// Wait for a packet
	rc = poll(timeout);
	if (rc != DC_STATUS_SUCCESS)
		return rc;

	QByteArray packet = receivedPackets.takeFirst();

	// Did we get more than asked for?
	//
	// Put back the left-over at the beginning of the
	// received packet list, and truncate the packet
	// we got to just the part asked for.
	if ((size_t)packet.size() > size) {
		receivedPackets.prepend(packet.mid(size));
		packet.truncate(size);
	}

	memcpy((char *)data, packet.data(), packet.size());
	if (actual)
		*actual += packet.size();

	if (verbose > 2 || debugCounter < DEBUG_THRESHOLD)
		qDebug() << QTime::currentTime() << "packet READ" << packet.toHex();

	return DC_STATUS_SUCCESS;
}

//
// select_preferred_service() gets called after all services
// have been discovered, and the discovery process has been
// started (by addService(), which calls service->discoverDetails())
//
// The role of this function is to wait for all service
// discovery to finish, and pick the preferred service.
//
// NOTE! Picking the preferred service is divecomputer-specific.
// Right now we special-case the HW known service number, but for
// others we just pick the first one that isn't a standard service.
//
// That's wrong, but works for the simple case.
//
dc_status_t BLEObject::select_preferred_service(void)
{
	// Wait for each service to finish discovering
	foreach (const QLowEnergyService *s, services) {
		WAITFOR(s->state() != QLowEnergyService::DiscoveringServices, BLE_TIMEOUT);
		if (s->state() == QLowEnergyService::DiscoveringServices)
			qDebug() << " .. service " << s->serviceUuid() << "still hasn't completed discovery - trouble ahead";
	}

	// Print out the services for debugging
	foreach (const QLowEnergyService *s, services) {
		qDebug() << "Found service" << s->serviceUuid() << s->serviceName();

		foreach (const QLowEnergyCharacteristic &c, s->characteristics()) {
			qDebug() << "   c:" << c.uuid();

			foreach (const QLowEnergyDescriptor &d, c.descriptors())
				qDebug() << "        d:" << d.uuid();
		}
	}

	// Pick the preferred one
	foreach (QLowEnergyService *s, services) {
		if (s->state() != QLowEnergyService::ServiceDiscovered)
			continue;

		bool hasread = false;
		bool haswrite = false;
		QBluetoothUuid uuid = s->serviceUuid();

		foreach (const QLowEnergyCharacteristic &c, s->characteristics()) {
			hasread |= is_read_characteristic(c);
			haswrite |= is_write_characteristic(c);
		}

		if (!hasread) {
			qDebug () << " .. ignoring service without read characteristic" << uuid;
			continue;
		}

		if (!haswrite) {
			qDebug () << " .. ignoring service without write characteristic" << uuid;
			continue;
		}

		// We now know that the service has both read and write characteristics
		preferred = s;
		qDebug() << "Using service" << s->serviceUuid() << "as preferred service";
		break;
	}

	if (!preferred) {
		qDebug() << "failed to find suitable service";
		report_error("Failed to find suitable BLE GATT service");
		return DC_STATUS_IO;
	}

	connect(preferred, &QLowEnergyService::stateChanged, this, &BLEObject::serviceStateChanged);
	connect(preferred, &QLowEnergyService::characteristicChanged, this, &BLEObject::characteristcStateChanged);
	connect(preferred, &QLowEnergyService::characteristicWritten, this, &BLEObject::characteristicWritten);
	connect(preferred, &QLowEnergyService::descriptorWritten, this, &BLEObject::writeCompleted);

	return DC_STATUS_SUCCESS;
}

dc_status_t BLEObject::setHwCredit(unsigned int c)
{
	/* The Terminal I/O client transmits initial UART credits to the server (see 6.5).
	 *
	 * Notice that we have to write to the characteristic here, and not to its
	 * descriptor as for the enabeling of notifications or indications.
	 *
	 * Futher notice that this function has the implicit effect of processing the
	 * event loop (due to waiting for the confirmation of the credit request).
	 * So, as characteristcStateChanged will be triggered, while receiving
	 * data from the OSTC, these are processed too.
	 */

	QList<QLowEnergyCharacteristic> list = preferredService()->characteristics();
	isCharacteristicWritten = false;
	preferredService()->writeCharacteristic(list[HW_OSTC_BLE_CREDITS_RX],
						QByteArray(1, c),
						QLowEnergyService::WriteWithResponse);

	/* And wait for the answer*/
	WAITFOR(isCharacteristicWritten, BLE_TIMEOUT);

	if (!isCharacteristicWritten)
		return DC_STATUS_TIMEOUT;
	return DC_STATUS_SUCCESS;
}

dc_status_t BLEObject::setupHwTerminalIo(const QList<QLowEnergyCharacteristic> &allC)
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

	/* The Terminal I/O client transmits initial UART credits to the server (see 6.5). */
	return setHwCredit(MAXIMAL_HW_CREDIT);
}

#if !defined(Q_OS_WIN)
// Bluez is broken, and doesn't have a sane way to query
// whether to use a random address or not. So we have to
// fake it.
static int use_random_address(device_data_t *user_device)
{
	return IS_SHEARWATER(user_device) || IS_GARMIN(user_device);
}
#endif

dc_status_t qt_ble_open(void **io, dc_context_t *, const char *devaddr, device_data_t *user_device)
{
	debugCounter = 0;
	QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));

	/*
	 * LE-only devices get the "LE:" prepended by the scanning
	 * code, so that the rfcomm code can see they only do LE.
	 *
	 * We just skip that prefix (and it doesn't always exist,
	 * since the device may support both legacy BT and LE).
	 */
	if (!strncmp(devaddr, "LE:", 3))
		devaddr += 3;

	// HACK ALERT! Qt 5.9 needs this for proper Bluez operation
	qputenv("QT_DEFAULT_CENTRAL_SERVICES", "1");

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
	QBluetoothDeviceInfo remoteDevice = getBtDeviceInfo(QString(devaddr));
	QLowEnergyController *controller = QLowEnergyController::createCentral(remoteDevice);
#else
	// this is deprecated but given that we don't use Qt to scan for
	// devices on Android, we don't have QBluetoothDeviceInfo for the
	// paired devices and therefore cannot use the newer interfaces
	// that are preferred starting with Qt 5.7
	QBluetoothAddress remoteDeviceAddress(devaddr);
	QLowEnergyController *controller = new QLowEnergyController(remoteDeviceAddress);
#endif
	qDebug() << "qt_ble_open(" << devaddr << ")";

#if !defined(Q_OS_WIN)
	if (use_random_address(user_device))
		controller->setRemoteAddressType(QLowEnergyController::RandomAddress);
#endif

	// Try to connect to the device
	controller->connectToDevice();

	// Create a timer. If the connection doesn't succeed after five seconds or no error occurs then stop the opening step
	WAITFOR(controller->state() != QLowEnergyController::ConnectingState, BLE_TIMEOUT);

	switch (controller->state()) {
	case QLowEnergyController::ConnectedState:
		qDebug() << "connected to the controller for device" << devaddr;
		break;
	case QLowEnergyController::ConnectingState:
		qDebug() << "timeout while trying to connect to the controller " << devaddr;
		report_error("Timeout while trying to connect to %s", devaddr);
		delete controller;
		return DC_STATUS_IO;
	default:
		qDebug() << "failed to connect to the controller " << devaddr << "with error" << controller->errorString();
		report_error("Failed to connect to %s: '%s'", devaddr, qPrintable(controller->errorString()));
		delete controller;
		return DC_STATUS_IO;
	}

	// We need to discover services etc here!
	// Note that ble takes ownership of controller and henceforth deleting ble will
	// take care of deleting controller.
	BLEObject *ble = new BLEObject(controller, user_device);
	// we used to call our addService function the moment a service was discovered, but that
	// could cause us to try to discover the details of a characteristic while we were still serching
	// for services, which can cause a failure in the Qt BLE stack.
	// While that actual error was likely caused by a bug in BLE implementation of a dive computer,
	// the underlying issue still seems worth addressing.
	// Finish discovering the services, then add all those services and discover their characteristics.
	ble->connect(controller, &QLowEnergyController::discoveryFinished, [=] {
		qDebug() << "finished service discovery, start discovering characteristics";
		foreach(QBluetoothUuid s, controller->services()) {
			ble->addService(s);
		}
	});
	ble->connect(controller, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error), [=](QLowEnergyController::Error newError) {
		qDebug() << "controler discovery error" << controller->errorString() << newError;
	});

	controller->discoverServices();

	WAITFOR(controller->state() != QLowEnergyController::DiscoveringState, BLE_TIMEOUT);
	if (controller->state() == QLowEnergyController::DiscoveringState)
		qDebug() << "  .. even after waiting for the full BLE timeout, controller is still in discovering state";

	qDebug() << " .. done discovering services";

	dc_status_t error = ble->select_preferred_service();

	if (error != DC_STATUS_SUCCESS) {
		qDebug() << "failed to find suitable service on" << devaddr;
		report_error("Failed to find suitable service on '%s'", devaddr);
		delete ble;
		return error;
	}

	qDebug() << " .. enabling notifications";

	/* Enable notifications */
	QList<QLowEnergyCharacteristic> list = ble->preferredService()->characteristics();
	if (IS_HW(user_device)) {
		dc_status_t r = ble->setupHwTerminalIo(list);
		if (r != DC_STATUS_SUCCESS) {
			delete ble;
			return r;
		}
	} else {
		for (const QLowEnergyCharacteristic &c: list) {
			if (!is_read_characteristic(c))
				continue;

			qDebug() << "Using read characteristic" << c.uuid();

			const QList<QLowEnergyDescriptor> l = c.descriptors();
			QLowEnergyDescriptor d = l.first();

			for (const QLowEnergyDescriptor &tmp: l) {
				if (tmp.type() == QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration) {
					d = tmp;
					break;
				}
			}

			qDebug() << "now writing \"0x0100\" to the descriptor" << d.uuid().toString();

			ble->preferredService()->writeDescriptor(d, QByteArray::fromHex("0100"));
			WAITFOR(ble->descriptorWritten(), 1000);
			break;
		}
	}

	// Fill in info
	*io = (void *)ble;
	return DC_STATUS_SUCCESS;
}

dc_status_t qt_ble_close(void *io)
{
	BLEObject *ble = (BLEObject *) io;

	delete ble;

	return DC_STATUS_SUCCESS;
}
static void checkThreshold()
{
	if (++debugCounter == DEBUG_THRESHOLD) {
		QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = false"));
		qDebug() << "turning off further BT debug output";
	}
}

/*
 * NOTE! The 'set_timeout()' function only affects the timeout
 * for qt_ble_read(), not for the various general BLE operations.
 */
dc_status_t qt_ble_set_timeout(void *io, int timeout)
{
	BLEObject *ble = (BLEObject *) io;
	ble->set_timeout(timeout);
	return DC_STATUS_SUCCESS;
}

dc_status_t qt_ble_read(void *io, void* data, size_t size, size_t *actual)
{
	checkThreshold();
	BLEObject *ble = (BLEObject *) io;
	return ble->read(data, size, actual);
}

dc_status_t qt_ble_write(void *io, const void* data, size_t size, size_t *actual)
{
	checkThreshold();
	BLEObject *ble = (BLEObject *) io;
	return ble->write(data, size, actual);
}

dc_status_t qt_ble_poll(void *io, int timeout)
{
	BLEObject *ble = (BLEObject *) io;

	return ble->poll(timeout);
}

dc_status_t qt_ble_ioctl(void *io, unsigned int request, void *data, size_t size)
{
	BLEObject *ble = (BLEObject *) io;

	switch (request) {
	case DC_IOCTL_BLE_GET_NAME:
		return ble->get_name((char *) data, size);
	default:
		return DC_STATUS_UNSUPPORTED;
	}
}


} /* extern "C" */
