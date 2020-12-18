// SPDX-License-Identifier: GPL-2.0
#include <QQmlContext>
#include <QDebug>
#include <QQuickItem>
#include <QModelIndex>

#include "mapwidget.h"
#include "core/divesite.h"
#include "core/selection.h"
#include "map-widget/qmlmapwidgethelper.h"
#include "qt-models/maplocationmodel.h"
#include "qt-models/divelocationmodel.h"
#include "commands/command.h"

static const QUrl urlMapWidget = QUrl(QStringLiteral("qrc:/qml/MapWidget.qml"));
static const QUrl urlMapWidgetError = QUrl(QStringLiteral("qrc:/qml/MapWidgetError.qml"));
static bool isReady = false;

#define CHECK_IS_READY_RETURN_VOID() \
	if (!isReady) return

MapWidget *MapWidget::m_instance = NULL;

MapWidget::MapWidget(QWidget *parent) : QQuickWidget(parent)
{
	m_rootItem = nullptr;
	m_mapHelper = nullptr;
	setResizeMode(QQuickWidget::SizeRootObjectToView);
	connect(this, &QQuickWidget::statusChanged, this, &MapWidget::doneLoading);
	connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &MapWidget::divesChanged);
	connect(&diveListNotifier, &DiveListNotifier::dataReset, this, &MapWidget::reload);
	setSource(urlMapWidget);
}

void MapWidget::doneLoading(QQuickWidget::Status status)
{
	// the default map widget QML failed; load the error QML.
	if (source() == urlMapWidget && status != QQuickWidget::Ready) {
		qDebug() << urlMapWidget << "failed to load with status:" << status;
		setSource(urlMapWidgetError);
		return;
	} else if (source() == urlMapWidgetError) { // the error QML finished loading.
		return;
	}

	isReady = true;
	m_rootItem = qobject_cast<QQuickItem *>(rootObject());
	m_mapHelper = rootObject()->findChild<MapWidgetHelper *>();
	connect(m_mapHelper, &MapWidgetHelper::selectedDivesChanged, this, &MapWidget::selectedDivesChanged);
	connect(m_mapHelper, &MapWidgetHelper::coordinatesChanged, this, &MapWidget::coordinatesChanged);
}

void MapWidget::centerOnDiveSite(struct dive_site *ds)
{
	CHECK_IS_READY_RETURN_VOID();
	m_mapHelper->centerOnDiveSite(ds);
}

void MapWidget::centerOnIndex(const QModelIndex& idx)
{
	CHECK_IS_READY_RETURN_VOID();
	dive_site *ds = idx.model()->index(idx.row(), LocationInformationModel::DIVESITE).data().value<dive_site *>();
	if (!ds || ds == RECENTLY_ADDED_DIVESITE || !dive_site_has_gps_location(ds))
		m_mapHelper->centerOnSelectedDiveSite();
	else
		centerOnDiveSite(ds);
}

void MapWidget::reload()
{
	CHECK_IS_READY_RETURN_VOID();
	m_mapHelper->reloadMapLocations();
	m_mapHelper->centerOnSelectedDiveSite();
}

bool MapWidget::editMode() const
{
	return isReady && m_mapHelper->editMode();
}

void MapWidget::setSelected(const QVector<dive_site *> &divesites)
{
	CHECK_IS_READY_RETURN_VOID();
	m_mapHelper->setSelected(divesites);
}

void MapWidget::selectionChanged()
{
	CHECK_IS_READY_RETURN_VOID();
	m_mapHelper->selectionChanged();
	m_mapHelper->centerOnSelectedDiveSite();
}

void MapWidget::selectedDivesChanged(const QList<int> &list)
{
	CHECK_IS_READY_RETURN_VOID();
	// We get a list of dive indices, but the selection code wants a list of dives.
	// Therefore, transform them here.
	std::vector<dive *> selection;
	selection.reserve(list.size());
	for (int idx: list) {
		if (dive *d = get_dive(idx))
			selection.push_back(d);
	}
	setSelection(selection, current_dive);
}

void MapWidget::coordinatesChanged(struct dive_site *ds, const location_t &location)
{
	Command::editDiveSiteLocation(ds, location);
}

void MapWidget::divesChanged(const QVector<dive *> &, DiveField field)
{
	if (field.divesite)
		reload();
}

// Sadly, for reasons out of our control, we can't use a normal singleton for the
// map widget: In a standard singleton, the object is freed after main() exits.
// However, if there is an animation running (map zooming), the thread is
// terminated, when the QApplication object is destroyed, which is before main()
// exits. The thread has a QQmlAnimationTimer that is freed. However, the map widget
// then tries to free the object itself, leading to a crash. Clearly, a bug in
// the QML MapWidget / QtQuick ecosystem.
// To solve this, the mainwindow will destroy the map widget and in the destructor
// the reference is cleared. Sad.
MapWidget::~MapWidget()
{
	m_instance = NULL;
}

MapWidget *MapWidget::instance()
{
	if (m_instance == NULL)
		m_instance = new MapWidget();
	return m_instance;
}
