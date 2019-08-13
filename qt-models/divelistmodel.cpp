// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divelistmodel.h"
#include "core/qthelper.h"
#include "core/trip.h"
#include "core/settings/qPrefGeneral.h"
#include <QDateTime>

DiveListSortModel::DiveListSortModel(QObject *parent) : QSortFilterProxyModel(parent)
{
	updateFilterState();
}

void DiveListSortModel::updateFilterState()
{
	if (filterString.isEmpty()) {
		resetFilter();
		return;
	}
	// store this in local variables to avoid having to call these methods over and over
	bool includeNotes = qPrefGeneral::filterFullTextNotes();
	Qt::CaseSensitivity cs = qPrefGeneral::filterCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive;

	int i;
	struct dive *d;
	for_each_dive(i, d)
		d->hidden_by_filter = !DiveObjectHelper::containsText(d, filterString, cs, includeNotes);
}

void DiveListSortModel::setSourceModel(QAbstractItemModel *sourceModel)
{
	QSortFilterProxyModel::setSourceModel(sourceModel);
}

void DiveListSortModel::setFilter(QString f)
{
	filterString = f;
	updateFilterState();
	invalidateFilter();
}

void DiveListSortModel::resetFilter()
{
	int i;
	struct dive *d;
	for_each_dive(i, d)
		d->hidden_by_filter = false;
	invalidateFilter();
}

// filtering is way too slow on mobile. Maybe we should roll our own?
bool DiveListSortModel::filterAcceptsRow(int source_row, const QModelIndex &) const
{
	DiveListModel *mySourceModel = qobject_cast<DiveListModel *>(sourceModel());
	DiveObjectHelper *d = mySourceModel->at(source_row);
	return d && !d->getDive()->hidden_by_filter;
}

int DiveListSortModel::shown()
{
	return rowCount();
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

void DiveListSortModel::clear()
{
	DiveListModel *mySourceModel = qobject_cast<DiveListModel *>(sourceModel());
	mySourceModel->clear();
}

void DiveListSortModel::addAllDives()
{
	DiveListModel *mySourceModel = qobject_cast<DiveListModel *>(sourceModel());
	mySourceModel->addAllDives();
	updateFilterState();
}

// In QML, section headings can only be strings. To identify dives that
// belong to the same trip, a string containing the trip-pointer in hexadecimal
// encoding is passed in. To format the trip heading, the string is then
// converted back with this function.
QVariant DiveListSortModel::tripIdToObject(const QString &s)
{
	if (s.isEmpty())
		return QVariant();
	return QVariant::fromValue((dive_trip *)s.toULongLong(nullptr, 16));
}

// the trip title is designed to be location (# dives)
// or, if there is no location name date range (# dives)
// where the date range is given as "month year" or "month-month year" or "month year - month year"
QString DiveListSortModel::tripTitle(const QVariant &tripIn)
{
	dive_trip *dt = tripIn.value<dive_trip *>();
	if (!dt)
		return QString();
	QString numDives = tr("(%n dive(s))", "", dt->dives.nr);
	int shown = trip_shown_dives(dt);
	QString shownDives = shown != dt->dives.nr ? QStringLiteral(" ") + tr("(%L1 shown)").arg(shown) : QString();
	QString title(dt->location);

	if (title.isEmpty()) {
		// so use the date range
		QDateTime firstTime = QDateTime::fromMSecsSinceEpoch(1000*trip_date(dt), Qt::UTC);
		QString firstMonth = firstTime.toString("MMM");
		QString firstYear = firstTime.toString("yyyy");
		QDateTime lastTime = QDateTime::fromMSecsSinceEpoch(1000*dt->dives.dives[0]->when, Qt::UTC);
		QString lastMonth = lastTime.toString("MMM");
		QString lastYear = lastTime.toString("yyyy");
		if (lastMonth == firstMonth && lastYear == firstYear)
			title = firstMonth + " " + firstYear;
		else if (lastMonth != firstMonth && lastYear == firstYear)
			title = firstMonth + "-" + lastMonth + " " + firstYear;
		else
			title = firstMonth + " " + firstYear + " - " + lastMonth + " " + lastYear;
	}
	return QStringLiteral("%1 %2%3").arg(title, numDives, shownDives);
}

QString DiveListSortModel::tripShortDate(const QVariant &tripIn)
{
	dive_trip *dt = tripIn.value<dive_trip *>();
	if (!dt)
		return QString();
	QDateTime firstTime = QDateTime::fromMSecsSinceEpoch(1000*trip_date(dt), Qt::UTC);
	QString firstMonth = firstTime.toString("MMM");
	return QStringLiteral("%1\n'%2").arg(firstMonth,firstTime.toString("yy"));
}

DiveListModel *DiveListModel::m_instance = NULL;

DiveListModel::DiveListModel(QObject *parent) : QAbstractListModel(parent)
{
	m_instance = this;
}

void DiveListModel::addDive(const QList<dive *> &listOfDives)
{
	if (listOfDives.isEmpty())
		return;
	beginInsertRows(QModelIndex(), rowCount(), rowCount() + listOfDives.count() - 1);
	for (dive *d: listOfDives) {
		m_dives.append(new DiveObjectHelper(d));
	}
	endInsertRows();
}

void DiveListModel::addAllDives()
{
	QList<dive *>listOfDives;
	int i;
	struct dive *d;
	for_each_dive (i, d)
		listOfDives.append(d);
	addDive(listOfDives);

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
	delete m_dives.at(i);
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
	// we need to make sure that QML knows that this dive has changed -
	// the only reliable way I've found is to remove and re-insert it
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

void DiveListModel::resetInternalData()
{
	// this is a hack. There is a long standing issue, that seems related to a
	// sync problem between QML engine and underlying model data. It causes delete
	// from divelist (on mobile) to crash. But not always. This function is part of
	// an attempt to fix this. See commit.
	beginResetModel();
	endResetModel();
}

int DiveListModel::rowCount(const QModelIndex &) const
{
	return m_dives.count();
}

int DiveListModel::getDiveIdx(int id) const
{
	int i;
	for (i = 0; i < m_dives.count(); i++) {
		if (m_dives.at(i)->id() == id)
			return i;
	}
	return -1;
}

QVariant DiveListModel::data(const QModelIndex &index, int role) const
{
	if(index.row() < 0 || index.row() >= m_dives.count())
		return QVariant();

	DiveObjectHelper *curr_dive = m_dives[index.row()];
	switch(role) {
	case DiveRole: return QVariant::fromValue<QObject*>(curr_dive);
	case DiveDateRole: return (qlonglong)curr_dive->timestamp();
	case FullTextRole: return curr_dive->fullText();
	case FullTextNoNotesRole: return curr_dive->fullTextNoNotes();
	}
	return QVariant();

}

QHash<int, QByteArray> DiveListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[DiveRole] = "dive";
	roles[DiveDateRole] = "date";
	roles[FullTextRole] = "fulltext";
	roles[FullTextNoNotesRole] = "fulltextnonotes";
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
	append_dive(d);
	insertDive(get_idx_by_uniq_id(d->id), new DiveObjectHelper(d));
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
