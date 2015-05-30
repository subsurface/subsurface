#ifndef DIVELOCATIONMODEL_H
#define DIVELOCATIONMODEL_H

#include <QAbstractListModel>

class LocationInformationModel : public QAbstractListModel {
Q_OBJECT
public:
	LocationInformationModel(QObject *obj = 0);
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index = QModelIndex(), int role = Qt::DisplayRole) const;
	void update();
private:
	int internalRowCount;
};

#endif
