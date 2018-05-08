#ifndef VEINFOMODEL_H
#define VEINFOMODEL_H


#include "cleanertablemodel.h"
#include <QDateTime>
#include <QDateEdit>
#include <QSpinBox>

/* Encapsulate ws_info */
class RegInfoModel : public CleanerTableModel {
	Q_OBJECT
public:
	static RegInfoModel *instance();

	enum Column {
		DESCRIPTION,
		SERVICE_INTERVAL_TIME,
		SERVICE_INTERVAL_DIVES,
		LAST_SERVICE,
		NEXT_SERVICE
	};
	RegInfoModel();

	/*reimp*/ QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex &parent = QModelIndex()) const;
	/*reimp*/ bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
	/*reimp*/ bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	const QString &biggerString() const;
	void clear();
	void update();
	void updateInfo();

private:
	int rows;
	QString biggerEntry;
};

#endif // VEINFOMODEL_H
