// SPDX-License-Identifier: GPL-2.0
#ifndef MESSAGEHANDLERMODEL_H
#define MESSAGEHANDLERMODEL_H

#include <QAbstractListModel>


class MessageHandlerModel : public QAbstractListModel {
	Q_OBJECT
public:
	static MessageHandlerModel *self();
	enum MsgTypes {Message = Qt::UserRole + 1, Severity};
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& idx, int role) const override;
	QHash<int, QByteArray> roleNames() const override;
	void addLog(QtMsgType type, const QString& message);

	/* call this to clear the debug data */
	Q_INVOKABLE void reset();

private:
	MessageHandlerModel(QObject *parent = 0);
	struct MessageData {
		QString message;
		QtMsgType type;
	};
	QVector<MessageData> m_data;
};

#endif
