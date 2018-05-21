// SPDX-License-Identifier: GPL-2.0
#include "messagehandlermodel.h"

/* based on logging bits from libdivecomputer */
#if !defined(Q_OS_ANDROID)
#define INFO(fmt, ...)	fprintf(stderr, "INFO: " fmt "\n", ##__VA_ARGS__)
#else
#include <android/log.h>
#define INFO(fmt, ...)	__android_log_print(ANDROID_LOG_DEBUG, __FILE__, "INFO: " fmt "\n", ##__VA_ARGS__);
#endif

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
extern void writeToAppLogFile(QString logText);
#endif

void logMessageHandler(QtMsgType type, const QMessageLogContext&, const QString &msg)
{
	MessageHandlerModel::self()->addLog(type, msg);
}

MessageHandlerModel * MessageHandlerModel::self()
{
	static MessageHandlerModel *self = new MessageHandlerModel();
	return self;
}

MessageHandlerModel::MessageHandlerModel(QObject*)
{
	// no more than one message handler.
	qInstallMessageHandler(logMessageHandler);
}

int MessageHandlerModel::rowCount(const QModelIndex&) const
{
	return m_data.size();
}

#include <iostream>

void MessageHandlerModel::addLog(QtMsgType type, const QString& message)
{
	beginInsertRows(QModelIndex(), rowCount(), rowCount());
	m_data.append({message, type});
	endInsertRows();
	INFO("%s", qPrintable(message));
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
	writeToAppLogFile(message);
#endif
}

const QString MessageHandlerModel::logAsString()
{
	QString copyString;

	// Loop through m_data and build big string to be put on the clipboard
	for(const MessageData &data: m_data)
		copyString += data.message + "\n";
	return copyString;
}
QVariant MessageHandlerModel::data(const QModelIndex& idx, int role) const
{
	switch(role) {
		case Message:
		case Qt::DisplayRole:
			return m_data.at(idx.row()).message;
	}
	return QVariant(QString("Role: %1").arg(role));
};

QHash<int, QByteArray> MessageHandlerModel::roleNames() const {
	static QHash<int, QByteArray> roles = {
		{Message, "message"},
		{Severity, "severity"}
	};
	return roles;
};

void MessageHandlerModel::reset()
{
	beginRemoveRows(QModelIndex(), 0, m_data.size()-1);
	m_data.clear();
	endRemoveRows();

}
