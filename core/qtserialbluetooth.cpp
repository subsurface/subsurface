// SPDX-License-Identifier: GPL-2.0
#include <errno.h>

#include <QtBluetooth/QBluetoothAddress>
#include <QtBluetooth/QBluetoothSocket>
#include <QBluetoothLocalDevice>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>
#include <QThread>

#include <libdivecomputer/version.h>
#include <libdivecomputer/context.h>
#include <libdivecomputer/custom.h>
#include <libdivecomputer/serial.h>

#ifdef BLE_SUPPORT
# include "qt-ble.h"
#endif

QList<QBluetoothUuid> registeredUuids;

void addBtUuid(QBluetoothUuid uuid)
{
	registeredUuids << uuid;
}

extern "C" {
typedef struct qt_serial_t {
	/*
	 * RFCOMM socket used for Bluetooth Serial communication.
	 */
	QBluetoothSocket *socket;
	long timeout;
} qt_serial_t;

static dc_status_t qt_serial_open(qt_serial_t **io, dc_context_t*, const char* devaddr)
{
	// Allocate memory.
	qt_serial_t *serial_port = (qt_serial_t *) malloc (sizeof (qt_serial_t));
	if (serial_port == NULL) {
		return DC_STATUS_NOMEMORY;
	}

	// Default to blocking reads.
	serial_port->timeout = -1;

	// Create a RFCOMM socket
	serial_port->socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol);

	// Wait until the connection succeeds or until an error occurs
	QEventLoop loop;
	loop.connect(serial_port->socket, SIGNAL(connected()), SLOT(quit()));
	loop.connect(serial_port->socket, SIGNAL(error(QBluetoothSocket::SocketError)), SLOT(quit()));

	// Create a timer. If the connection doesn't succeed after five seconds or no error occurs then stop the opening step
	QTimer timer;
	int msec = 5000;
	timer.setSingleShot(true);
	loop.connect(&timer, SIGNAL(timeout()), SLOT(quit()));

	QBluetoothAddress remoteDeviceAddress(devaddr);
#if defined(Q_OS_ANDROID)
	QBluetoothUuid uuid = QBluetoothUuid(QUuid("{00001101-0000-1000-8000-00805f9b34fb}"));
	qDebug() << "connecting to Uuid" << uuid;
	serial_port->socket->setPreferredSecurityFlags(QBluetooth::Security::NoSecurity);
	serial_port->socket->connectToService(remoteDeviceAddress, uuid, QIODevice::ReadWrite | QIODevice::Unbuffered);
#else
	QBluetoothLocalDevice dev;
	QBluetoothUuid uuid = QBluetoothUuid(QUuid("{00001101-0000-1000-8000-00805f9b34fb}"));
	qDebug() << "Linux Bluez connecting to Uuid" << uuid;
	serial_port->socket->connectToService(remoteDeviceAddress, uuid, QIODevice::ReadWrite | QIODevice::Unbuffered);
#endif
	timer.start(msec);
	loop.exec();

	if (serial_port->socket->state() == QBluetoothSocket::SocketState::ConnectingState ||
	    serial_port->socket->state() == QBluetoothSocket::SocketState::ServiceLookupState) {
		// It seems that the connection step took more than expected. Wait another 20 seconds.
		qDebug() << "The connection step took more than expected. Wait another 20 seconds";
		timer.start(4 * msec);
		loop.exec();
	}

	if (serial_port->socket->state() != QBluetoothSocket::SocketState::ConnectedState) {

		// Get the latest error and try to match it with one from libdivecomputer
		QBluetoothSocket::SocketError err = serial_port->socket->error();
		qDebug() << "Failed to connect to device " << devaddr << ". Device state " << serial_port->socket->state() << ". Error: " << err;

		free (serial_port);
		switch(err) {
		case QBluetoothSocket::SocketError::HostNotFoundError:
		case QBluetoothSocket::SocketError::ServiceNotFoundError:
			return DC_STATUS_NODEVICE;
		case QBluetoothSocket::SocketError::UnsupportedProtocolError:
			return DC_STATUS_PROTOCOL;
		case QBluetoothSocket::SocketError::OperationError:
			return DC_STATUS_UNSUPPORTED;
		case QBluetoothSocket::SocketError::NetworkError:
			return DC_STATUS_IO;
		default:
			return DC_STATUS_IO;
		}
	}

	*io = serial_port;

	return DC_STATUS_SUCCESS;
}

static dc_status_t qt_serial_close(void *io)
{
	qt_serial_t *device = (qt_serial_t*) io;

	if (device == NULL)
		return DC_STATUS_SUCCESS;

	if (device->socket == NULL) {
		free(device);
		return DC_STATUS_SUCCESS;
	}

	device->socket->close();

	delete device->socket;
	free(device);

	return DC_STATUS_SUCCESS;
}

static dc_status_t qt_serial_read(void *io, void* data, size_t size, size_t *actual)
{
	qt_serial_t *device = (qt_serial_t*) io;

	if (device == NULL || device->socket == NULL || !actual)
		return DC_STATUS_INVALIDARGS;

	*actual = 0;
	for (;;) {
		int rc;

		if (device->socket->state() != QBluetoothSocket::SocketState::ConnectedState)
			return DC_STATUS_IO;

		rc = device->socket->read((char *) data, size);
		if (rc < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			return DC_STATUS_IO;
		}

		*actual = rc;
		if (rc > 0 || !size)
			return DC_STATUS_SUCCESS;

		// Timeout handling
		QEventLoop loop;
		QTimer timer;
		timer.setSingleShot(true);
		loop.connect(&timer, SIGNAL(timeout()), SLOT(quit()));
		loop.connect(device->socket, SIGNAL(readyRead()), SLOT(quit()));
		timer.start(device->timeout);
		loop.exec();

		if (!timer.isActive())
			return DC_STATUS_TIMEOUT;
	}
}

static dc_status_t qt_serial_write(void *io, const void* data, size_t size, size_t *actual)
{
	qt_serial_t *device = (qt_serial_t*) io;

	if (device == NULL || device->socket == NULL || !actual)
		return DC_STATUS_INVALIDARGS;

	*actual = 0;
	for (;;) {
		int rc;

		if (device->socket->state() != QBluetoothSocket::SocketState::ConnectedState)
			return DC_STATUS_IO;

		rc = device->socket->write((char *) data, size);

		if (rc < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			return DC_STATUS_IO;
		}

		*actual = rc;
		return rc ? DC_STATUS_SUCCESS : DC_STATUS_IO;
	}
}

static dc_status_t qt_serial_poll(void *io, int timeout)
{
	qt_serial_t *device = (qt_serial_t*) io;

	if (!device)
		return DC_STATUS_INVALIDARGS;

	if (!device->socket)
		return DC_STATUS_INVALIDARGS;

	QEventLoop loop;
	QTimer timer;
	timer.setSingleShot(true);
	loop.connect(&timer, SIGNAL(timeout()), SLOT(quit()));
	loop.connect(device->socket, SIGNAL(readyRead()), SLOT(quit()));
	timer.start(timeout);
	loop.exec();

	if (!timer.isActive())
		return DC_STATUS_SUCCESS;
	return DC_STATUS_TIMEOUT;
}

static dc_status_t qt_serial_ioctl(void *io, unsigned int request, void *data, size_t size)
{
	return DC_STATUS_UNSUPPORTED;
}

static dc_status_t qt_serial_purge(void *io, dc_direction_t)
{
	qt_serial_t *device = (qt_serial_t*) io;

	if (device == NULL)
		return DC_STATUS_INVALIDARGS;
	// TODO: add implementation

	return DC_STATUS_SUCCESS;
}

static dc_status_t qt_serial_get_available(void *io, size_t *available)
{
	qt_serial_t *device = (qt_serial_t*) io;

	if (device == NULL || device->socket == NULL)
		return DC_STATUS_INVALIDARGS;

	*available = device->socket->bytesAvailable();

	return DC_STATUS_SUCCESS;
}

/* UNUSED! */
static int qt_serial_get_transmitted(qt_serial_t *device) __attribute__ ((unused));

static int qt_serial_get_transmitted(qt_serial_t *device)
{
	if (device == NULL || device->socket == NULL)
		return DC_STATUS_INVALIDARGS;

	return device->socket->bytesToWrite();
}

static dc_status_t qt_serial_set_timeout(void *io, int timeout)
{
	qt_serial_t *device = (qt_serial_t*) io;

	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	device->timeout = timeout;

	return DC_STATUS_SUCCESS;
}

static dc_status_t qt_custom_sleep(void *io, unsigned int timeout)
{
	QThread::msleep(timeout);
	return DC_STATUS_SUCCESS;
}

#ifdef BLE_SUPPORT
dc_status_t
ble_packet_open(dc_iostream_t **iostream, dc_context_t *context, const char* devaddr, void *userdata)
{
	dc_status_t rc = DC_STATUS_SUCCESS;
	void *io = NULL;

	static const dc_custom_cbs_t callbacks = {
		.set_timeout	= qt_ble_set_timeout,
		.set_break	= nullptr,
		.set_dtr	= nullptr,
		.set_rts	= nullptr,
		.get_lines	= nullptr,
		.get_available	= nullptr,
		.configure	= nullptr,
		.poll		= qt_ble_poll,
		.read		= qt_ble_read,
		.write		= qt_ble_write,
		.ioctl		= qt_ble_ioctl,
		.flush		= nullptr,
		.purge		= nullptr,
		.sleep		= qt_custom_sleep,
		.close		= qt_ble_close,
	};

	rc = qt_ble_open(&io, context, devaddr, (device_data_t *) userdata);
	if (rc != DC_STATUS_SUCCESS) {
		return rc;
	}

	return dc_custom_open (iostream, context, DC_TRANSPORT_BLE, &callbacks, io);
}
#endif /* BLE_SUPPORT */


dc_status_t
rfcomm_stream_open(dc_iostream_t **iostream, dc_context_t *context, const char* devaddr)
{
	dc_status_t rc = DC_STATUS_SUCCESS;
	qt_serial_t *io = NULL;

	static const dc_custom_cbs_t callbacks = {
		.set_timeout	= qt_serial_set_timeout,
		.set_break	= nullptr,
		.set_dtr	= nullptr,
		.set_rts	= nullptr,
		.get_lines	= nullptr,
		.get_available	= qt_serial_get_available,
		.configure	= nullptr,
		.poll		= qt_serial_poll,
		.read		= qt_serial_read,
		.write		= qt_serial_write,
		.ioctl		= qt_serial_ioctl,
		.flush		= nullptr,
		.purge		= qt_serial_purge,
		.sleep		= qt_custom_sleep,
		.close		= qt_serial_close,
	};

	rc = qt_serial_open(&io, context, devaddr);
	if (rc != DC_STATUS_SUCCESS) {
		return rc;
	}

	return dc_custom_open (iostream, context, DC_TRANSPORT_BLUETOOTH, &callbacks, io);
}

}
