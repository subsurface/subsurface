#include "messagehandlermodel.h"

void logMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	MessageHandlerModel::self()->addLog(type, msg);
}

MessageHandlerModel * MessageHandlerModel::self()
{
	static MessageHandlerModel *self = new MessageHandlerModel();
	return self;
}

MessageHandlerModel::MessageHandlerModel(QObject *parent)
{
	// no more than one message handler.
	qInstallMessageHandler(logMessageHandler);
}

int MessageHandlerModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return m_data.size();
}

#include <iostream>

void MessageHandlerModel::addLog(QtMsgType type, const QString& message)
{
	beginInsertRows(QModelIndex(), rowCount(), rowCount());
	m_data.append({message, type});
	endInsertRows();
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
