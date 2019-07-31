#include "datestatsmodel.h"
#include "core/dive.h"
#include "core/divelist.h"
#include "core/time.c"
#include <QtCore/QVector>

DateStatsTableModel::DateStatsTableModel(QObject *parent) :
	QAbstractTableModel(parent)
{
	int idx;
	struct dive *dp;
	struct tm tm;
	int current_year = -1;
	QVector<int> yearVec(YEAR + 1, 0);

	// Iterate through dives and count for each month and year
	for_each_dive(idx, dp) {
		utc_mkdate(dp->when, &tm);
		// Advance current_year to the current dive, adding an empty
		// QVector for each year
		while (tm.tm_year != current_year) {
			// Check if this is the first dive
			if (current_year == -1) {
				yearVec[YEAR] = tm.tm_year;
				current_year = tm.tm_year;
			} else {
				m_data.append(yearVec);
				current_year++;
				yearVec.fill(0);
				yearVec[YEAR] = current_year;
			}
		}
		// Increment current year and month and year total
		yearVec[tm.tm_mon]++;
		yearVec[TOTAL]++;
	}

	// Append the last QVector if there were any dives
	if (current_year != -1)
		m_data.append(yearVec);
}

int DateStatsTableModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return YEAR;
}

int DateStatsTableModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return m_data.count();
}

int DateStatsTableModel::axisMax(Qt::Orientation orientation) const
{
	int idx, month, dive_count, max_dives = 0;

	if (orientation == Qt::Horizontal) {
		// Find max number of year dives and round up to next multiple of 10
		for (idx = 0; idx < columnCount(); idx++)
			if (m_data[idx].at(TOTAL) > max_dives)
				max_dives = m_data[idx].at(TOTAL);
		return (((max_dives / 10) + 1) * 10);
	} else {
		// Find max number of dives for a given month and round to multiple of 5
		for (month = JAN; month <= DEC; month++) {
			dive_count = 0;
			for (idx =0; idx < columnCount(); idx++)
				dive_count += m_data[idx].at(month);
			if (dive_count > max_dives)
				max_dives = dive_count;
		}
		return (((max_dives / 5) + 1) * 5);
	}
}

QVariant DateStatsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal) {
		// Return QString with Year lable in it.
		return QString("%1").arg(m_data[section].at(YEAR));
	} else {
		// Return labels for months and total.
		switch (section) {
		case TOTAL:
			return tr("Total");
		case JAN:
			return tr("January");
		case FEB:
			return tr("February");
		case MAR:
			return tr("March");
		case APR:
			return tr("April");
		case MAY:
			return tr("May");
		case JUN:
			return tr("June");
		case JUL:
			return tr("July");
		case AUG:
			return tr("August");
		case SEP:
			return tr("September");
		case OCT:
			return tr("October");
		case NOV:
			return tr("November");
		case DEC:
			return tr("December");
		}
	}
	return QVariant();
}

QVariant DateStatsTableModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::DisplayRole)
		return m_data[index.column()].at(index.row());
	return QVariant();
}
