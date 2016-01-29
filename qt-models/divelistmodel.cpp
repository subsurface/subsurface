#include "divelistmodel.h"
#include "helpers.h"
#include <QDateTime>

DiveListSortModel::DiveListSortModel(QObject *parent) : QSortFilterProxyModel(parent)
{

}

int DiveListSortModel::getDiveId(int idx)
{
	DiveListModel *mySourceModel = qobject_cast<DiveListModel *>(sourceModel());
	return mySourceModel->getDiveId(mapToSource(index(idx,0)).row());
}

int DiveListSortModel::getIdxForId(int id)
{
	for (int i = 0; i < rowCount(); i++) {
		QVariant v = data(index(i, 0), DiveListModel::DiveRole);
		DiveObjectHelper *d = v.value<DiveObjectHelper *>();
		if (d->id() == id)
			return i;
	}
	return -1;
}

DiveListModel *DiveListModel::m_instance = NULL;

DiveListModel::DiveListModel(QObject *parent) : QAbstractListModel(parent)
{
	m_instance = this;
}

void DiveListModel::addDive(dive *d)
{
	beginInsertRows(QModelIndex(), rowCount(), rowCount());
	m_dives.append(new DiveObjectHelper(d));
	endInsertRows();
}

void DiveListModel::insertDive(int i, DiveObjectHelper *newDive)
{
	beginInsertRows(QModelIndex(), i, i);
	m_dives.insert(i, newDive);
	endInsertRows();
}

void DiveListModel::removeDive(int i)
{
	beginRemoveRows(QModelIndex(), i, i);
	m_dives.removeAt(i);
	endRemoveRows();
}

void DiveListModel::removeDiveById(int id)
{
	for (int i = 0; i < rowCount(); i++) {
		if (m_dives.at(i)->id() == id) {
			removeDive(i);
			return;
		}
	}
}

void DiveListModel::updateDive(int i, dive *d)
{
	DiveObjectHelper *newDive = new DiveObjectHelper(d);
	removeDive(i);
	insertDive(i, newDive);
}

void DiveListModel::clear()
{
	if (m_dives.count()) {
		beginRemoveRows(QModelIndex(), 0, m_dives.count() - 1);
		qDeleteAll(m_dives);
		m_dives.clear();
		endRemoveRows();
	}
}

int DiveListModel::rowCount(const QModelIndex &) const
{
	return m_dives.count();
}

int DiveListModel::getDiveId(int idx) const
{
	if (idx < 0 || idx >= m_dives.count())
		return -1;
	return m_dives[idx]->id();
}

QVariant DiveListModel::data(const QModelIndex &index, int role) const
{
	if(index.row() < 0 || index.row() > m_dives.count())
		return QVariant();

	DiveObjectHelper *curr_dive = m_dives[index.row()];
	switch(role) {
	case DiveRole: return QVariant::fromValue<QObject*>(curr_dive);
	case DiveDateRole: return (qlonglong)curr_dive->timestamp();
	}
	return QVariant();

}

QHash<int, QByteArray> DiveListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[DiveRole] = "dive";
	roles[DiveDateRole] = "date";
	return roles;
}

// create a new dive. set the current time and add it to the end of the dive list
QString DiveListModel::startAddDive()
{
	struct dive *d;
	d = alloc_dive();
	d->when = QDateTime::currentMSecsSinceEpoch() / 1000L + gettimezoneoffset();

	// find the highest dive nr we have and pick the next one
	struct dive *pd;
	int i, nr = 0;
	for_each_dive(i, pd) {
		if (pd->number > nr)
			nr = pd->number;
	}
	nr++;
	d->number = nr;
	d->dc.model = strdup("manually added dive");
	add_single_dive(-1, d);
	addDive(d);
	return QString::number(d->id);
}

DiveListModel *DiveListModel::instance()
{
	return m_instance;
}

DiveObjectHelper* DiveListModel::at(int i)
{
	return m_dives.at(i);
}
