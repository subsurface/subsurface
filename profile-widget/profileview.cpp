// SPDX-License-Identifier: GPL-2.0
#include "profileview.h"
#include "profilescene.h"
#include "zvalues.h"
#include "core/errorhelper.h"
#include "core/pref.h"
#include "core/settings/qPrefTechnicalDetails.h"
#include "core/settings/qPrefDisplay.h"
#include "qt-quick/chartitem.h"

#include <QAbstractAnimation>
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

static double calcZoom(int zoomLevel)
{
	// Base of exponential zoom function: one wheel-click will increase the zoom by 15%.
	constexpr double zoomFactor = 1.15;
	return zoomLevel == 0 ? 1.0 : pow(zoomFactor, zoomLevel);
}

ProfileView::ProfileView(QQuickItem *parent) : ChartView(parent, ProfileZValue::Count),
	d(nullptr),
	dc(0),
	zoomLevel(0),
	zoomedPosition(0.0),
	empty(true),
	shouldCalculateMax(true)
{
	setBackgroundColor(Qt::black);
	setFlag(ItemHasContents, true);

	setAcceptHoverEvents(true);
	setAcceptedMouseButtons(Qt::LeftButton);
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
		double dpr = std::clamp(property("dpr").toReal(), 1.0, 100.0);
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

	double zoom = calcZoom(zoomLevel);

	int animSpeed = flags & RenderFlags::Instant ? 0 : qPrefDisplay::animation_speed();

	profileScene->resize(size());
	profileScene->plotDive(d, dc, animSpeed, model, inPlanner,
			       flags & RenderFlags::DontRecalculatePlotInfo,
			       shouldCalculateMax, zoom, zoomedPosition);
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
