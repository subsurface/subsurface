// SPDX-License-Identifier: GPL-2.0
#include "messagehandlermodel.h"

/* based on logging bits from libdivecomputer */
#ifndef __ANDROID__
#define INFO(fmt, ...)	fprintf(stderr, "INFO: " fmt "\n", ##__VA_ARGS__)
#else
#include <android/log.h>
#define INFO(fmt, ...)	__android_log_print(ANDROID_LOG_DEBUG, __FILE__, "INFO: " fmt "\n", ##__VA_ARGS__)
#endif

void logMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	Q_UNUSED(context)
	MessageHandlerModel::self()->addLog(type, msg);
}

MessageHandlerModel * MessageHandlerModel::self()
{
	static MessageHandlerModel *self = new MessageHandlerModel();
	return self;
}

MessageHandlerModel::MessageHandlerModel(QObject *parent)
{
	Q_UNUSED(parent)
	// no more than one message handler.
	qInstallMessageHandler(logMessageHandler);
}

int MessageHandlerModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
	return m_data.size();
}

#include <iostream>

void MessageHandlerModel::addLog(QtMsgType type, const QString& message)
{
	beginInsertRows(QModelIndex(), rowCount(), rowCount());
	m_data.append({message, type});
	endInsertRows();
	INFO("%s", message.toUtf8().constData());
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
