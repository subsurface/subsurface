#ifndef DIVELOCATIONMODEL_H
#define DIVELOCATIONMODEL_H

#include <QAbstractListModel>

class LocationInformationModel : public QAbstractListModel {
Q_OBJECT
public:
	static LocationInformationModel *instance();
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index = QModelIndex(), int role = Qt::DisplayRole) const;
public slots:
	void update();
private:
	LocationInformationModel(QObject *obj = 0);
	int internalRowCount;
};

#endif
