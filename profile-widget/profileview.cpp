// SPDX-License-Identifier: GPL-2.0
#include "profileview.h"
#include "profilescene.h"
#include "zvalues.h"
#include "core/errorhelper.h"
#include "core/pref.h"
#include "core/settings/qPrefDisplay.h"
#include "core/settings/qPrefPartialPressureGas.h"
#include "core/settings/qPrefTechnicalDetails.h"
#include "qt-quick/chartitem.h"

#include <QAbstractAnimation>
#include <QCursor>
#include <QDebug>
#include <QElapsedTimer>

// Class for animations (if any). Might want to do our own.
class ProfileAnimation : public QAbstractAnimation {
	ProfileView &view;
	// For historical reasons, speed is actually the duration
	// (i.e. the reciprocal of speed). Ouch, that hurts.
	int speed;

	int duration() const override
	{
		return speed;
	}
	void updateCurrentTime(int time) override
	{
		// Note: we explicitly pass 1.0 at the end, so that
		// the callee can do a simple float comparison for "end".
		view.anim(time == speed ? 1.0
					: static_cast<double>(time) / speed);
	}
public:
	ProfileAnimation(ProfileView &view, int animSpeed) :
		view(view),
		speed(animSpeed)
	{
		start();
	}
};

ProfileView::ProfileView(QQuickItem *parent) : ChartView(parent, ProfileZValue::Count),
	d(nullptr),
	dc(0),
	zoomLevel(1.00),
	zoomedPosition(0.0),
	panning(false),
	empty(true),
	shouldCalculateMax(true)
{
	setBackgroundColor(Qt::black);
	setFlag(ItemHasContents, true);

	setAcceptHoverEvents(true);
	setAcceptedMouseButtons(Qt::LeftButton);

	auto tec = qPrefTechnicalDetails::instance();
	connect(tec, &qPrefTechnicalDetails::calcalltissuesChanged           , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::calcceilingChanged              , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::gflowChanged                    , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::gfhighChanged                   , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::dcceilingChanged                , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::eadChanged                      , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::calcceiling3mChanged            , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::modChanged                      , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::calcndlttsChanged               , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::hrgraphChanged                  , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::rulergraphChanged               , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::show_sacChanged                 , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::zoomed_plotChanged              , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::decoinfoChanged                 , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::show_pictures_in_profileChanged , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::tankbarChanged                  , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::percentagegraphChanged          , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::infoboxChanged                  , this, &ProfileView::replot);

	auto pp_gas = qPrefPartialPressureGas::instance();
	connect(pp_gas, &qPrefPartialPressureGas::pheChanged, this, &ProfileView::replot);
	connect(pp_gas, &qPrefPartialPressureGas::pn2Changed, this, &ProfileView::replot);
	connect(pp_gas, &qPrefPartialPressureGas::po2Changed, this, &ProfileView::replot);

	setAcceptTouchEvents(true);
}

ProfileView::ProfileView() : ProfileView(nullptr)
{
}

ProfileView::~ProfileView()
{
}

void ProfileView::resetPointers()
{
	profileItem.reset();
}

void ProfileView::plotAreaChanged(const QSizeF &s)
{
	if (!empty)
		plotDive(d, dc, RenderFlags::Instant);
}

void ProfileView::replot()
{
	if (!empty)
		plotDive(d, dc, RenderFlags::None);
}

void ProfileView::clear()
{
	//clearPictures();
	//disconnectTemporaryConnections();
	if (profileScene)
		profileScene->clear();
	//handles.clear();
	//gases.clear();
	empty = true;
	d = nullptr;
	dc = 0;
}

void ProfileView::plotDive(const struct dive *dIn, int dcIn, int flags)
{
	d = dIn;
	dc = dcIn;
	if (!d) {
		clear();
		return;
	}

	// We can't create the scene in the constructor, because we can't get the DPR property there. Oh joy!
	if (!profileScene) {
		double dpr = std::clamp(property("dpr").toReal(), 0.5, 100.0);
		profileScene = std::make_unique<ProfileScene>(dpr, false, false);
	}
	// If there was no previously displayed dive, turn off animations
	if (empty)
		flags |= RenderFlags::Instant;
	empty = false;

	// If Qt decided to destroy our canvas, recreate it
	if (!profileItem)
		profileItem = createChartItem<ChartGraphicsSceneItem>(ProfileZValue::Profile);

	profileItem->setPos(QPointF(0.0, 0.0));

	QElapsedTimer measureDuration; // let's measure how long this takes us (maybe we'll turn of TTL calculation later
	measureDuration.start();

	//DivePlannerPointsModel *model = currentState == EDIT || currentState == PLAN ? plannerModel : nullptr;
	DivePlannerPointsModel *model = nullptr;
	bool inPlanner = flags & RenderFlags::PlanMode;

	int animSpeed = flags & RenderFlags::Instant ? 0 : qPrefDisplay::animation_speed();

	profileScene->resize(size());
	profileScene->plotDive(d, dc, animSpeed, model, inPlanner,
			       flags & RenderFlags::DontRecalculatePlotInfo,
			       shouldCalculateMax, zoomLevel, zoomedPosition);
	background = inPlanner ? QColor("#D7E3EF") : getColor(::BACKGROUND, false);
	profileItem->draw(size(), background, *profileScene);

	//rulerItem->setVisible(prefs.rulergraph && currentState != PLAN && currentState != EDIT);
	//toolTipItem->setPlotInfo(profileScene->plotInfo);
	//rulerItem->setPlotInfo(d, profileScene->plotInfo);

	//if ((currentState == EDIT || currentState == PLAN) && plannerModel) {
		//repositionDiveHandlers();
		//plannerModel->deleteTemporaryPlan();
	//}

	// On zoom / pan don't recreate the picture thumbnails, only change their position.
	//if (flags & RenderFlags::DontRecalculatePlotInfo)
		//updateThumbnails();
	//else
		//plotPicturesInternal(d, flags & RenderFlags::Instant);

	//toolTipItem->refresh(d, mapToScene(mapFromGlobal(QCursor::pos())), currentState == PLAN);

	update();

	// OK, how long did this take us? Anything above the second is way too long,
	// so if we are calculation TTS / NDL then let's force that off.
	qint64 elapsedTime = measureDuration.elapsed();
	if (verbose)
		qDebug() << "Profile calculation for dive " << d->number << "took" << elapsedTime << "ms" << " -- calculated ceiling preference is" << prefs.calcceiling;
	if (elapsedTime > 1000 && prefs.calcndltts) {
		qPrefTechnicalDetails::set_calcndltts(false);
		report_error(qPrintable(tr("Show NDL / TTS was disabled because of excessive processing time")));
	}

	// Reset animation.
	if (animSpeed <= 0)
		animation.reset();
	else
		animation = std::make_unique<ProfileAnimation>(*this, animSpeed);
}

void ProfileView::anim(double fraction)
{
	if (!profileScene || !profileItem)
		return;
	profileScene->anim(fraction);
	profileItem->draw(size(), background, *profileScene);
	update();
}

void ProfileView::resetZoom()
{
	zoomLevel = 1.0;
	zoomedPosition = 0.0;
}

void ProfileView::setZoom(double level)
{
	level = std::clamp(level, 1.0, 20.0);
	double old = std::exchange(zoomLevel, level);
	if (level != old)
		plotDive(d, dc, RenderFlags::DontRecalculatePlotInfo);
	emit zoomLevelChanged();
}

void ProfileView::wheelEvent(QWheelEvent *event)
{
	if (!d)
		return;
	if (panning)
		return;	// No change in zoom level while panning.
	if (event->buttons() == Qt::LeftButton)
		return;
	if (event->angleDelta().y() > 0)
		setZoom(zoomLevel * 1.15);
	else if (event->angleDelta().y() < 0)
		setZoom(zoomLevel / 1.15);
	else if (event->angleDelta().x() && zoomLevel > 0) {
		double oldPos = zoomedPosition;
		zoomedPosition = profileScene->calcZoomPosition(zoomLevel,
								oldPos,
								oldPos - event->angleDelta().x());
		if (oldPos != zoomedPosition)
			plotDive(d, dc, RenderFlags::Instant | RenderFlags::DontRecalculatePlotInfo);
	}
}

void ProfileView::mousePressEvent(QMouseEvent *event)
{
	panning = true;
	QPointF pos = mapToScene(event->pos());
	panStart(pos.x(), pos.y());
	setCursor(Qt::ClosedHandCursor);
	event->accept();
}

void ProfileView::mouseReleaseEvent(QMouseEvent *)
{
	if (panning) {
		panning = false;
		unsetCursor();
	}
	//if (currentState == PLAN || currentState == EDIT) {
	//	shouldCalculateMax = true;
	//	replot();
	//}
}

void ProfileView::mouseMoveEvent(QMouseEvent *event)
{
	QPointF pos = mapToScene(event->pos());
	if (panning)
		pan(pos.x(), pos.y());

	//toolTipItem->refresh(d, mapToScene(mapFromGlobal(QCursor::pos())), currentState == PLAN);

	//if (currentState == PLAN || currentState == EDIT) {
		//QRectF rect = profileScene->profileRegion;
		//auto [miny, maxy] = profileScene->profileYAxis->screenMinMax();
		//double x = std::clamp(pos.x(), rect.left(), rect.right());
		//double y = std::clamp(pos.y(), miny, maxy);
		//mouseFollowerHorizontal->setLine(rect.left(), y, rect.right(), y);
		//mouseFollowerVertical->setLine(x, rect.top(), x, rect.bottom());
	//}
}

int ProfileView::getDiveId() const
{
	return d ? d->id : -1;
}

void ProfileView::setDiveId(int id)
{
	plotDive(get_dive_by_uniq_id(id), 0);
}

int ProfileView::numDC() const
{
	return d ? number_of_computers(d) : 0;
}

void ProfileView::pinchStart()
{
	zoomLevelPinchStart = zoomLevel;
}

void ProfileView::pinch(double factor)
{
	setZoom(zoomLevelPinchStart * factor);
}

void ProfileView::nextDC()
{
	rotateDC(1);
}

void ProfileView::prevDC()
{
	rotateDC(-1);
}

void ProfileView::rotateDC(int dir)
{
	int num = numDC();
	if (num <= 1)
		return;
	dc = (dc + dir) % num;
	if (dc < 0)
		dc += num;
	replot();
}

void ProfileView::panStart(double x, double y)
{
	panningOriginalMousePosition = x;
	panningOriginalProfilePosition = zoomedPosition;
}

void ProfileView::pan(double x, double y)
{
	double oldPos = zoomedPosition;
	zoomedPosition = profileScene->calcZoomPosition(zoomLevel,
							panningOriginalProfilePosition,
							panningOriginalMousePosition - x);
	if (oldPos != zoomedPosition)
		plotDive(d, dc, RenderFlags::Instant | RenderFlags::DontRecalculatePlotInfo); // TODO: animations don't work when scrolling
}
