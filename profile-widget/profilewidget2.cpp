// SPDX-License-Identifier: GPL-2.0
#include "profile-widget/profilescene.h"
#include "core/device.h"
#include "core/event.h"
#include "core/eventtype.h"
#include "core/subsurface-string.h"
#include "core/qthelper.h"
#include "core/range.h"
#include "core/settings/qPrefTechnicalDetails.h"
#include "core/settings/qPrefPartialPressureGas.h"
#include "profile-widget/diveeventitem.h"
#include "profile-widget/divetextitem.h"
#include "profile-widget/divetooltipitem.h"
#include "profile-widget/divehandler.h"
#include "core/planner.h"
#include "profile-widget/ruleritem.h"
#include "core/pref.h"
#include "qt-models/diveplannermodel.h"
#include "qt-models/models.h"
#include "core/errorhelper.h"
#ifndef SUBSURFACE_MOBILE
#include "desktop-widgets/simplewidgets.h"
#include "commands/command.h"
#include "core/gettextfromc.h"
#include "core/imagedownloader.h"
#endif

#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>
#include <QWheelEvent>
#include <QMenu>
#include <QMimeData>
#include <QElapsedTimer>

#ifndef QT_NO_DEBUG
#include <QTableView>
#endif

// Constant describing at which z-level the thumbnails are located.
// We might add more constants here for easier customability.
static const double thumbnailBaseZValue = 100.0;

static double calcZoom(int zoomLevel)
{
	// Base of exponential zoom function: one wheel-click will increase the zoom by 15%.
	constexpr double zoomFactor = 1.15;
	return zoomLevel == 0 ? 1.0 : pow(zoomFactor, zoomLevel);
}

ProfileWidget2::ProfileWidget2(DivePlannerPointsModel *plannerModelIn, double dpr, QWidget *parent) : QGraphicsView(parent),
	profileScene(new ProfileScene(dpr, false, false)),
	currentState(INIT),
	plannerModel(plannerModelIn),
	zoomLevel(0),
	zoomedPosition(0.0),
#ifndef SUBSURFACE_MOBILE
	toolTipItem(new ToolTipItem()),
#endif
	d(nullptr),
	dc(0),
	empty(true),
	panning(false),
#ifndef SUBSURFACE_MOBILE
	mouseFollowerVertical(new DiveLineItem()),
	mouseFollowerHorizontal(new DiveLineItem()),
	rulerItem(new RulerItem2()),
#endif
	shouldCalculateMax(true)
{
	setupSceneAndFlags();
	setupItemOnScene();
	addItemsToScene();
	scene()->installEventFilter(this);
#ifndef SUBSURFACE_MOBILE
	setAcceptDrops(true);

	connect(Thumbnailer::instance(), &Thumbnailer::thumbnailChanged, this, &ProfileWidget2::updateThumbnail, Qt::QueuedConnection);
	connect(&diveListNotifier, &DiveListNotifier::cylinderEdited, this, &ProfileWidget2::profileChanged);
	connect(&diveListNotifier, &DiveListNotifier::eventsChanged, this, &ProfileWidget2::profileChanged);
	connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &ProfileWidget2::divesChanged);
	connect(&diveListNotifier, &DiveListNotifier::deviceEdited, this, &ProfileWidget2::replot);
	connect(&diveListNotifier, &DiveListNotifier::diveComputerEdited, this, &ProfileWidget2::replot);
#endif // SUBSURFACE_MOBILE

#if !defined(QT_NO_DEBUG) && defined(SHOW_PLOT_INFO_TABLE)
	QTableView *diveDepthTableView = new QTableView();
	diveDepthTableView->setModel(profileScene->dataModel);
	diveDepthTableView->show();
#endif

	setProfileState();
}

ProfileWidget2::~ProfileWidget2()
{
}

#ifndef SUBSURFACE_MOBILE
void ProfileWidget2::keyPressEvent(QKeyEvent *e)
{
	switch (e->key()) {
		case Qt::Key_Delete: return keyDeleteAction();
		case Qt::Key_Up: return keyUpAction();
		case Qt::Key_Down: return keyDownAction();
		case Qt::Key_Left: return keyLeftAction();
		case Qt::Key_Right: return keyRightAction();
	}
	QGraphicsView::keyPressEvent(e);
}
#endif // SUBSURFACE_MOBILE

void ProfileWidget2::addItemsToScene()
{
#ifndef SUBSURFACE_MOBILE
	scene()->addItem(toolTipItem);
	scene()->addItem(rulerItem);
	scene()->addItem(rulerItem->sourceNode());
	scene()->addItem(rulerItem->destNode());
	scene()->addItem(mouseFollowerHorizontal);
	scene()->addItem(mouseFollowerVertical);
	QPen pen(QColor(Qt::red).lighter());
	pen.setWidth(0);
	mouseFollowerHorizontal->setPen(pen);
	mouseFollowerVertical->setPen(pen);
#endif
}

void ProfileWidget2::setupItemOnScene()
{
#ifndef SUBSURFACE_MOBILE
	toolTipItem->setZValue(9998);
	toolTipItem->setTimeAxis(profileScene->timeAxis);
	rulerItem->setZValue(9997);
	rulerItem->setAxis(profileScene->timeAxis, profileScene->profileYAxis);
	mouseFollowerHorizontal->setZValue(9996);
	mouseFollowerVertical->setZValue(9995);
#endif
}

void ProfileWidget2::replot()
{
	plotDive(d, dc);
}

void ProfileWidget2::setupSceneAndFlags()
{
	setScene(profileScene.get());
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setOptimizationFlags(QGraphicsView::DontSavePainterState);
	setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
	setMouseTracking(true);
}

// Currently just one dive, but the plan is to enable All of the selected dives.
void ProfileWidget2::plotDive(const struct dive *dIn, int dcIn, int flags)
{
	d = dIn;
	dc = dcIn;
	if (!d) {
		clear();
		return;
	}

	// If there was no previously displayed dive, turn off animations
	if (empty)
		flags |= RenderFlags::Instant;
	empty = false;

	QElapsedTimer measureDuration; // let's measure how long this takes us (maybe we'll turn of TTL calculation later
	measureDuration.start();

	DivePlannerPointsModel *model = currentState == EDIT || currentState == PLAN ? plannerModel : nullptr;
	bool inPlanner = currentState == PLAN;

	double zoom = calcZoom(zoomLevel);
	profileScene->plotDive(d, dc, model, inPlanner, flags & RenderFlags::Instant,
			       flags & RenderFlags::DontRecalculatePlotInfo,
			       shouldCalculateMax, zoom, zoomedPosition);

#ifndef SUBSURFACE_MOBILE
	toolTipItem->setVisible(prefs.infobox);
	toolTipItem->setPlotInfo(profileScene->plotInfo);
	rulerItem->setVisible(prefs.rulergraph && currentState != PLAN && currentState != EDIT);
	rulerItem->setPlotInfo(d, profileScene->plotInfo);

	if ((currentState == EDIT || currentState == PLAN) && plannerModel) {
		repositionDiveHandlers();
		plannerModel->deleteTemporaryPlan();
	}

	// On zoom / pan don't recreate the picture thumbnails, only change their position.
	if (flags & RenderFlags::DontRecalculatePlotInfo)
		updateThumbnails();
	else
		plotPicturesInternal(d, flags & RenderFlags::Instant);

	toolTipItem->refresh(d, mapToScene(mapFromGlobal(QCursor::pos())), currentState == PLAN);
#endif

	// OK, how long did this take us? Anything above the second is way too long,
	// so if we are calculation TTS / NDL then let's force that off.
	qint64 elapsedTime = measureDuration.elapsed();
	if (verbose)
		qDebug() << "Profile calculation for dive " << d->number << "took" << elapsedTime << "ms" << " -- calculated ceiling preference is" << prefs.calcceiling;
	if (elapsedTime > 1000 && prefs.calcndltts) {
		qPrefTechnicalDetails::set_calcndltts(false);
		report_error(qPrintable(tr("Show NDL / TTS was disabled because of excessive processing time")));
	}
}

void ProfileWidget2::divesChanged(const QVector<dive *> &dives, DiveField field)
{
	// If the mode of the currently displayed dive changed, replot
	if (field.mode &&
	    std::find(dives.begin(), dives.end(), d) != dives.end())
		replot();
}

void ProfileWidget2::actionRequestedReplot(bool)
{
	settingsChanged();
}

void ProfileWidget2::settingsChanged()
{
	replot();
}

void ProfileWidget2::resizeEvent(QResizeEvent *event)
{
	QGraphicsView::resizeEvent(event);
	profileScene->resize(viewport()->size());
	plotDive(d, dc, RenderFlags::Instant | RenderFlags::DontRecalculatePlotInfo); // disable animation on resize events
}

#ifndef SUBSURFACE_MOBILE
bool ProfileWidget2::eventFilter(QObject *object, QEvent *event)
{
	QGraphicsScene *s = qobject_cast<QGraphicsScene *>(object);
	if (s && event->type() == QEvent::GraphicsSceneHelp) {
		event->ignore();
		return true;
	}
	return QGraphicsView::eventFilter(object, event);
}
#endif

template <typename T>
static void hideAll(const T &container)
{
	for (auto &item: container)
		item->setVisible(false);
}

void ProfileWidget2::setProfileState(const dive *dIn, int dcIn)
{
	d = dIn;
	dc = dcIn;

	setProfileState();
}

void ProfileWidget2::setProfileState()
{
	if (currentState == PROFILE)
		return;

	currentState = PROFILE;
	setBackgroundBrush(getColor(::BACKGROUND, profileScene->isGrayscale));

#ifndef SUBSURFACE_MOBILE
	toolTipItem->readPos();
	toolTipItem->setVisible(prefs.infobox);
	rulerItem->setVisible(prefs.rulergraph);
	mouseFollowerHorizontal->setVisible(false);
	mouseFollowerVertical->setVisible(false);
#endif

	handles.clear();
	gases.clear();
}

#ifndef SUBSURFACE_MOBILE

void ProfileWidget2::setEditState(const dive *d, int dc)
{
	if (currentState == EDIT)
		return;

	setProfileState(d, dc);
	mouseFollowerHorizontal->setVisible(true);
	mouseFollowerVertical->setVisible(true);

	currentState = EDIT;

	repositionDiveHandlers();
}

void ProfileWidget2::setPlanState(const dive *d, int dc)
{
	if (currentState == PLAN)
		return;

	setProfileState(d, dc);
	mouseFollowerHorizontal->setVisible(true);
	mouseFollowerVertical->setVisible(true);

	currentState = PLAN;
	setBackgroundBrush(QColor("#D7E3EF"));

	repositionDiveHandlers();
}
#endif

bool ProfileWidget2::isPlanner() const
{
	return currentState == PLAN;
}

#if 0 // TODO::: FINISH OR DISABLE
struct int ProfileWidget2::getEntryFromPos(QPointF pos)
{
	// find the time stamp corresponding to the mouse position
	int seconds = lrint(timeAxis->valueAt(pos));

	for (int i = 0; i < plotInfo.nr; i++) {
		if (plotInfo.entry[i].sec >= seconds)
			return i;
	}
	return plotInfo.nr - 1;
}
#endif

#ifndef SUBSURFACE_MOBILE
/// Prints cylinder information for display.
/// eg : "Cyl 1 (AL80 EAN32)"
static QString printCylinderDescription(int i, const cylinder_t *cylinder)
{
	QString label = gettextFromC::tr("Cyl") + QString(" %1").arg(i+1);
	if( cylinder != NULL ) {
		QString mix = get_gas_string(cylinder->gasmix);
		label += QString(" (%2 %3)").arg(cylinder->type.description).arg(mix);
	}
	return label;
}

static bool isDiveTextItem(const QGraphicsItem *item, const DiveTextItem *textItem)
{
	while (item) {
		if (item == textItem)
			return true;
		item = item->parentItem();
	}
	return false;
}

void ProfileWidget2::contextMenuEvent(QContextMenuEvent *event)
{
	if (currentState == EDIT || currentState == PLAN) {
		QGraphicsView::contextMenuEvent(event);
		return;
	}
	QMenu m;
	if (!d)
		return;

	// create the profile context menu
	QPointF scenePos = mapToScene(mapFromGlobal(event->globalPos()));
	qreal sec_val = profileScene->timeAxis->valueAt(scenePos);
	int seconds = (sec_val < 0.0) ? 0 : (int)sec_val;
	DiveEventItem *item = dynamic_cast<DiveEventItem *>(sceneItem);

	// Add or edit Gas Change
	if (d && item && event_is_gaschange(item->getEvent())) {
		int eventTime = item->getEvent()->time.seconds;
		QMenu *gasChange = m.addMenu(tr("Edit Gas Change"));
		for (int i = 0; i < d->cylinders.nr; i++) {
			const cylinder_t *cylinder = get_cylinder(d, i);
			QString label = printCylinderDescription(i, cylinder);
			gasChange->addAction(label, [this, i, eventTime] { changeGas(i, eventTime); });
		}
	} else if (d && d->cylinders.nr > 1) {
		// if we have more than one gas, offer to switch to another one
		QMenu *gasChange = m.addMenu(tr("Add gas change"));
		for (int i = 0; i < d->cylinders.nr; i++) {
			const cylinder_t *cylinder = get_cylinder(d, i);
			QString label = printCylinderDescription(i, cylinder);
			gasChange->addAction(label, [this, i, seconds] { changeGas(i, seconds); });
		}
	}
	m.addAction(tr("Add setpoint change"), [this, seconds]() { ProfileWidget2::addSetpointChange(seconds); });
	m.addAction(tr("Add bookmark"), [this, seconds]() { addBookmark(seconds); });
	m.addAction(tr("Split dive into two"), [this, seconds]() { splitDive(seconds); });
	const struct event *ev = NULL;
	enum divemode_t divemode = UNDEF_COMP_TYPE;

	get_current_divemode(get_dive_dc_const(d, dc), seconds, &ev, &divemode);
	QMenu *changeMode = m.addMenu(tr("Change divemode"));
	if (divemode != OC)
		changeMode->addAction(gettextFromC::tr(divemode_text_ui[OC]),
				      [this, seconds](){ addDivemodeSwitch(seconds, OC); });
	if (divemode != CCR)
		changeMode->addAction(gettextFromC::tr(divemode_text_ui[CCR]),
				      [this, seconds](){ addDivemodeSwitch(seconds, CCR); });
	if (divemode != PSCR)
		changeMode->addAction(gettextFromC::tr(divemode_text_ui[PSCR]),
				      [this, seconds](){ addDivemodeSwitch(seconds, PSCR); });

	if (DiveEventItem *item = dynamic_cast<DiveEventItem *>(sceneItem)) {
		const struct event *dcEvent = item->getEvent();
		m.addAction(tr("Remove event"), [this,item] { removeEvent(item); });
		m.addAction(tr("Hide event"), [this, item] { hideEvent(item); });
		m.addAction(tr("Hide events of type '%1'").arg(event_type_name(dcEvent)),
			    [this, item] { hideEventType(item); });
		if (dcEvent->type == SAMPLE_EVENT_BOOKMARK)
			m.addAction(tr("Edit name"), [this, item] { editName(item); });
#if 0 // TODO::: FINISH OR DISABLE
		QPointF scenePos = mapToScene(event->pos());
		int idx = getEntryFromPos(scenePos);
		// this shows how to figure out if we should ask the user if they want adjust interpolated pressures
		// at either side of a gas change
		if (dcEvent->type == SAMPLE_EVENT_GASCHANGE || dcEvent->type == SAMPLE_EVENT_GASCHANGE2) {
			qDebug() << "figure out if there are interpolated pressures";
			int gasChangeIdx = idx;
			while (gasChangeIdx > 0) {
				--gasChangeIdx;
				if (plotInfo.entry[gasChangeIdx].sec <= dcEvent->time.seconds)
					break;
			}
			const struct plot_data &gasChangeEntry = plotInfo.entry[newGasIdx];
			qDebug() << "at gas change at" << gasChangeEntry->sec << ": sensor pressure" << get_plot_sensor_pressure(&plotInfo, newGasIdx)
				 << "interpolated" << ;get_plot_sensor_pressure(&plotInfo, newGasIdx);
			// now gasChangeEntry points at the gas change, that entry has the final pressure of
			// the old tank, the next entry has the starting pressure of the next tank
			if (gasChangeIdx < plotInfo.nr - 1) {
				int newGasIdx = gasChangeIdx + 1;
				const struct plot_data &newGasEntry = plotInfo.entry[newGasIdx];
				qDebug() << "after gas change at " << newGasEntry->sec << ": sensor pressure" << newGasEntry->pressure[0] << "interpolated" << newGasEntry->pressure[1];
				if (get_plot_sensor_pressure(&plotInfo, gasChangeIdx) == 0 || get_cylinder(d, gasChangeEntry->sensor[0])->sample_start.mbar == 0) {
					// if we have no sensorpressure or if we have no pressure from samples we can assume that
					// we only have interpolated pressure (the pressure in the entry may be stored in the sensor
					// pressure field if this is the first or last entry for this tank... see details in gaspressures.c
					pressure_t pressure;
					pressure.mbar = get_plot_interpolated_pressure(&plotInfo, gasChangeIdx) ? : get_plot_sensor_pressure(&plotInfo, gasChangeIdx);
					QAction *adjustOldPressure = m.addAction(tr("Adjust pressure of cyl. %1 (currently interpolated as %2)")
										 .arg(gasChangeEntry->sensor[0] + 1).arg(get_pressure_string(pressure)));
				}
				if (get_plot_sensor_pressure(&plotInfo, newGasIdx) == 0 || get_cylinder(d, newGasEntry->sensor[0])->sample_start.mbar == 0) {
					// we only have interpolated press -- see commend above
					pressure_t pressure;
					pressure.mbar = get_plot_interpolated_pressure(&plotInfo, newGasIdx) ? : get_plot_sensor_pressure(&plotInfo, newGasIdx);
					QAction *adjustOldPressure = m.addAction(tr("Adjust pressure of cyl. %1 (currently interpolated as %2)")
										 .arg(newGasEntry->sensor[0] + 1).arg(get_pressure_string(pressure)));
				}
			}
		}
#endif
	}
	if (any_event_types_hidden()) {
		QMenu *m2 = m.addMenu(tr("Unhide event type"));
		for (int i: hidden_event_types()) {
			m2->addAction(event_type_name(i), [this, i]() {
				show_event_type(i);
				replot();
			});
		}
		m2->addAction(tr("All event types"), this, &ProfileWidget2::unhideEventTypes);
	}
	if (std::any_of(profileScene->eventItems.begin(), profileScene->eventItems.end(),
	    [] (const DiveEventItem *item) { return item->getEvent()->hidden; }))
		m.addAction(tr("Unhide individually hidden events of this dive"), this, &ProfileWidget2::unhideEvents);
	m.exec(event->globalPos());
}

void ProfileWidget2::hideEvent(DiveEventItem *item)
{
	item->getEventMutable()->hidden = true;
	item->hide();
}

void ProfileWidget2::hideEventType(DiveEventItem *item)
{
	const struct event *event = item->getEvent();

	if (!empty_string(event->name)) {
		hide_event_type(event);

		replot();
	}
}

void ProfileWidget2::unhideEvents()
{
	for (DiveEventItem *item: profileScene->eventItems) {
		item->getEventMutable()->hidden = false;
		item->show();
	}
}

void ProfileWidget2::unhideEventTypes()
{
	show_all_event_types();

	replot();
}

void ProfileWidget2::removeEvent(DiveEventItem *item)
{
	struct event *event = item->getEventMutable();
	if (!event || !d)
		return;

	if (QMessageBox::question(this, TITLE_OR_TEXT(
					  tr("Remove the selected event?"),
					  tr("%1 @ %2:%3").arg(event->name).arg(event->time.seconds / 60).arg(event->time.seconds % 60, 2, 10, QChar('0'))),
				  QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok)
		Command::removeEvent(mutable_dive(), dc, event);
}

void ProfileWidget2::addBookmark(int seconds)
{
	if (d)
		Command::addEventBookmark(mutable_dive(), dc, seconds);
}

void ProfileWidget2::addDivemodeSwitch(int seconds, int divemode)
{
	if (d)
		Command::addEventDivemodeSwitch(mutable_dive(), dc, seconds, divemode);
}

void ProfileWidget2::addSetpointChange(int seconds)
{
	if (!d)
		return;
	SetpointDialog dialog(mutable_dive(), dc, seconds);
	dialog.exec();
}

void ProfileWidget2::splitDive(int seconds)
{
	if (!d)
		return;
	Command::splitDives(mutable_dive(), duration_t{ seconds });
}

void ProfileWidget2::changeGas(int tank, int seconds)
{
	if (!d || tank < 0 || tank >= d->cylinders.nr)
		return;

	Command::addGasSwitch(mutable_dive(), dc, seconds, tank);
}
#endif

#ifndef SUBSURFACE_MOBILE
void ProfileWidget2::editName(DiveEventItem *item)
{
	struct event *event = item->getEventMutable();
	if (!event || !d)
		return;
	bool ok;
	QString newName = QInputDialog::getText(this, tr("Edit name of bookmark"),
						tr("Custom name:"), QLineEdit::Normal,
						event->name, &ok);
	if (ok && !newName.isEmpty()) {
		if (newName.length() > 22) { //longer names will display as garbage.
			QMessageBox lengthWarning;
			lengthWarning.setText(tr("Name is too long!"));
			lengthWarning.exec();
			return;
		}
		Command::renameEvent(mutable_dive(), dc, event, qPrintable(newName));
	}
}
#endif

#ifndef SUBSURFACE_MOBILE

void ProfileWidget2::profileChanged(dive *dive)
{
	if (dive != d)
		return; // Cylinders of a differnt dive than the shown one changed.
	replot();
}

#endif

struct dive *ProfileWidget2::mutable_dive() const
{
	return const_cast<dive *>(d);
}
