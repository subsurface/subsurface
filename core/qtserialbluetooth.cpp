#include <errno.h>

#include <QtBluetooth/QBluetoothAddress>
#include <QtBluetooth/QBluetoothSocket>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

#include <libdivecomputer/version.h>

#if defined(SSRF_CUSTOM_SERIAL)

#if defined(Q_OS_WIN)
	#include <winsock2.h>
	#include <windows.h>
	#include <ws2bth.h>
#endif

#include <libdivecomputer/custom_serial.h>

extern "C" {
typedef struct qt_serial_t {
	/*
	 * RFCOMM socket used for Bluetooth Serial communication.
	 */
#if defined(Q_OS_WIN)
	SOCKET socket;
#else
	QBluetoothSocket *socket;
#endif
	long timeout;
} qt_serial_t;

static dc_status_t qt_serial_open(void **userdata, const char* devaddr)
{
	// Allocate memory.
	qt_serial_t *serial_port = (qt_serial_t *) malloc (sizeof (qt_serial_t));
	if (serial_port == NULL) {
		return DC_STATUS_NOMEMORY;
	}

	// Default to blocking reads.
	serial_port->timeout = -1;

#if defined(Q_OS_WIN)
	// Create a RFCOMM socket
	serial_port->socket = ::socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);

	if (serial_port->socket == INVALID_SOCKET) {
		free(serial_port);
		return DC_STATUS_IO;
	}

	SOCKADDR_BTH socketBthAddress;
	int socketBthAddressBth = sizeof (socketBthAddress);
	char *address = strdup(devaddr);

	ZeroMemory(&socketBthAddress, socketBthAddressBth);
	qDebug() << "Trying to connect to address " << devaddr;

	if (WSAStringToAddressA(address,
				AF_BTH,
				NULL,
				(LPSOCKADDR) &socketBthAddress,
				&socketBthAddressBth
				) != 0) {
		qDebug() << "FAiled to convert the address " << address;
		free(address);

		return DC_STATUS_IO;
	}

	free(address);

	socketBthAddress.addressFamily = AF_BTH;
	socketBthAddress.port = BT_PORT_ANY;
	memset(&socketBthAddress.serviceClassId, 0, sizeof(socketBthAddress.serviceClassId));
	socketBthAddress.serviceClassId = SerialPortServiceClass_UUID;

	// Try to connect to the device
	if (::connect(serial_port->socket,
		      (struct sockaddr *) &socketBthAddress,
		      socketBthAddressBth
		      ) != 0) {
		qDebug() << "Failed to connect to device";

		return DC_STATUS_NODEVICE;
	}

	qDebug() << "Succesfully connected to device";
#else
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

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
	// First try to connect on RFCOMM channel 1. This is the default channel for most devices
	QBluetoothAddress remoteDeviceAddress(devaddr);
	serial_port->socket->connectToService(remoteDeviceAddress, 1, QIODevice::ReadWrite | QIODevice::Unbuffered);
	timer.start(msec);
	loop.exec();

	if (serial_port->socket->state() == QBluetoothSocket::ConnectingState) {
		// It seems that the connection on channel 1 took more than expected. Wait another 15 seconds
		qDebug() << "The connection on RFCOMM channel number 1 took more than expected. Wait another 15 seconds.";
		timer.start(3 * msec);
		loop.exec();
	} else if (serial_port->socket->state() == QBluetoothSocket::UnconnectedState) {
		// Try to connect on channel number 5. Maybe this is a Shearwater Petrel2 device.
		qDebug() << "Connection on channel 1 failed. Trying on channel number 5.";
		serial_port->socket->connectToService(remoteDeviceAddress, 5, QIODevice::ReadWrite | QIODevice::Unbuffered);
		timer.start(msec);
		loop.exec();

		if (serial_port->socket->state() == QBluetoothSocket::ConnectingState) {
			// It seems that the connection on channel 5 took more than expected. Wait another 15 seconds
			qDebug() << "The connection on RFCOMM channel number 5 took more than expected. Wait another 15 seconds.";
			timer.start(3 * msec);
			loop.exec();
		}
	}
#elif defined(Q_OS_ANDROID) || (QT_VERSION >= 0x050500 && defined(Q_OS_MAC))
	// Try to connect to the device using the uuid of the Serial Port Profile service
	QBluetoothAddress remoteDeviceAddress(devaddr);
	serial_port->socket->connectToService(remoteDeviceAddress, QBluetoothUuid(QBluetoothUuid::SerialPort));
	timer.start(msec);
	loop.exec();

	if (serial_port->socket->state() == QBluetoothSocket::ConnectingState ||
	    serial_port->socket->state() == QBluetoothSocket::ServiceLookupState) {
		// It seems that the connection step took more than expected. Wait another 20 seconds.
		qDebug() << "The connection step took more than expected. Wait another 20 seconds";
		timer.start(4 * msec);
		loop.exec();
	}
#endif
	if (serial_port->socket->state() != QBluetoothSocket::ConnectedState) {

		// Get the latest error and try to match it with one from libdivecomputer
		QBluetoothSocket::SocketError err = serial_port->socket->error();
		qDebug() << "Failed to connect to device " << devaddr << ". Device state " << serial_port->socket->state() << ". Error: " << err;

		free (serial_port);
		switch(err) {
		case QBluetoothSocket::HostNotFoundError:
		case QBluetoothSocket::ServiceNotFoundError:
			return DC_STATUS_NODEVICE;
		case QBluetoothSocket::UnsupportedProtocolError:
			return DC_STATUS_PROTOCOL;
#if QT_VERSION >= 0x050400
		case QBluetoothSocket::OperationError:
			return DC_STATUS_UNSUPPORTED;
#endif
		case QBluetoothSocket::NetworkError:
			return DC_STATUS_IO;
		default:
			return DC_STATUS_IO;
		}
	}
#endif
	*userdata = serial_port;

	return DC_STATUS_SUCCESS;
}

static dc_status_t qt_serial_close(void **userdata)
{
	qt_serial_t *device = (qt_serial_t*) *userdata;

	if (device == NULL)
		return DC_STATUS_SUCCESS;

#if defined(Q_OS_WIN)
	// Cleanup
	closesocket(device->socket);
	free(device);
#else
	if (device->socket == NULL) {
		free(device);
		return DC_STATUS_SUCCESS;
	}

	device->socket->close();

	delete device->socket;
	free(device);
#endif

	*userdata = NULL;

	return DC_STATUS_SUCCESS;
}

static dc_status_t qt_serial_read(void **userdata, void* data, size_t size, size_t *actual)
{
	qt_serial_t *device = (qt_serial_t*) *userdata;

#if defined(Q_OS_WIN)
	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	size_t nbytes = 0;
	int rc;

	while (nbytes < size) {
		rc = recv (device->socket, (char *) data + nbytes, size - nbytes, 0);

		if (rc < 0) {
			return DC_STATUS_IO; // Error during recv call.
		} else if (rc == 0) {
			break; // EOF reached.
		}

		nbytes += rc;
	}
#else
	if (device == NULL || device->socket == NULL)
		return DC_STATUS_INVALIDARGS;

	size_t nbytes = 0;
	int rc;

	while(nbytes < size && device->socket->state() == QBluetoothSocket::ConnectedState)
	{
		rc = device->socket->read((char *) data + nbytes, size - nbytes);

		if (rc < 0) {
			if (errno == EINTR || errno == EAGAIN)
			    continue; // Retry.

			return DC_STATUS_IO; // Something really bad happened :-(
		} else if (rc == 0) {
			// Wait until the device is available for read operations
			QEventLoop loop;
			QTimer timer;
			timer.setSingleShot(true);
			loop.connect(&timer, SIGNAL(timeout()), SLOT(quit()));
			loop.connect(device->socket, SIGNAL(readyRead()), SLOT(quit()));
			timer.start(device->timeout);
			loop.exec();

			if (!timer.isActive())
				break;
		}

		nbytes += rc;
	}
#endif
	if (actual)
		*actual = nbytes;

	return DC_STATUS_SUCCESS;
}

static dc_status_t qt_serial_write(void **userdata, const void* data, size_t size, size_t *actual)
{
	qt_serial_t *device = (qt_serial_t*) *userdata;

#if defined(Q_OS_WIN)
	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	size_t nbytes = 0;
	int rc;

	while (nbytes < size) {
	    rc = send(device->socket, (char *) data + nbytes, size - nbytes, 0);

	    if (rc < 0) {
	       return -1; // Error during send call.
	    }

	    nbytes += rc;
	}
#else
	if (device == NULL || device->socket == NULL)
		return DC_STATUS_INVALIDARGS;

	size_t nbytes = 0;
	int rc;

	while(nbytes < size && device->socket->state() == QBluetoothSocket::ConnectedState)
	{
		rc = device->socket->write((char *) data + nbytes, size - nbytes);

		if (rc < 0) {
			if (errno == EINTR || errno == EAGAIN)
			    continue; // Retry.

			return DC_STATUS_IO; // Something really bad happened :-(
		} else if (rc == 0) {
			break;
		}

		nbytes += rc;
	}
#endif
	if (actual)
		*actual = nbytes;

	return DC_STATUS_SUCCESS;
}

static dc_status_t qt_serial_flush(void **userdata, dc_direction_t queue)
{
	qt_serial_t *device = (qt_serial_t*) *userdata;
	(void)queue;
	if (device == NULL)
		return DC_STATUS_INVALIDARGS;
#if !defined(Q_OS_WIN)
	if (device->socket == NULL)
		return DC_STATUS_INVALIDARGS;
#endif
	// TODO: add implementation

	return DC_STATUS_SUCCESS;
}

static dc_status_t qt_serial_get_received(void **userdata, size_t *available)
{
	qt_serial_t *device = (qt_serial_t*) *userdata;
#if defined(Q_OS_WIN)
	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	// TODO use WSAIoctl to get the information

	*available = 0;
#else
	if (device == NULL || device->socket == NULL)
		return DC_STATUS_INVALIDARGS;

	*available = device->socket->bytesAvailable();
#endif

	return DC_STATUS_SUCCESS;
}

static int qt_serial_get_transmitted(qt_serial_t *device)
{
#if defined(Q_OS_WIN)
	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	// TODO add implementation

	return 0;
#else
	if (device == NULL || device->socket == NULL)
		return DC_STATUS_INVALIDARGS;

	return device->socket->bytesToWrite();
#endif
}

static dc_status_t qt_serial_set_timeout(void **userdata, long timeout)
{
	qt_serial_t *device = (qt_serial_t*) *userdata;

	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	device->timeout = timeout;

	return DC_STATUS_SUCCESS;
}

dc_custom_serial_t qt_serial_ops = {
	.userdata = NULL,
	.open = qt_serial_open,
	.close = qt_serial_close,
	.read = qt_serial_read,
	.write = qt_serial_write,
	.purge = qt_serial_flush,
	.get_available = qt_serial_get_received,
	.set_timeout = qt_serial_set_timeout,
// These doesn't make sense over bluetooth
// NULL means NOP
	.configure = NULL,
	.set_dtr = NULL,
	.set_rts = NULL,
	.set_halfduplex = NULL,
	.set_break = NULL
};

dc_custom_serial_t* get_qt_serial_ops() {
	return (dc_custom_serial_t*) &qt_serial_ops;
}

}
#endif
