#ifndef CONNECTIONLISTMODEL_H
#define CONNECTIONLISTMODEL_H

#include <QAbstractListModel>

class ConnectionListModel : public QAbstractListModel {
	Q_OBJECT
public:
	ConnectionListModel(QObject *parent = nullptr);
	QHash<int, QByteArray> roleNames() const override;
	QVariant data(const QModelIndex &index, int role) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	void addAddress(const QString &address);
	void removeAllAddresses();
	int indexOf(const QString &address) const;
private:
	QStringList m_addresses;
};

#endif
