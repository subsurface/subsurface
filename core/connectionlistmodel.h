#ifndef CONNECTIONLISTMODEL_H
#define CONNECTIONLISTMODEL_H

#include <QAbstractListModel>

class ConnectionListModel : public QAbstractListModel {
	Q_OBJECT
public:
	enum CLMRole {
		AddressRole = Qt::UserRole + 1
	};
	ConnectionListModel(QObject *parent = 0);
	QHash<int, QByteArray> roleNames() const;
	QVariant data(const QModelIndex &index, int role = AddressRole) const;
	QString address(int idx) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	void addAddress(const QString address);
	void removeAllAddresses();
private:
	QStringList m_addresses;
};

#endif
