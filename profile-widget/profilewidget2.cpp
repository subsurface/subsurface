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
		report_info("Profile calculation for dive %d took %lld ms -- calculated ceiling preference is %d", d->number, elapsedTime, prefs.calcceiling);
	if (elapsedTime > 1000 && prefs.calcndltts) {
		qPrefTechnicalDetails::set_calcndltts(false);
		report_error("%s", qPrintable(tr("Show NDL / TTS was disabled because of excessive processing time")));
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
static bool isDiveTextItem(const QGraphicsItem *item, const DiveTextItem *textItem)
{
	while (item) {
		if (item == textItem)
			return true;
		item = item->parentItem();
	}
	return false;
}

void ProfileWidget2::addGasChangeMenu(QMenu &m, QString menuTitle, const struct dive &d, int dcNr, int changeTime)
{
	QMenu *gasChange = m.addMenu(menuTitle);
	std::vector<std::pair<int, QString>> gases = get_dive_gas_list(&d, dcNr, true);
	for (unsigned i = 0; i < gases.size(); i++) {
		int cylinderIndex = gases[i].first;
		gasChange->addAction(gases[i].second, [this, cylinderIndex, changeTime] { addGasSwitch(cylinderIndex, changeTime); });
	}
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
	if (d && item && item->ev.is_gaschange()) {
		addGasChangeMenu(m, tr("Edit gas change"), *d, dc, item->ev.time.seconds);
	} else if (d && d->cylinders.size() > 1) {
		// if we have more than one gas, offer to switch to another one
		const struct divecomputer *currentdc = d->get_dc(dc);
		if (seconds == 0 || (!currentdc->samples.empty() && seconds <= currentdc->samples[0].time.seconds))
			addGasChangeMenu(m, tr("Set initial gas"), *d, dc, 0);
		else
			addGasChangeMenu(m, tr("Add gas change"), *d, dc, seconds);
	}
	m.addAction(tr("Add setpoint change"), [this, seconds]() { ProfileWidget2::addSetpointChange(seconds); });
	m.addAction(tr("Add bookmark"), [this, seconds]() { addBookmark(seconds); });
	m.addAction(tr("Split dive into two"), [this, seconds]() { splitDive(seconds); });

	divemode_loop loop(*d->get_dc(dc));
	divemode_t divemode = loop.at(seconds);
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
		m.addAction(tr("Remove event"), [this,item] { removeEvent(item); });
		m.addAction(tr("Hide event"), [this, item] { hideEvent(item); });
		m.addAction(tr("Hide events of type '%1'").arg(event_type_name(item->ev)),
			    [this, item] { hideEventType(item); });
		if (item->ev.type == SAMPLE_EVENT_BOOKMARK)
			m.addAction(tr("Edit name"), [this, item] { editName(item); });
#if 0 // TODO::: FINISH OR DISABLE
		QPointF scenePos = mapToScene(event->pos());
		int idx = getEntryFromPos(scenePos);
		// this shows how to figure out if we should ask the user if they want adjust interpolated pressures
		// at either side of a gas change
		if (item->ev->type == SAMPLE_EVENT_GASCHANGE || item->ev->type == SAMPLE_EVENT_GASCHANGE2) {
			int gasChangeIdx = idx;
			while (gasChangeIdx > 0) {
				--gasChangeIdx;
				if (plotInfo.entry[gasChangeIdx].sec <= item->ev->time.seconds)
					break;
			}
			const struct plot_data &gasChangeEntry = plotInfo.entry[newGasIdx];
			// now gasChangeEntry points at the gas change, that entry has the final pressure of
			// the old tank, the next entry has the starting pressure of the next tank
			if (gasChangeIdx < plotInfo.nr - 1) {
				int newGasIdx = gasChangeIdx + 1;
				const struct plot_data &newGasEntry = plotInfo.entry[newGasIdx];
				if (get_plot_sensor_pressure(&plotInfo, gasChangeIdx) == 0 || d->get_cylinder(gasChangeEntry->sensor[0])->sample_start.mbar == 0) {
					// if we have no sensorpressure or if we have no pressure from samples we can assume that
					// we only have interpolated pressure (the pressure in the entry may be stored in the sensor
					// pressure field if this is the first or last entry for this tank... see details in gaspressures.c
					pressure_t pressure;
					pressure.mbar = get_plot_interpolated_pressure(&plotInfo, gasChangeIdx) ? : get_plot_sensor_pressure(&plotInfo, gasChangeIdx);
					QAction *adjustOldPressure = m.addAction(tr("Adjust pressure of cyl. %1 (currently interpolated as %2)")
										 .arg(gasChangeEntry->sensor[0] + 1).arg(get_pressure_string(pressure)));
				}
				if (get_plot_sensor_pressure(&plotInfo, newGasIdx) == 0 || d->get_cylinder(newGasEntry->sensor[0])->sample_start.mbar == 0) {
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
	const struct divecomputer *currentdc = d->get_dc(dc);
	if (currentdc && std::any_of(currentdc->events.begin(), currentdc->events.end(),
	    [] (auto &ev) { return ev.hidden; }))
		m.addAction(tr("Unhide individually hidden events of this dive"), this, &ProfileWidget2::unhideEvents);
	m.exec(event->globalPos());
}

void ProfileWidget2::hideEvent(DiveEventItem *item)
{
	if (!d)
		return;
	struct divecomputer *currentdc = mutable_dive()->get_dc(dc);
	int idx = item->idx;
	if (!currentdc || idx < 0 || static_cast<size_t>(idx) >= currentdc->events.size())
		return;
	currentdc->events[idx].hidden = true;
	item->hide();
}

void ProfileWidget2::hideEventType(DiveEventItem *item)
{
	if (!item->ev.name.empty()) {
		hide_event_type(&item->ev);

		replot();
	}
}

void ProfileWidget2::unhideEvents()
{
	if (!d)
		return;
	struct divecomputer *currentdc = mutable_dive()->get_dc(dc);
	if (!currentdc)
		return;
	for (auto &ev: currentdc->events)
		ev.hidden = false;
	for (DiveEventItem *item: profileScene->eventItems)
		item->show();
}

void ProfileWidget2::unhideEventTypes()
{
	show_all_event_types();

	replot();
}

void ProfileWidget2::removeEvent(DiveEventItem *item)
{
	const struct event &ev = item->ev;
	if (QMessageBox::question(this, TITLE_OR_TEXT(
					  tr("Remove the selected event?"),
					  tr("%1 @ %2:%3").arg(QString::fromStdString(ev.name)).arg(ev.time.seconds / 60).arg(ev.time.seconds % 60, 2, 10, QChar('0'))),
				  QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok)
		Command::removeEvent(mutable_dive(), dc, item->idx);
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
	Command::splitDives(mutable_dive(), duration_t{ .seconds = seconds });
}

void ProfileWidget2::addGasSwitch(int tank, int seconds)
{
	if (!d || tank < 0 || static_cast<size_t>(tank) >= d->cylinders.size())
		return;

	Command::addGasSwitch(mutable_dive(), dc, seconds, tank);
}

void ProfileWidget2::changeGas(int index, int newCylinderId)
{
	if ((currentState == PLAN || currentState == EDIT) && plannerModel) {
		QModelIndex modelIndex = plannerModel->index(index, DivePlannerPointsModel::GAS);
		plannerModel->gasChange(modelIndex.sibling(modelIndex.row() + 1, modelIndex.column()), newCylinderId);

		if (currentState == EDIT)
			emit stopEdited();
	}
}

void ProfileWidget2::editName(DiveEventItem *item)
{
	if (!d)
		return;
	bool ok;
	QString newName = QInputDialog::getText(this, tr("Edit name of bookmark"),
						tr("Custom name:"), QLineEdit::Normal,
						item->ev.name.c_str(), &ok);
	if (ok && !newName.isEmpty()) {
		if (newName.length() > 22) { //longer names will display as garbage.
			QMessageBox lengthWarning;
			lengthWarning.setText(tr("Name is too long!"));
			lengthWarning.exec();
			return;
		}
		Command::renameEvent(mutable_dive(), dc, item->idx, newName.toStdString());
	}
}

void ProfileWidget2::connectPlannerModel()
{
	connect(plannerModel, &DivePlannerPointsModel::dataChanged, this, &ProfileWidget2::replot);
	connect(plannerModel, &DivePlannerPointsModel::cylinderModelEdited, this, &ProfileWidget2::replot);
	connect(plannerModel, &DivePlannerPointsModel::modelReset, this, &ProfileWidget2::pointsReset);
	connect(plannerModel, &DivePlannerPointsModel::rowsInserted, this, &ProfileWidget2::pointInserted);
	connect(plannerModel, &DivePlannerPointsModel::rowsRemoved, this, &ProfileWidget2::pointsRemoved);
	connect(plannerModel, &DivePlannerPointsModel::rowsMoved, this, &ProfileWidget2::pointsMoved);
}
#endif

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
