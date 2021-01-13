// SPDX-License-Identifier: GPL-2.0
#include "messagehandlermodel.h"
#include "core/qthelper.h"

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
	if (!m_data.isEmpty()) {
		struct MessageData *lm = &m_data.last();
		QString lastMessage = lm->message.mid(lm->message.indexOf(':'));
		QString newMessage = message.mid(message.indexOf(':'));
		if (lastMessage == newMessage)
			return;
	}
	// filter extremely noisy and unhelpful messages
	if (message.contains("Updating RSSI for") || (message.contains(QRegExp(".*kirigami.*onFoo properties in Connections"))))
		return;

	beginInsertRows(QModelIndex(), rowCount(), rowCount());
	m_data.append({message, type});
	endInsertRows();
	SSRF_INFO("%s", qPrintable(message));
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
