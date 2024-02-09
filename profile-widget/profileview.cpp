// SPDX-License-Identifier: GPL-2.0
#include "profileview.h"
#include "pictureitem.h"
#include "profilescene.h"
#include "handleitem.h"
#include "ruleritem.h"
#include "tooltipitem.h"
#include "zvalues.h"
#include "core/dive.h"
#include "core/divelog.h"
#include "commands/command.h"
#include "core/errorhelper.h"
#include "core/imagedownloader.h"
#include "core/pref.h"
#include "core/qthelper.h" // for localFilePath()
#include "core/range.h"
#include "core/settings/qPrefDisplay.h"
#include "core/settings/qPrefPartialPressureGas.h"
#include "core/settings/qPrefTechnicalDetails.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "qt-models/diveplannermodel.h"
#include "qt-quick/chartitem.h"

#include <QAbstractAnimation>
#include <QCursor>
#include <QDebug>
#include <QDesktopServices>
#include <QElapsedTimer>

#ifndef SUBSURFACE_MOBILE
#include "core/device.h"
#include <QMenu>
#include <QInputDialog>
#endif

static const QColor mouseFollowerColor = QColor(Qt::red).lighter();

// Class templates for animations (if any). Might want to do our own.
// Calls the function object passed in the constructor with a time argument,
// where 0.0 = start at 1.0 = end.
// On the last invocation, a 1.0 literal is passed, so floating-point
// comparison is OK.
class ProfileAnimation : public QAbstractAnimation {
	int duration() const override
	{
		return speed;
	}
protected:
	// For historical reasons, speed is actually the duration
	// (i.e. the reciprocal of speed). Ouch, that hurts.
	int speed;
public:
	ProfileAnimation(int animSpeed) : speed(animSpeed)
	{
	}
};

template <typename FUNC>
class ProfileAnimationTemplate : public ProfileAnimation {
	void updateCurrentTime(int time) override
	{
		// Note: we explicitly pass 1.0 at the end, so that
		// the callee can do a simple float comparison for "end".
		func(time == speed ? 1.0
				   : static_cast<double>(time) / speed);
	}
	FUNC func;
public:
	ProfileAnimationTemplate(FUNC func, int animSpeed) :
		ProfileAnimation(animSpeed),
		func(func)
	{
		start();
	}
};

// Helper function to make creation of animations somewhat more palatable.
// Returns a null-pointer if animSpeed is <= 0 to simplify logic.
template <typename FUNC>
std::unique_ptr<ProfileAnimationTemplate<FUNC>> make_anim(FUNC func, int animSpeed)
{
	return animSpeed > 0 ? std::make_unique<ProfileAnimationTemplate<FUNC>>(func, animSpeed)
			     : std::unique_ptr<ProfileAnimationTemplate<FUNC>>();
}

ProfileView::ProfileView(QQuickItem *parent) :
	ChartView(parent, ProfileZValue::Count),
	initialized(false),
	d(nullptr),
	dc(0),
	plannerModel(nullptr),
	mode(Mode::Normal),
	simplified(false),
	dpr(1.0),
	zoomLevel(1.00),
	zoomedPosition(0.0),
	panning(false),
	empty(true),
	selectedHandleIdx(-1),
	highlightedPicture(nullptr)
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
	connect(tec, &qPrefTechnicalDetails::show_pictures_in_profileChanged , [this]() {
		if (d) {
			plotPictures(d, false);
			update();
		}
	} );
	connect(tec, &qPrefTechnicalDetails::tankbarChanged                  , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::percentagegraphChanged          , this, &ProfileView::replot);
	connect(tec, &qPrefTechnicalDetails::infoboxChanged                  , this, &ProfileView::replot);

	auto pp_gas = qPrefPartialPressureGas::instance();
	connect(pp_gas, &qPrefPartialPressureGas::pheChanged, this, &ProfileView::replot);
	connect(pp_gas, &qPrefPartialPressureGas::pn2Changed, this, &ProfileView::replot);
	connect(pp_gas, &qPrefPartialPressureGas::po2Changed, this, &ProfileView::replot);

	connect(Thumbnailer::instance(), &Thumbnailer::thumbnailChanged, this, &ProfileView::updateThumbnail, Qt::QueuedConnection);

	connect(&diveListNotifier, &DiveListNotifier::picturesRemoved, this, &ProfileView::picturesRemoved);
	connect(&diveListNotifier, &DiveListNotifier::picturesAdded, this, &ProfileView::picturesAdded);
	connect(&diveListNotifier, &DiveListNotifier::pictureOffsetChanged, this, &ProfileView::pictureOffsetChanged);

	setAcceptTouchEvents(true);
	setAcceptHoverEvents(true);
	setFocus(true); // Necessary to get keyPress events
}

ProfileView::ProfileView() : ProfileView(nullptr)
{
}

ProfileView::~ProfileView()
{
}

// Since this is a QML object, we can't easily pass the planner model in the constructor. :(
void ProfileView::setPlannerModel(DivePlannerPointsModel &model)
{
	if (plannerModel)
		fprintf(stderr, "Warning: setting plannerModel twice is not supported\n");
	plannerModel = &model;
	connect(plannerModel, &DivePlannerPointsModel::dataChanged, this, &ProfileView::replot);
	connect(plannerModel, &DivePlannerPointsModel::cylinderModelEdited, this, &ProfileView::replot);
	connect(plannerModel, &DivePlannerPointsModel::modelReset, this, &ProfileView::resetHandles);
	connect(plannerModel, &DivePlannerPointsModel::rowsInserted, this, &ProfileView::pointsInserted);
	connect(plannerModel, &DivePlannerPointsModel::rowsRemoved, this, &ProfileView::pointsRemoved);
	connect(plannerModel, &DivePlannerPointsModel::rowsMoved, this, &ProfileView::pointsMoved);
}

void ProfileView::resetPointers()
{
	profileItem.reset();
	tooltip.reset();
	ruler.reset();
	mouseFollowerHorizontal.reset();
	mouseFollowerVertical.reset();
	pictures.clear();
	handles.clear();
	highlightedPicture = nullptr;
}

// Calculate render flags for the current state.
// Used when replotting the current plit.
int ProfileView::rerenderFlags() const
{
	int flags = simplified ? RenderFlags::Simplified : RenderFlags::None;
	if (mode == Mode::Edit)
		flags |= RenderFlags::EditMode;
	else if (mode == Mode::Plan)
		flags |= RenderFlags::PlanMode;
	return flags;
}

void ProfileView::plotAreaChanged(const QSizeF &s)
{
	if (!empty)
		plotDive(d, dc, rerenderFlags() | RenderFlags::Instant);
}

void ProfileView::replot()
{
	if (!empty)
		plotDive(d, dc, rerenderFlags());
}

void ProfileView::clear()
{
	clearPictures();
	//disconnectPlannerConnections();
	if (profileScene)
		profileScene->clear();
	clearHandles();
	if (tooltip)
		tooltip->setVisible(false);
	if (ruler)
		ruler->setVisible(false);
	if (mouseFollowerHorizontal)
		mouseFollowerHorizontal->setVisible(false);
	if (mouseFollowerVertical)
		mouseFollowerVertical->setVisible(false);
	empty = true;
	d = nullptr;
	dc = 0;
}

void ProfileView::plotDive(const struct dive *dIn, int dcIn, int flags)
{
	bool diveChanged = std::exchange(d, dIn) != d;
	diveChanged |= std::exchange(dc, dcIn) != dc;
	simplified = flags & RenderFlags::Simplified;
	if (!d) {
		clear();
		return;
	}

	if (flags & RenderFlags::PlanMode)
		mode = Mode::Plan;
	else if (flags & RenderFlags::EditMode)
		mode = Mode::Edit;
	else
		mode = Mode::Normal;

	// We can't create the scene in the constructor, because we can't get the DPR property there. Oh joy!
	if (!profileScene) {
		dpr = std::clamp(property("dpr").toReal(), 0.5, 100.0);
		profileScene = std::make_unique<ProfileScene>(dpr, false, false);
	}
	// If there was no previously displayed dive, turn off animations
	if (std::exchange(empty, false))
		flags |= RenderFlags::Instant;

	// If Qt decided to destroy our canvas, recreate it
	if (!profileItem)
		profileItem = createChartItem<ChartGraphicsSceneItem>(ProfileZValue::Profile);

	profileItem->setPos(QPointF(0.0, 0.0));

	QElapsedTimer measureDuration; // let's measure how long this takes us (maybe we'll turn of TTL calculation later
	measureDuration.start();

	DivePlannerPointsModel *model = flags & (RenderFlags::EditMode | RenderFlags::PlanMode) ? plannerModel : nullptr;
	bool inPlanner = flags & RenderFlags::PlanMode;

	int animSpeed = flags & RenderFlags::Instant ? 0 : qPrefDisplay::animation_speed();
	bool calculateMax = !(flags & RenderFlags::DontCalculateMax);

	profileScene->resize(size());
	profileScene->plotDive(d, dc, animSpeed, simplified, model, inPlanner,
			       flags & RenderFlags::DontRecalculatePlotInfo,
			       calculateMax, zoomLevel, zoomedPosition);
	background = inPlanner ? QColor("#D7E3EF") : getColor(::BACKGROUND, false);
	profileItem->draw(size(), background, *profileScene);

	if ((mode == Mode::Edit || mode == Mode::Plan) && plannerModel) {
		if (diveChanged)
			resetHandles();
		placeHandles();
		plannerModel->deleteTemporaryPlan();
	} else {
		clearHandles();
	}

	// On zoom / pan don't recreate the picture thumbnails, only change their position.
	if (!inPlanner) {
		if (flags & RenderFlags::DontRecalculatePlotInfo)
			updateThumbnails();
		else
			plotPictures(d, flags);
	} else {
		clearPictures();
	}

	update();

	// OK, how long did this take us? Anything above the second is way too long,
	// so if we are calculation TTS / NDL then let's force that off.
	qint64 elapsedTime = measureDuration.elapsed();
	if (verbose)
		qDebug() << "Profile calculation for dive " << d->number << "took" << elapsedTime << "ms" << " -- calculated ceiling preference is" << prefs.calcceiling;
	if (elapsedTime > 1000 && prefs.calcndltts) {
		qPrefTechnicalDetails::set_calcndltts(false);
		report_error("%s", qPrintable(tr("Show NDL / TTS was disabled because of excessive processing time")));
	}

	if (!tooltip)
		tooltip = createChartItem<ToolTipItem>(dpr);
	if (prefs.infobox) {
		QPoint pos = mapFromGlobal(QCursor::pos()).toPoint();
		tooltip->setVisible(true);
		updateTooltip(pos, flags & RenderFlags::PlanMode, animSpeed);
	} else {
		tooltip->setVisible(false);
	}

	if (!ruler)
		ruler = std::make_unique<RulerItem>(*this, dpr);
	if (prefs.rulergraph && !(flags & RenderFlags::PlanMode) && !(flags & RenderFlags::EditMode)) {
		ruler->setVisible(true);
		updateRuler(animSpeed);
	} else {
		ruler->setVisible(false);
	}

	if (!mouseFollowerHorizontal)
		mouseFollowerHorizontal = createChartItem<ChartLineItem>(ProfileZValue::MouseFollower,
									 mouseFollowerColor, 1.0 * dpr);
	if (!mouseFollowerVertical)
		mouseFollowerVertical = createChartItem<ChartLineItem>(ProfileZValue::MouseFollower,
								       mouseFollowerColor, 1.0 * dpr);
	mouseFollowerHorizontal->setVisible(mode == Mode::Edit || mode == Mode::Plan);
	mouseFollowerVertical->setVisible(mode == Mode::Edit || mode == Mode::Plan);

	// Reset animation.
	animation = make_anim([this](double progress) { anim(progress); }, animSpeed);
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
		plotDive(d, dc, rerenderFlags() | RenderFlags::DontRecalculatePlotInfo);
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

// Helper to make menus on mobile and desktop
struct MenuEntry {
	QString text;
	std::function<void()> action;
	MenuEntry(QString text, std::function<void()> action)
		: text(text), action(std::move(action))
	{
	}
};

static void execMenu(const std::vector<MenuEntry> &entries, QPoint pos)
{
	if (entries.empty())
		return;
#ifndef SUBSURFACE_MOBILE
	QMenu m;
	for (const MenuEntry &e: entries) {
		// Dang. Qt doesn't support std::function! This is too many indirections for my taste. :(
		m.addAction(e.text, [f = e.action] { f(); });
	}
	m.exec(pos);
#endif
}

// TODO: How should that work on mobile?
void ProfileView::renameCurrentDC()
{
#ifndef SUBSURFACE_MOBILE
	bool ok;
	struct divecomputer *currentdc = mutable_dive()->get_dc(dc);
	if (!currentdc)
		return;
	// TODO: center on window by passing a QWidget as first argument!
	QString newName = QInputDialog::getText(nullptr, tr("Edit nickname"),
						tr("Set new nickname for %1 (serial %2):").arg(currentdc->model.c_str()).arg(currentdc->serial.c_str()),
						QLineEdit::Normal, get_dc_nickname(currentdc).c_str(), &ok);
	if (ok)
		Command::editDeviceNickname(currentdc, newName);
#endif
}

void ProfileView::mousePressEvent(QMouseEvent *event)
{
	// Handle dragging of items
	ChartView::mousePressEvent(event);
	if (event->isAccepted())
		return;

	// Open context menu if computer name is clicked
	if (d && profileScene->pointOnDiveComputerText(event->pos())) {
		std::vector<MenuEntry> m;
		const struct divecomputer *currentdc = d->get_dc(dc);
		if (!currentdc->deviceid && dc == 0 && d->number_of_computers() == 1)
			// nothing to do, can't rename, delete or reorder
			return;
		// create menu to show when right clicking on dive computer name
		if (dc > 0)
			m.emplace_back(tr("Make first dive computer"), [this]() {
					Command::moveDiveComputerToFront(mutable_dive(), dc);
				});
		if (d->number_of_computers() > 1) {
			m.emplace_back(tr("Delete this dive computer"), [this]() {
					Command::deleteDiveComputer(mutable_dive(), dc);
				});
			m.emplace_back(tr("Split this dive computer into own dive"), [this]() {
					Command::splitDiveComputer(mutable_dive(), dc);
				});
		}
		if (currentdc->deviceid)
			m.emplace_back(tr("Rename this dive computer"), [this] { renameCurrentDC(); });
		execMenu(m, event->globalPos());
		return;
	}

	// Check if current picture is clicked
	if (highlightedPicture &&
	    highlightedPicture->thumbnail->underMouse(event->pos()) &&
	    event->button() == Qt::LeftButton) {
		if (highlightedPicture->thumbnail->removeIconUnderMouse(event->pos())) {
			if (d) {
				dive *d_nonconst = const_cast<dive *>(d); // Ouch. Let's just make the dive pointer non-const.
				Command::removePictures({ { d_nonconst, { highlightedPicture->filename.toStdString() } } });
			}
		} else {
			QDesktopServices::openUrl(
				QUrl::fromLocalFile(localFilePath(highlightedPicture->filename))
			);
		}
		event->accept();
		return;
	}

	// Do panning
	if (event->button() == Qt::LeftButton) {
		panning = true;
		QPointF pos = mapToScene(event->pos());
		panStart(pos.x(), pos.y());
		setCursor(Qt::ClosedHandCursor);
		event->accept();
	}
}

void ProfileView::mouseReleaseEvent(QMouseEvent *event)
{
	ChartView::mouseReleaseEvent(event);

	if (panning) {
		panning = false;
		unsetCursor();
	}
	if (mode == Mode::Plan || mode == Mode::Edit)
		replot();
}

void ProfileView::mouseMoveEvent(QMouseEvent *event)
{
	ChartView::mouseMoveEvent(event);

	QPointF pos = mapToScene(event->pos());
	if (panning)
		pan(pos.x(), pos.y());

	updateMouseFollowers(pos);
}

int ProfileView::getDiveId() const
{
	return d ? d->id : -1;
}

void ProfileView::setDiveId(int id)
{
	// This is used by mobile, therefore use the simplified version
	plotDive(divelog.dives.get_by_uniq_id(id), RenderFlags::Simplified);
}

int ProfileView::numDC() const
{
	return d ? d->number_of_computers() : 0;
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
		plotDive(d, dc, rerenderFlags() | RenderFlags::Instant | RenderFlags::DontRecalculatePlotInfo); // TODO: animations don't work when scrolling
}

void ProfileView::hoverEnterEvent(QHoverEvent *)
{
}

void ProfileView::shrinkPictureItem(PictureEntry &e, int animSpeed)
{
	auto it = std::find_if(pictures.begin(), pictures.end(), [&e](const PictureEntry &e2)
			       { return &e == &e2; });
	if (it != pictures.end()) { // If we didn't find it, something is very weird.
		++it;
		if (it != pictures.end() && &*it == highlightedPicture)
			++it;
	}
	if (it != pictures.end()) {
		e.thumbnail->moveBefore(*it->thumbnail);
		if (e.durationLine)
			e.durationLine->moveBefore(*it->thumbnail);
	}
	e.thumbnail->shrink(animSpeed);
	e.animation = make_anim([this, thumbnail = e.thumbnail](double progress)
		{ thumbnail->anim(progress); update(); }, animSpeed);
}

void ProfileView::growPictureItem(PictureEntry &e, int animSpeed)
{
	e.thumbnail->grow(animSpeed);
	e.thumbnail->moveBack();
	if (e.durationLine)
		e.durationLine->moveBack();
	e.animation = make_anim([this, thumbnail = e.thumbnail](double progress)
			{ thumbnail->anim(progress); update(); }, animSpeed);
}

ProfileView::PictureEntry *ProfileView::getPictureUnderMouse(QPointF pos)
{
	// First, check highlighted picture.
	if (highlightedPicture && highlightedPicture->thumbnail->underMouse(pos))
		return highlightedPicture;

	// Do binary search using the fact that pictures are stored chronologically.
	auto it1 = std::lower_bound(pictures.begin(), pictures.end(), pos.x(), [](PictureEntry &p, double x)
				   { return p.thumbnail->right() < x; }); // Skip over pictures to left of mouse.
	auto it2 = std::lower_bound(it1, pictures.end(), pos.x(), [](PictureEntry &p, double x)
				   { return p.thumbnail->left() < x; }); // Search until pictures are right of mouse.
	// Check potential pictures from the rear, because these are on top of the prior pictures.
	auto it = std::find_if(std::reverse_iterator(it2), std::reverse_iterator(it1),
			       [pos](PictureEntry &p) { return p.thumbnail->underMouse(pos); });
	return it != std::reverse_iterator(it1) ? &*it : nullptr;
}

void ProfileView::hoverMoveEvent(QHoverEvent *event)
{
	if (!profileScene)
		return;

	// This is incredibly stupid: For some weird reason (a bug?), when
	// resizing the ToolTipItem we get spurious hoverMoveEvents, which
	// restarts the animation, giving an infinite loop.
	// Prevent this by comparing to the old mouse position.
	QPointF pos = event->pos();
	if (std::exchange(previousHoverMovePosition, pos) == previousHoverMovePosition)
		return;

	if (tooltip && prefs.infobox) {
		updateTooltip(pos, false, qPrefDisplay::animation_speed()); // TODO: plan mode
		update();
	}

	PictureEntry *pictureUnderMouse = getPictureUnderMouse(pos);
	if (pictureUnderMouse) {
		PictureEntry *oldHighlighted = std::exchange(highlightedPicture, pictureUnderMouse);
		if (highlightedPicture != oldHighlighted) {
			int animSpeed = qPrefDisplay::animation_speed();
			growPictureItem(*pictureUnderMouse, animSpeed);
			if (oldHighlighted)
				shrinkPictureItem(*oldHighlighted, animSpeed);
		}
		update();
	} else if (highlightedPicture) {
		int animSpeed = qPrefDisplay::animation_speed();
		shrinkPictureItem(*highlightedPicture, animSpeed);
		highlightedPicture = nullptr;
		update();
	}

	updateMouseFollowers(pos);
}

void ProfileView::updateMouseFollowers(QPointF pos)
{
	if (!mouseFollowerHorizontal || !mouseFollowerVertical)
		return;
	if (mode != Mode::Plan && mode != Mode::Edit)
		return;
	QRectF rect = profileScene->profileRegion;
	double x = std::clamp(pos.x(), rect.left(), rect.right());
	double y = std::clamp(pos.y(), rect.top(), rect.bottom());
	mouseFollowerHorizontal->setLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
	mouseFollowerVertical->setLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
}

void ProfileView::placeHandles()
{
	if (!plannerModel)
		return;

	int count = plannerModel->rowCount();
	if ((size_t)count != handles.size()) {
		fprintf(stderr, "Handle number inconsistent with planner model\n");
		return;
	}

	// Place the dive handles
	gasmix prevgas = gasmix_invalid;
	for (int i = 0; i < count; i++) {
		struct divedatapoint datapoint = plannerModel->at(i);
		if (datapoint.time == 0) // those are the magic entries for tanks
			continue;
		auto &h = *handles[i];
		if (!datapoint.entered) {
			h.setVisible(false, false);
			continue;
		}

		h.setPos(QPointF(profileScene->posAtTime(datapoint.time), profileScene->posAtDepth(datapoint.depth.mm)));

		gasmix gas = d->get_cylinder(datapoint.cylinderid)->gasmix;
		bool textVisible = datapoint.cylinderid >= 0 && datapoint.cylinderid < static_cast<int>(d->cylinders.size()) && prevgas != gas;
		prevgas = gas;
		if (textVisible) {
			h.setText(get_gas_string(gas));
			QPointF p1;
			if (i == 0) {
				if (prefs.drop_stone_mode)
					// place the text on the straight line from the drop to stone position
					p1 = QPointF(profileScene->posAtTime(datapoint.depth.mm / prefs.descrate),
						     profileScene->posAtDepth(datapoint.depth.mm));
				else
					// place the text on the straight line from the origin to the first position
					p1 = QPointF(profileScene->posAtTime(0), profileScene->posAtDepth(0));
			} else {
				// place the text on the line from the last position
				p1 = handles[i - 1]->getPos();
			}
			QPointF p2 = h.getPos();
			QLineF line(p1, p2);
			QPointF pos = line.pointAt(0.5);
			h.setTextPos(pos);
		}
		h.setVisible(true, textVisible);
	}
}

void ProfileView::handleSelected(int idx)
{
	selectedHandleIdx = idx;
}

// TODO: Move the editing code from the desktop-widget to here.
// The problem is that the dragging tramples on the dive struct, so we work on a copy of the dive.
void ProfileView::handleReleased(int)
{
	if (mode == Mode::Edit)
		emit stopMoved(1);
	replot();
}

void ProfileView::keyPressEvent(QKeyEvent *e)
{
	if ((mode != Mode::Edit && mode != Mode::Plan) || !plannerModel || selectedHandleIdx < 0)
		return ChartView::keyPressEvent(e);

	switch (e->key()) {
		case Qt::Key_Delete: return deleteHandle();
		case Qt::Key_Up: return moveHandle(0, -M_OR_FT(1, 5));
		case Qt::Key_Down: return moveHandle(0, M_OR_FT(1, 5));
		case Qt::Key_Left: return moveHandle(-60, 0);
		case Qt::Key_Right: return moveHandle(60, 0);
	}
	ChartView::keyPressEvent(e);
}

// TODO: Handle multi-selection
void ProfileView::moveHandle(int time, int mm)
{
	if (!plannerModel || selectedHandleIdx < 0 || selectedHandleIdx >= plannerModel->rowCount())
		return;

	divedatapoint dp = plannerModel->at(selectedHandleIdx);

	dp.depth.mm += mm;
	dp.time += time;
	if (dp.depth.mm < 0 || dp.time < 0)
		return;
	plannerModel->editStop(selectedHandleIdx, dp);

	if (mode == Mode::Edit)
		emit stopMoved(1); // TODO: Accumulate key moves
	replot();
}

// TODO: Handle multi-selection
void ProfileView::deleteHandle()
{
	if (!plannerModel || selectedHandleIdx < 0 || selectedHandleIdx >= plannerModel->rowCount())
		return;

	std::vector<int> handleIndices { selectedHandleIdx };
	plannerModel->removeSelectedPoints(handleIndices);
	if (mode == Mode::Edit)
		emit stopRemoved(handleIndices.size());
	replot();
}

void ProfileView::handleDragged(int idx, QPointF pos)
{
	if (!plannerModel || (size_t)idx >= handles.size())
		return;

	if (!profileScene->pointOnProfile(pos))
		return;
	// Grow the time axis if necessary.
	//int minutes = lrint(profileScene->timeAxis->valueAt(pos) / 60);
	//if (minutes * 60 > profileScene->timeAxis->maximum() * 0.9)
		//profileScene->timeAxis->setBounds(0.0, profileScene->timeAxis->maximum() * 1.02);

	divedatapoint data = plannerModel->at(idx);
	data.depth.mm = profileScene->depthAt(pos) / M_OR_FT(1, 1) * M_OR_FT(1, 1);
	data.time = profileScene->timeAt(pos);

	plannerModel->editStop(idx, data);
	plotDive(d, dc, rerenderFlags() | RenderFlags::DontCalculateMax);
}

void ProfileView::pointsInserted(const QModelIndex &, int from, int to)
{
	for (int i = from; i <= to; ++i)
		handles.emplace(handles.begin() + i, std::make_unique<HandleItem>(*this, dpr, i));
	reindexHandles();

	// Note: we don't replot the dive here, because when adding multiple
	// points, these might trickle in one-by-one. Instead, the model will
	// emit a data-changed signal.
}

void ProfileView::pointsRemoved(const QModelIndex &, int start, int end)
{
	// Qt's model/view API is mad. The end-point is inclusive, which means that the empty range is [0,-1]!
	if (selectedHandleIdx >= start && selectedHandleIdx <= end)
		selectedHandleIdx = -1;
	handles.erase(handles.begin() + start, handles.begin() + end + 1);
	reindexHandles();

	// Note: we don't replot the dive here, because when removing multiple
	// points, these might trickle in one-by-one. Instead, the model will
	// emit a data-changed signal.
}

void ProfileView::pointsMoved(const QModelIndex &, int start, int end, const QModelIndex &, int row)
{
	move_in_range(handles, start, end + 1, row);
	reindexHandles();
}

void ProfileView::reindexHandles()
{
	HandleItem *old_selected = selectedHandleIdx >= 0 ? handles[selectedHandleIdx].get()
							  : nullptr;
	for (int i = 0; (size_t)i < handles.size(); ++i) {
		if (handles[i].get() == old_selected)
			selectedHandleIdx = i;
		handles[i]->setIdx(i);
	}
}

void ProfileView::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (mode != Mode::Edit && mode != Mode::Plan)
		return;

	QPointF pos = event->pos();
	if (!profileScene->pointOnProfile(pos))
		return;

	int minutes = lrint(timeAt(pos) / 60);
	int milimeters = lrint(profileScene->depthAt(pos) / M_OR_FT(1, 1)) * M_OR_FT(1, 1);
	plannerModel->addStop(milimeters, minutes * 60);
	if (mode == Mode::Edit)
		emit stopAdded();
}

void ProfileView::unhighlightPicture()
{
	PictureEntry *oldHighlighted = std::exchange(highlightedPicture, nullptr);
	int animSpeed = qPrefDisplay::animation_speed();
	if (oldHighlighted)
		shrinkPictureItem(*oldHighlighted, animSpeed);
}

int ProfileView::timeAt(QPointF pos) const
{
	return profileScene->timeAt(pos);
}

void ProfileView::updateTooltip(QPointF pos, bool plannerMode, int animSpeed)
{
	int time = timeAt(pos);
	auto events = profileScene->eventsAt(pos);
	tooltip->update(d, dpr, time, profileScene->getPlotInfo(), events, plannerMode, animSpeed);

	// Reset animation.
	tooltip_animation = make_anim([this](double progress)
		{ if (tooltip) tooltip->anim(progress); update(); }, animSpeed);
}

void ProfileView::rulerDragged()
{
	updateRuler(qPrefDisplay::animation_speed());
}

void ProfileView::updateRuler(int animSpeed)
{
	ruler->update(d, dpr, *profileScene, profileScene->getPlotInfo(), animSpeed);

	// Reset animation.
	ruler_animation = make_anim([this](double progress)
		{ if (ruler) ruler->anim(progress); update(); }, animSpeed);
}

// Create a PictureEntry object and add its thumbnail to the scene if profile pictures are shown.
ProfileView::PictureEntry::PictureEntry(offset_t offset, const QString &filename, ChartItemPtr<PictureItem> thumbnail, double dpr, bool synchronous) : offset(offset),
	filename(filename),
	thumbnail(thumbnail)
{
	int size = lrint(Thumbnailer::defaultThumbnailSize() * dpr);
	QImage img = Thumbnailer::instance()->fetchThumbnail(filename, false).scaled(size, size, Qt::KeepAspectRatio);
	thumbnail->setPixmap(QPixmap::fromImage(img));
}

// Define a default sort order for picture-entries: sort lexicographically by timestamp and filename.
bool ProfileView::PictureEntry::operator< (const PictureEntry &e) const
{
	// Use std::tie() for lexicographical sorting.
	return std::tie(offset.seconds, filename) < std::tie(e.offset.seconds, e.filename);
}

static constexpr double durationLineWidth = 2.5;
static constexpr double durationLinePenWidth = 1.0;

// Reset the duration line after an image was moved or we found a new duration
void ProfileView::updateDurationLine(PictureEntry &e)
{
	if (e.duration.seconds > 0) {
		// We know the duration of this video, reset the line symbolizing its extent accordingly
		double begin = profileScene->posAtTime(e.offset.seconds);
		double end = profileScene->posAtTime(e.offset.seconds + e.duration.seconds);

		if (!e.durationLine)
			e.durationLine = createChartItem<ChartRectItem>(ProfileZValue::Pictures,
									QPen(getColor(DURATION_LINE, false)),
									getColor(::BACKGROUND, false),
									durationLinePenWidth * dpr,
									false);
		e.durationLine->resize(QSizeF(end - begin, durationLineWidth * dpr));
		e.durationLine->setPos(QPointF(begin, e.y - durationLineWidth * dpr - durationLinePenWidth * dpr));
		e.durationLine->moveAfter(*e.thumbnail);
	} else {
		// This is either a picture or a video with unknown duration.
		// In case there was a line (how could that be?) remove it.
		if (e.durationLine)
			deleteChartItem(e.durationLine);
	}
}

// This function is called asynchronously by the thumbnailer if a thumbnail
// was fetched from disk or freshly calculated.
void ProfileView::updateThumbnail(QString filename, QImage thumbnail, duration_t duration)
{
	// Find the picture with the given filename
	auto it = std::find_if(pictures.begin(), pictures.end(), [&filename](const PictureEntry &e)
			       { return e.filename == filename; });

	// If we didn't find a picture, it does either not belong to the current dive,
	// or its timestamp is outside of the profile.
	if (it != pictures.end()) {
		// Replace the pixmap of the thumbnail with the newly calculated one.
		int size = lrint(Thumbnailer::defaultThumbnailSize() * dpr);
		it->thumbnail->setPixmap(QPixmap::fromImage(thumbnail.scaled(size, size, Qt::KeepAspectRatio)));

		// If the duration changed, update the line
		if (duration.seconds != it->duration.seconds) {
			it->duration = duration;
			updateDurationLine(*it);
		}

		update();
	}
}

// Calculate the y-coordinates of the thumbnails, which are supposed to be sorted by x-coordinate.
void ProfileView::calculatePictureYPositions()
{
	double lastX = -1.0, lastY = 0.0;
	constexpr double yStart = 0.05; // At which depth the thumbnails start (in fraction of total depth).
	constexpr double yStep = 0.01; // Increase of depth for overlapping thumbnails (in fraction of total depth).
	const double xSpace = 18.0 * dpr; // Horizontal range in which thumbnails are supposed to be overlapping (in pixels).
	constexpr int maxDepth = 14; // Maximal depth of thumbnail stack (in thumbnails).
	for (PictureEntry &e: pictures) {
		// Invisible items are outside of the shown range - ignore.
		if (!e.thumbnail->isVisible())
			continue;

		// Let's put the picture at the correct time, but at a fixed "depth" on the profile
		// not sure this is ideal, but it seems to look right.
		if (e.x < 0.0)
			continue;
		double y;
		if (lastX >= 0.0 && fabs(e.x - lastX) < xSpace * dpr && lastY <= (yStart + maxDepth * yStep) - 1e-10)
			y = lastY + yStep;
		else
			y = yStart;
		lastX = e.x;
		lastY = y;
		e.y = profileScene->yToScreen(y);
		e.thumbnail->setPos(QPointF(e.x, e.y));
		updateDurationLine(e); // If we changed the y-position, we also have to change the duration-line.
	}
}

void ProfileView::updateThumbnailXPos(PictureEntry &e)
{
	// Here, we only set the x-coordinate of the picture. The y-coordinate
	// will be set later in calculatePictureYPositions().
	// Thumbnails outside of the shown range are hidden.
	double time = e.offset.seconds;
	auto [min, max] = profileScene->minMaxTime();
	if (time >= min && time <= max) {
		e.x = profileScene->posAtTime(time);
		e.thumbnail->setVisible(true);
		if (e.durationLine)
			e.durationLine->setVisible(true);
	} else {
		e.thumbnail->setVisible(false);
		if (e.durationLine)
			e.durationLine->setVisible(false);
	}
}

void ProfileView::clearPictures()
{
	// The ChartItemPtrs are non-owning, so we have to delete the pictures manually. Sad.
	for (auto &e: pictures) {
		if (e.durationLine)
			deleteChartItem(e.durationLine);
		deleteChartItem(e.thumbnail);
	}
	pictures.clear();
	highlightedPicture = nullptr;
}

void ProfileView::clearHandles()
{
	// The ChartItemPtrs are non-owning, so we have to delete the handles manually. Sad.
	for (auto &handle: handles)
		handle->del();
	handles.clear();
	selectedHandleIdx = -1;
}

void ProfileView::resetHandles()
{
	if (!plannerModel)
		return;
	clearHandles();
	int count = plannerModel->rowCount();
	for (int i = 0; i < count; ++i)
		handles.push_back(std::make_unique<HandleItem>(*this, dpr, i));
}

// Helper function to compare offset_ts.
static bool operator<(offset_t o1, offset_t o2)
{
	return o1.seconds < o2.seconds;
}

// Note: the synchronous flag is currently not used.
void ProfileView::plotPictures(const struct dive *d, bool synchronous)
{
	clearPictures();

	if (!prefs.show_pictures_in_profile)
		return;

	// Collect and sort pictures, so that we can add them in the correct order.
	// Make sure the sorting function is equivalent to PictureEntry::operator<().
	std::vector<std::pair<offset_t, QString>> picturesToAdd;
	picturesToAdd.reserve(d->pictures.size());
	for (auto &picture: d->pictures) {
		if (picture.offset.seconds > 0 && picture.offset.seconds <= d->duration.seconds)
			picturesToAdd.emplace_back(picture.offset, QString::fromStdString(picture.filename));
	}
	if (picturesToAdd.empty())
		return;
	std::sort(picturesToAdd.begin(), picturesToAdd.end()); // Use lexicographical comparison of std::pair

	// Fetch all pictures of the dive, but consider only those that are within the dive time.
	// For each picture, create a PictureEntry object in the pictures-vector.
	// emplace_back() constructs an object at the end of the vector. The parameters are passed directly to the constructor.
	for (auto [offset, fn]: picturesToAdd) {
		pictures.emplace_back(offset, fn, createChartItem<PictureItem>(dpr),
				      dpr, synchronous);
	}

	updateThumbnails();
}

void ProfileView::updateThumbnails()
{
	// Calculate thumbnail positions. First the x-coordinates and and then the y-coordinates.
	for (PictureEntry &e: pictures)
		updateThumbnailXPos(e);
	calculatePictureYPositions();
}

// I dislike that we need this - the object should free its resources autonomously.
void ProfileView::removePictureThumbnail(PictureEntry &entry)
{
	if (&entry == highlightedPicture)
		highlightedPicture = nullptr;
	if (entry.durationLine)
		deleteChartItem(entry.durationLine);
	deleteChartItem(entry.thumbnail);
}

// Remove the pictures with the given filenames from the profile plot.
void ProfileView::picturesRemoved(dive *d, QVector<QString> fileUrls)
{
	if (!prefs.show_pictures_in_profile)
		return;

	unhighlightPicture();

	// Use a custom implementation of the erase-remove idiom to erase pictures:
	// https://en.wikipedia.org/wiki/Erase%E2%80%93remove_idiom
	// In contrast to std::remove_if() we can act on the item to be removed.
	// (c.f. erase-remove idiom: https://en.wikipedia.org/wiki/Erase%E2%80%93remove_idiom)
	auto it1 = pictures.begin();
	for(auto it2 = pictures.begin(); it2 != pictures.end(); ++it2) {
		// Check whether filename of entry is in list of provided filenames
		if (std::find(fileUrls.begin(), fileUrls.end(), it2->filename) != fileUrls.end()) {
			removePictureThumbnail(*it2);
		} else {
			if (it2 != it1)
				*it1 = std::move(*it2);
			it1++;
		}
	}
	pictures.erase(it1, pictures.end());
	calculatePictureYPositions();
	update();
}

void ProfileView::moveThumbnailBefore(PictureEntry &e, std::vector<PictureEntry>::iterator &before)
{
	if (before != pictures.end())
		e.thumbnail->moveBefore(*before->thumbnail);
	else
		e.thumbnail->moveBack();
	if (e.durationLine)
		e.durationLine->moveAfter(*e.thumbnail);
}

void ProfileView::picturesAdded(dive *d, QVector<picture> pics)
{
	if (!prefs.show_pictures_in_profile)
		return;

	// We might rearrange pictures, which makes the highlighted picture pointer invalid.
	unhighlightPicture();

	// Collect and sort pictures, so that we can add them in the correct order.
	// Make sure the sorting function is equivalent to PictureEntry::operator<().
	std::vector<std::pair<offset_t, QString>> picturesToAdd;
	picturesToAdd.reserve(pics.size());
	for (const picture &pic: pics) {
		if (pic.offset.seconds > 0 && pic.offset.seconds <= d->duration.seconds)
			picturesToAdd.emplace_back(pic.offset, QString::fromStdString(pic.filename));
	}
	if (picturesToAdd.empty())
		return;

	std::sort(picturesToAdd.begin(), picturesToAdd.end()); // Use lexicographical comparison of std::pair

	auto it = pictures.begin();
	for (auto &[offset, fn]: picturesToAdd) {
		// Do binary search.
		it = std::lower_bound(it, pictures.end(), std::make_pair(offset, fn),
				      [](const PictureEntry &e, const std::tuple<offset_t, QString> &p)
				      { return std::tie(e.offset, e.filename) < p; });
		it = pictures.emplace(it, offset, fn, createChartItem<PictureItem>(dpr), dpr, false);
		updateThumbnailXPos(*it);
		auto it2 = std::next(it);

		// Assert correct drawing order.
		if (it2 == pictures.end())
			it->thumbnail->moveBack();
		else
			it->thumbnail->moveBefore(*it2->thumbnail);
		it = it2;
	}

	calculatePictureYPositions();
	update();
}

void ProfileView::pictureOffsetChanged(dive *dIn, QString filename, offset_t offset)
{
	if (!prefs.show_pictures_in_profile)
		return;

	if (dIn != d)
		return; // Picture of a different dive than the one shown changed.

	// We might rearrange pictures, which makes the highlighted picture pointer invalid.
	unhighlightPicture();

	// Calculate time in dive where picture was dropped and whether the new position is during the dive.
	bool duringDive = d && offset.seconds > 0 && offset.seconds < d->duration.seconds;

	// A picture was drag&dropped onto the profile: We have four cases to consider:
	//	1a) The image was already shown on the profile and is moved to a different position on the profile.
	//	    Calculate the new position and move the picture.
	//	1b) The image was on the profile and is moved outside of the dive time.
	//	    Remove the picture.
	//	2a) The image was not on the profile and is moved into the dive time.
	//	    Add the picture to the profile.
	//	2b) The image was not on the profile and is moved outside of the dive time.
	//	    Do nothing.
	auto oldPos = std::find_if(pictures.begin(), pictures.end(), [filename](const PictureEntry &e)
				   { return e.filename == filename; });
	if (oldPos != pictures.end()) {
		// Cases 1a) and 1b): picture is on profile
		if (duringDive) {
			// Case 1a): move to new position
			// First, find new position. Note that we also have to compare filenames,
			// because it is quite easy to generate equal offsets.
			auto newPos = std::find_if(pictures.begin(), pictures.end(), [offset, &filename](const PictureEntry &e)
						   { return std::tie(e.offset.seconds, e.filename) > std::tie(offset.seconds, filename); });
			// Set new offset
			oldPos->offset.seconds = offset.seconds;
			updateThumbnailXPos(*oldPos);

			// Update drawing order
			// Move image from old to new position
			moveThumbnailBefore(*oldPos, newPos);

			int oldIndex = oldPos - pictures.begin();
			int newIndex = newPos - pictures.begin();
			move_in_range(pictures, oldIndex, oldIndex + 1, newIndex);
		} else {
			// Case 1b): remove picture
			removePictureThumbnail(*oldPos);
			pictures.erase(oldPos);
		}

		// In both cases the picture list changed, therefore we must recalculate the y-coordinates.
		calculatePictureYPositions();
	} else {
		// Cases 2a) and 2b): picture not on profile. We only have to take action for
		// the first case: picture is moved into dive-time.
		if (duringDive) {
			// Case 2a): add the picture at the appropriate position.
			// The case move from outside-to-outside of the profile plot was handled by
			// the "duringDive" condition in the if above.
			// As in the case 1a), we have to also consider filenames in the case of equal offsets.
			auto newPos = std::find_if(pictures.begin(), pictures.end(), [offset, &filename](const PictureEntry &e)
						   { return std::tie(e.offset.seconds, e.filename) > std::tie(offset.seconds, filename); });
			// emplace() constructs the element at the given position in the vector.
			// The parameters are passed directly to the contructor.
			// The call returns an iterator to the new element (which might differ from
			// the old iterator, since the buffer might have been reallocated).
			newPos = pictures.emplace(newPos, offset, filename, createChartItem<PictureItem>(dpr), dpr, false);

			// Update thumbnail paint order
			auto nextPos = std::next(newPos);
			moveThumbnailBefore(*newPos, nextPos);
			updateThumbnailXPos(*newPos);
			calculatePictureYPositions();
		}
	}
	update();
}

dive *ProfileView::mutable_dive() const
{
	return const_cast<dive *>(d);
}
