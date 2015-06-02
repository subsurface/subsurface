#ifndef DIVELOCATIONMODEL_H
#define DIVELOCATIONMODEL_H

#include <QAbstractListModel>
#include <stdint.h>

class LocationInformationModel : public QAbstractListModel {
Q_OBJECT
public:
	enum { DIVE_SITE_UUID = Qt::UserRole+1};
	static LocationInformationModel *instance();
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index = QModelIndex(), int role = Qt::DisplayRole) const;
	int32_t addDiveSite(const QString& name, int lat = 0, int lon = 0);
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());

public slots:
	void update();
private:
	LocationInformationModel(QObject *obj = 0);
	int internalRowCount;
};

#endif
