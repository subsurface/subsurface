#ifndef CONNECTIONLISTMODEL_H
#define CONNECTIONLISTMODEL_H

#include <QAbstractListModel>

class ConnectionListModel : public QAbstractListModel {
	Q_OBJECT
public:
	ConnectionListModel(QObject *parent = 0);
	QVariant data(const QModelIndex &index, int role) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	void addAddress(const QString address);
	void removeAllAddresses();
	int indexOf(QString address);
private:
	QStringList m_addresses;
};

#endif
