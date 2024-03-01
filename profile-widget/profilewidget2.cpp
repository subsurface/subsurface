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
	QPen pen(QColor(Qt::red).lighter());
	pen.setWidth(0);
#endif
}

void ProfileWidget2::setupItemOnScene()
{
#ifndef SUBSURFACE_MOBILE
	toolTipItem->setZValue(9998);
	toolTipItem->setTimeAxis(profileScene->timeAxis);
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

	currentState = EDIT;

	repositionDiveHandlers();
}

void ProfileWidget2::setPlanState(const dive *d, int dc)
{
	if (currentState == PLAN)
		return;

	setProfileState(d, dc);

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

	m.exec(event->globalPos());
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
