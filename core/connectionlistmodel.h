#ifndef CONNECTIONLISTMODEL_H
#define CONNECTIONLISTMODEL_H

#include <QAbstractListModel>

class ConnectionListModel : public QAbstractListModel {
	Q_OBJECT
public:
	ConnectionListModel(QObject *parent = nullptr);
	QVariant data(const QModelIndex &index, int role) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	void addAddress(const QString &address);
	void removeAllAddresses();
	int indexOf(const QString &address) const;
private:
	QStringList m_addresses;
};

#endif
