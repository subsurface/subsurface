#ifndef DIVELISTMODEL_H
#define DIVELISTMODEL_H

#include <QAbstractListModel>
#include "dive.h"
#include "helpers.h"

class DiveListModel : public QAbstractListModel
{
	Q_OBJECT
public:

	enum DiveListRoles {
		DiveNumberRole = Qt::UserRole + 1,
		DiveTripRole,
		DiveDateRole,
		DiveDateStringRole,
		DiveRatingRole,
		DiveDepthRole,
		DiveDurationRole,
		DiveWaterTemperatureRole,
		DiveAirTemperatureRole,
		DiveWeightRole,
		DiveSuitRole,
		DiveCylinderRole,
		DiveGasRole,
		DiveSacRole,
		DiveLocationRole,
		DiveNotesRole,
		DiveBuddyRole,
		DiveMasterRole,
		DiveIdRole
	};

	static DiveListModel *instance();
	DiveListModel(QObject *parent = 0);
	void addDive(dive *d);
	void updateDive(dive *d);
	void clear();
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QHash<int, QByteArray> roleNames() const;
	void startAddDive();
private:
	QList<Dive> m_dives;
	static DiveListModel *m_instance;
};

#endif // DIVELISTMODEL_H
