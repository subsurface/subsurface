// SPDX-License-Identifier: GPL-2.0
#ifndef DIVESUMMARYMODEL_H
#define DIVESUMMARYMODEL_H

#include <QAbstractTableModel>
#include <vector>

class DiveSummaryModel : public QAbstractTableModel {
	Q_OBJECT
public:
	enum Row {
		DIVES,
		DIVES_EAN,
		DIVES_DEEP,
		PLANS,
		TIME,
		TIME_MAX,
		TIME_AVG,
		DEPTH_MAX,
		DEPTH_AVG,
		SAC_MIN,
		SAC_MAX,
		SAC_AVG,
		NUM_ROW
	};

	// Roles for QML. Amazingly it appears that QML before Qt 5.12 *cannot*
	// display run-of-the-mill tabular data.
	// Therefore we transform a fixed number of columns, including the header
	// into roles that can be displayed by QML. The mind boggles.
	enum QMLRoles {
		HEADER_ROLE = Qt::UserRole + 1,
		COLUMN0_ROLE,
		COLUMN1_ROLE,
		SECTION_ROLE,
	};

	struct Result {
		QString dives, divesEAN, divesDeep, plans;
		QString time, time_max, time_avg;
		QString depth_max, depth_avg;
		QString sac_min, sac_max, sac_avg;
	};

	Q_INVOKABLE void setNumData(int num);
	Q_INVOKABLE void calc(int column, int period);
private:
	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	QHash<int, QByteArray> roleNames() const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	QVariant dataDisplay(int row, int col) const;

	std::vector<Result> results;
};

#endif
