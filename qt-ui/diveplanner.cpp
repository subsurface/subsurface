#include "diveplanner.h"
#include "graphicsview-common.h"
#include "models.h"
#include "modeldelegates.h"
#include "mainwindow.h"
#include "maintab.h"
#include "tableview.h"

#include "../dive.h"
#include "../divelist.h"
#include "../planner.h"
#include "display.h"
#include "helpers.h"

#include <QMouseEvent>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QMessageBox>
#include <QListView>
#include <QModelIndex>
#include <QSettings>
#include <QTableView>
#include <QColor>

#include <algorithm>

#define TIME_INITIAL_MAX 30

#define MAX_DEPTH M_OR_FT(150, 450)
#define MIN_DEPTH M_OR_FT(20, 60)

QString gasToStr(const int o2Permille, const int hePermille)
{
	uint o2 = (o2Permille + 5) / 10, he = (hePermille + 5) / 10;
	QString result = is_air(o2Permille, hePermille) ? QObject::tr("AIR") : he == 0 ? QString("EAN%1").arg(o2, 2, 10, QChar('0')) : QString("%1/%2").arg(o2).arg(he);
	return result;
}

QString dpGasToStr(const divedatapoint &p)
{
	return gasToStr(p.o2, p.he);
}

static DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();

DivePlannerGraphics::DivePlannerGraphics(QWidget *parent) : QGraphicsView(parent),
	verticalLine(new QGraphicsLineItem(fromPercent(0, Qt::Horizontal), fromPercent(0, Qt::Vertical), fromPercent(0, Qt::Horizontal), fromPercent(100, Qt::Vertical))),
	horizontalLine(new QGraphicsLineItem(fromPercent(0, Qt::Horizontal), fromPercent(0, Qt::Vertical), fromPercent(100, Qt::Horizontal), fromPercent(0, Qt::Vertical))),
	activeDraggedHandler(0),
	diveBg(new QGraphicsPolygonItem()),
	timeLine(new Ruler()),
	timeString(new QGraphicsSimpleTextItem()),
	depthString(new QGraphicsSimpleTextItem()),
	depthHandler(new ExpanderGraphics()),
	timeHandler(new ExpanderGraphics()),
	minMinutes(TIME_INITIAL_MAX),
	minDepth(M_OR_FT(40, 120)),
	dpMaxTime(0)
{
	setBackgroundBrush(profile_color[BACKGROUND].at(0));
	setMouseTracking(true);
	setScene(new QGraphicsScene());
	scene()->setSceneRect(0, 0, 1920, 1080);

	verticalLine->setPen(QPen(Qt::DotLine));
	scene()->addItem(verticalLine);

	horizontalLine->setPen(QPen(Qt::DotLine));
	scene()->addItem(horizontalLine);

	timeLine->setMinimum(0);
	timeLine->setMaximum(TIME_INITIAL_MAX);
	timeLine->setTickInterval(10);
	timeLine->setColor(getColor(TIME_GRID));
	timeLine->setLine(fromPercent(10, Qt::Horizontal),
			  fromPercent(85, Qt::Vertical),
			  fromPercent(90, Qt::Horizontal),
			  fromPercent(85, Qt::Vertical));
	timeLine->setOrientation(Qt::Horizontal);
	timeLine->setTickSize(fromPercent(1, Qt::Vertical));
	timeLine->setTextColor(getColor(TIME_TEXT));
	timeLine->updateTicks();
	scene()->addItem(timeLine);

	depthLine = new Ruler();
	depthLine->setMinimum(0);
	depthLine->setMaximum(M_OR_FT(40, 120));
	depthLine->setTickInterval(M_OR_FT(10, 30));
	depthLine->setLine(fromPercent(10, Qt::Horizontal),
			   fromPercent(10, Qt::Vertical),
			   fromPercent(10, Qt::Horizontal),
			   fromPercent(85, Qt::Vertical));
	depthLine->setOrientation(Qt::Vertical);
	depthLine->setTickSize(fromPercent(1, Qt::Horizontal));
	depthLine->setColor(getColor(DEPTH_GRID));
	depthLine->setTextColor(getColor(SAMPLE_DEEP));
	depthLine->updateTicks();
	depthLine->unitSystem = prefs.units.length;
	scene()->addItem(depthLine);

	timeString->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	timeString->setBrush(profile_color[TIME_TEXT].at(0));
	scene()->addItem(timeString);

	depthString->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	depthString->setBrush(profile_color[SAMPLE_DEEP].at(0));
	scene()->addItem(depthString);

	diveBg->setPen(QPen(QBrush(), 0));
	scene()->addItem(diveBg);

	QString incrText;
	if (prefs.units.length == units::METERS)
		incrText = tr("10m");
	else
		incrText = tr("30ft");

	timeHandler->increaseBtn->setPixmap(QString(":plan_plus"));
	timeHandler->decreaseBtn->setPixmap(QString(":plan_minus"));
	timeHandler->icon->setPixmap(QString(":icon_time"));
	connect(timeHandler->increaseBtn, SIGNAL(clicked()), this, SLOT(increaseTime()));
	connect(timeHandler->decreaseBtn, SIGNAL(clicked()), this, SLOT(decreaseTime()));
	timeHandler->setPos(fromPercent(83, Qt::Horizontal), fromPercent(100, Qt::Vertical));
	timeHandler->setZValue(-2);
	scene()->addItem(timeHandler);

	depthHandler->increaseBtn->setPixmap(QString(":arrow_down"));
	depthHandler->decreaseBtn->setPixmap(QString(":arrow_up"));
	depthHandler->icon->setPixmap(QString(":icon_depth"));
	connect(depthHandler->decreaseBtn, SIGNAL(clicked()), this, SLOT(decreaseDepth()));
	connect(depthHandler->increaseBtn, SIGNAL(clicked()), this, SLOT(increaseDepth()));
	depthHandler->setPos(fromPercent(0, Qt::Horizontal), fromPercent(100, Qt::Vertical));
	depthHandler->setZValue(-2);
	scene()->addItem(depthHandler);

	QAction *action = NULL;

#define ADD_ACTION(SHORTCUT, Slot)                      \
	action = new QAction(this);                     \
	action->setShortcut(SHORTCUT);                  \
	action->setShortcutContext(Qt::WindowShortcut); \
	addAction(action);                              \
	connect(action, SIGNAL(triggered(bool)), this, SLOT(Slot))

	ADD_ACTION(Qt::Key_Escape, keyEscAction());
	ADD_ACTION(Qt::Key_Delete, keyDeleteAction());
	ADD_ACTION(Qt::Key_Up, keyUpAction());
	ADD_ACTION(Qt::Key_Down, keyDownAction());
	ADD_ACTION(Qt::Key_Left, keyLeftAction());
	ADD_ACTION(Qt::Key_Right, keyRightAction());
#undef ADD_ACTION

	connect(plannerModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(drawProfile()));
	connect(plannerModel, SIGNAL(cylinderModelEdited()), this, SLOT(drawProfile()));

	connect(plannerModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
		this, SLOT(pointInserted(const QModelIndex &, int, int)));
	connect(plannerModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
		this, SLOT(pointsRemoved(const QModelIndex &, int, int)));
	setRenderHint(QPainter::Antialiasing);
}

void DivePlannerGraphics::settingsChanged()
{
	if (depthLine->unitSystem == prefs.units.length)
		return;

	depthLine->setTickInterval(M_OR_FT(10, 30));
	depthLine->updateTicks();
	depthLine->unitSystem = prefs.units.length;
}

void DivePlannerGraphics::pointInserted(const QModelIndex &parent, int start, int end)
{
	DiveHandler *item = new DiveHandler();
	scene()->addItem(item);
	handles << item;

	QGraphicsSimpleTextItem *gasChooseBtn = new QGraphicsSimpleTextItem();
	scene()->addItem(gasChooseBtn);
	gasChooseBtn->setZValue(10);
	gasChooseBtn->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	gases << gasChooseBtn;
	if (plannerModel->recalcQ())
		drawProfile();
}

void DivePlannerGraphics::keyDownAction()
{
	Q_FOREACH(QGraphicsItem * i, scene()->selectedItems()) {
		if (DiveHandler *handler = qgraphicsitem_cast<DiveHandler *>(i)) {
			int row = handles.indexOf(handler);
			divedatapoint dp = plannerModel->at(row);
			if (dp.depth >= depthLine->maximum())
				continue;

			dp.depth += M_OR_FT(1, 5);
			plannerModel->editStop(row, dp);
		}
	}
}

void DivePlannerGraphics::keyUpAction()
{
	Q_FOREACH(QGraphicsItem * i, scene()->selectedItems()) {
		if (DiveHandler *handler = qgraphicsitem_cast<DiveHandler *>(i)) {
			int row = handles.indexOf(handler);
			divedatapoint dp = plannerModel->at(row);

			if (dp.depth <= 0)
				continue;

			dp.depth -= M_OR_FT(1, 5);
			plannerModel->editStop(row, dp);
		}
	}
	drawProfile();
}

void DivePlannerGraphics::keyLeftAction()
{
	Q_FOREACH(QGraphicsItem * i, scene()->selectedItems()) {
		if (DiveHandler *handler = qgraphicsitem_cast<DiveHandler *>(i)) {
			int row = handles.indexOf(handler);
			divedatapoint dp = plannerModel->at(row);

			if (dp.time / 60 <= 0)
				continue;

			// don't overlap positions.
			// maybe this is a good place for a 'goto'?
			double xpos = timeLine->posAtValue((dp.time - 60) / 60);
			bool nextStep = false;
			Q_FOREACH(DiveHandler * h, handles) {
				if (IS_FP_SAME(h->pos().x(), xpos)) {
					nextStep = true;
					break;
				}
			}
			if (nextStep)
				continue;

			dp.time -= 60;
			plannerModel->editStop(row, dp);
		}
	}
}

void DivePlannerGraphics::keyRightAction()
{
	Q_FOREACH(QGraphicsItem * i, scene()->selectedItems()) {
		if (DiveHandler *handler = qgraphicsitem_cast<DiveHandler *>(i)) {
			int row = handles.indexOf(handler);
			divedatapoint dp = plannerModel->at(row);
			if (dp.time / 60 >= timeLine->maximum())
				continue;

			// don't overlap positions.
			// maybe this is a good place for a 'goto'?
			double xpos = timeLine->posAtValue((dp.time + 60) / 60);
			bool nextStep = false;
			Q_FOREACH(DiveHandler * h, handles) {
				if (IS_FP_SAME(h->pos().x(), xpos)) {
					nextStep = true;
					break;
				}
			}
			if (nextStep)
				continue;

			dp.time += 60;
			plannerModel->editStop(row, dp);
		}
	}
}

void DivePlannerGraphics::keyDeleteAction()
{
	int selCount = scene()->selectedItems().count();
	if (selCount) {
		QVector<int> selectedIndexes;
		Q_FOREACH(QGraphicsItem * i, scene()->selectedItems()) {
			if (DiveHandler *handler = qgraphicsitem_cast<DiveHandler *>(i)) {
				selectedIndexes.push_back(handles.indexOf(handler));
			}
		}
		plannerModel->removeSelectedPoints(selectedIndexes);
	}
}

void DivePlannerGraphics::pointsRemoved(const QModelIndex &, int start, int end)
{ // start and end are inclusive.
	int num = (end - start) + 1;
	for (int i = num; i != 0; i--) {
		delete handles.back();
		handles.pop_back();
		delete gases.back();
		gases.pop_back();
	}
	scene()->clearSelection();
	drawProfile();
}

bool intLessThan(int a, int b)
{
	return a <= b;
}
void DivePlannerPointsModel::removeSelectedPoints(const QVector<int> &rows)
{
	int firstRow = rowCount() - rows.count();
	QVector<int> v2 = rows;
	std::sort(v2.begin(), v2.end(), intLessThan);
	beginRemoveRows(QModelIndex(), firstRow, rowCount() - 1);
	for (int i = v2.count() - 1; i >= 0; i--) {
		divepoints.remove(v2[i]);
	}
	endRemoveRows();
}

void DivePlannerGraphics::keyEscAction()
{
	if (scene()->selectedItems().count()) {
		scene()->clearSelection();
		return;
	}
	if (DivePlannerPointsModel::instance()->isPlanner())
		plannerModel->cancelPlan();
}

qreal DivePlannerGraphics::fromPercent(qreal percent, Qt::Orientation orientation)
{
	qreal total = orientation == Qt::Horizontal ? sceneRect().width() : sceneRect().height();
	qreal result = (total * percent) / 100;
	return result;
}

void DivePlannerGraphics::increaseDepth()
{
	if (depthLine->maximum() + M_OR_FT(10, 30) > MAX_DEPTH)
		return;
	minDepth += M_OR_FT(10, 30);
	depthLine->setMaximum(minDepth);
	depthLine->updateTicks();
	drawProfile();
}

void DivePlannerGraphics::increaseTime()
{
	minMinutes += 10;
	timeLine->setMaximum(minMinutes);
	timeLine->updateTicks();
	drawProfile();
}

void DivePlannerGraphics::decreaseDepth()
{
	if (depthLine->maximum() - M_OR_FT(10, 30) < MIN_DEPTH)
		return;

	Q_FOREACH(DiveHandler * d, handles) {
		if (depthLine->valueAt(d->pos()) > depthLine->maximum() - M_OR_FT(10, 30)) {
			QMessageBox::warning(MainWindow::instance(),
					     tr("Handler Position Error"),
					     tr("One or more of your stops will be lost with this operations, \n"
						"Please, remove them first."));
			return;
		}
	}
	minDepth -= M_OR_FT(10, 30);
	depthLine->setMaximum(minDepth);
	depthLine->updateTicks();
	drawProfile();
}

void DivePlannerGraphics::decreaseTime()
{
	if (timeLine->maximum() - 10 < TIME_INITIAL_MAX || timeLine->maximum() - 10 < dpMaxTime)
		return;

	minMinutes -= 10;
	timeLine->setMaximum(timeLine->maximum() - 10);
	timeLine->updateTicks();
	drawProfile();
}

void DivePlannerGraphics::mouseDoubleClickEvent(QMouseEvent *event)
{
	QPointF mappedPos = mapToScene(event->pos());
	if (isPointOutOfBoundaries(mappedPos))
		return;

	int minutes = rint(timeLine->valueAt(mappedPos));
	int milimeters = rint(depthLine->valueAt(mappedPos) / M_OR_FT(1, 1)) * M_OR_FT(1, 1);
	plannerModel->addStop(milimeters, minutes * 60, -1, 0, 0, true);
}

void DivePlannerPointsModel::createSimpleDive()
{
	//	plannerModel->addStop(0, 0, O2_IN_AIR, 0, 0);
	plannerModel->addStop(M_OR_FT(15, 45), 1 * 60, O2_IN_AIR, 0, 0, true);
	plannerModel->addStop(M_OR_FT(15, 45), 40 * 60, O2_IN_AIR, 0, 0, true);
	plannerModel->addStop(M_OR_FT(5, 15), 42 * 60, O2_IN_AIR, 0, 0, true);
	plannerModel->addStop(M_OR_FT(5, 15), 45 * 60, O2_IN_AIR, 0, 0, true);
}

void DivePlannerPointsModel::loadFromDive(dive *d)
{
	// We need to make a copy, because as soon as the model is modified, it will
	// remove all samples from the dive.
	memcpy(&backupDive, d, sizeof(struct dive));
	copy_samples(d, &backupDive);
	copy_events(d, &backupDive);
	copy_cylinders(d, stagingDive); // this way the correct cylinder data is shown
	CylindersModel::instance()->setDive(stagingDive);
	int lasttime = 0;
	// we start with the first gas and see if it was changed
	int o2 = get_o2(&backupDive.cylinder[0].gasmix);
	int he = get_he(&backupDive.cylinder[0].gasmix);
	for (int i = 0; i < backupDive.dc.samples - 1; i++) {
		const sample &s = backupDive.dc.sample[i];
		if (s.time.seconds == 0)
			continue;
		get_gas_from_events(&backupDive.dc, lasttime, &o2, &he);
		plannerModel->addStop(s.depth.mm, s.time.seconds, o2, he, 0, true);
		lasttime = s.time.seconds;
	}
}

void DivePlannerPointsModel::restoreBackupDive()
{
	memcpy(current_dive, &backupDive, sizeof(struct dive));
}

void DivePlannerPointsModel::copyCylinders(dive *d)
{
	copy_cylinders(stagingDive, d);
}

void DivePlannerPointsModel::copyCylindersFrom(dive *d)
{
	copy_cylinders(d, stagingDive);
}

QStringList &DivePlannerPointsModel::getGasList()
{
	struct dive *activeDive = isPlanner() ? stagingDive : current_dive;
	static QStringList list;
	list.clear();
	if (!activeDive) {
		list.push_back(tr("AIR"));
	} else {
		for (int i = 0; i < MAX_CYLINDERS; i++) {
			cylinder_t *cyl = &activeDive->cylinder[i];
			if (cylinder_nodata(cyl))
				break;
			list.push_back(gasToStr(get_o2(&cyl->gasmix), get_he(&cyl->gasmix)));
		}
	}
	return list;
}

void DivePlannerPointsModel::removeDeco()
{
	bool oldrec = setRecalc(false);
	QVector<int> computedPoints;
	for (int i = 0; i < plannerModel->rowCount(); i++)
		if (!plannerModel->at(i).entered)
			computedPoints.push_back(i);
	removeSelectedPoints(computedPoints);
	setRecalc(oldrec);
}

void DivePlannerGraphics::drawProfile()
{
	if (!plannerModel->recalcQ())
		return;
	qDeleteAll(lines);
	lines.clear();

	plannerModel->createTemporaryPlan();
	struct diveplan diveplan = plannerModel->getDiveplan();
	struct divedatapoint *dp = diveplan.dp;
	unsigned int max_depth = 0;

	if (!dp) {
		plannerModel->deleteTemporaryPlan();
		return;
	}
	//TODO: divedatapoint_list_get_max_depth on C - code?
	while (dp->next) {
		if (dp->time && dp->depth > max_depth)
			max_depth = dp->depth;
		dp = dp->next;
	}

	if (!activeDraggedHandler && (timeLine->maximum() < dp->time / 60.0 + 5 || dp->time / 60.0 + 15 < timeLine->maximum())) {
		minMinutes = fmax(dp->time / 60.0 + 5, minMinutes);
		timeLine->setMaximum(minMinutes);
		timeLine->updateTicks();
	}
	if (!activeDraggedHandler && (depthLine->maximum() < max_depth + M_OR_FT(10, 30) || max_depth + M_OR_FT(10, 30) < depthLine->maximum())) {
		minDepth = fmax(max_depth + M_OR_FT(10, 30), minDepth);
		depthLine->setMaximum(minDepth);
		depthLine->updateTicks();
	}

	// Re-position the user generated dive handlers
	int last = 0;
	for (int i = 0; i < plannerModel->rowCount(); i++) {
		struct divedatapoint datapoint = plannerModel->at(i);
		if (datapoint.time == 0) // those are the magic entries for tanks
			continue;
		DiveHandler *h = handles.at(i);
		h->setPos(timeLine->posAtValue(datapoint.time / 60), depthLine->posAtValue(datapoint.depth));
		QPointF p1 = (last == i) ? QPointF(timeLine->posAtValue(0), depthLine->posAtValue(0)) : handles[last]->pos();
		QPointF p2 = handles[i]->pos();
		QLineF line(p1, p2);
		QPointF pos = line.pointAt(0.5);
		gases[i]->setPos(pos);
		gases[i]->setText(dpGasToStr(plannerModel->at(i)));
		last = i;
	}

	// (re-) create the profile with different colors for segments that were
	// entered vs. segments that were calculated
	double lastx = timeLine->posAtValue(0);
	double lasty = depthLine->posAtValue(0);

	QPolygonF poly;
	poly.append(QPointF(lastx, lasty));

	bool oldRecalc = plannerModel->setRecalc(false);
	plannerModel->removeDeco();

	unsigned int lastdepth = 0;
	for (dp = diveplan.dp; dp != NULL; dp = dp->next) {
		if (dp->time == 0) // magic entry for available tank
			continue;
		double xpos = timeLine->posAtValue(dp->time / 60.0);
		double ypos = depthLine->posAtValue(dp->depth);
		if (!dp->entered) {
			QGraphicsLineItem *item = new QGraphicsLineItem(lastx, lasty, xpos, ypos);
			item->setPen(QPen(QBrush(Qt::red), 0));

			scene()->addItem(item);
			lines << item;
			if (dp->depth) {
				if (dp->depth == lastdepth || dp->o2 != dp->next->o2 || dp->he != dp->next->he)
					plannerModel->addStop(dp->depth, dp->time, dp->next->o2, dp->next->he, 0, false);
				lastdepth = dp->depth;
			}
		}
		lastx = xpos;
		lasty = ypos;
		poly.append(QPointF(lastx, lasty));
	}
	plannerModel->setRecalc(oldRecalc);

	diveBg->setPolygon(poly);
	QRectF b = poly.boundingRect();
	QLinearGradient pat(
	    b.x(),
	    b.y(),
	    b.x(),
	    b.height() + b.y());

	pat.setColorAt(1, profile_color[DEPTH_BOTTOM].first());
	pat.setColorAt(0, profile_color[DEPTH_TOP].first());
	diveBg->setBrush(pat);

	plannerModel->deleteTemporaryPlan();
}

void DivePlannerGraphics::resizeEvent(QResizeEvent *event)
{
	QGraphicsView::resizeEvent(event);
	fitInView(sceneRect(), Qt::IgnoreAspectRatio);
}

void DivePlannerGraphics::showEvent(QShowEvent *event)
{
	QGraphicsView::showEvent(event);
	fitInView(sceneRect(), Qt::IgnoreAspectRatio);
}

void DivePlannerGraphics::mouseMoveEvent(QMouseEvent *event)
{
	QPointF mappedPos = mapToScene(event->pos());


	double xpos = timeLine->valueAt(mappedPos);
	double ypos = depthLine->valueAt(mappedPos);

	xpos = (xpos > timeLine->maximum()) ? timeLine->posAtValue(timeLine->maximum()) : (xpos < timeLine->minimum()) ? timeLine->posAtValue(timeLine->minimum()) : timeLine->posAtValue(xpos);

	ypos = (ypos > depthLine->maximum()) ? depthLine->posAtValue(depthLine->maximum()) : (ypos < depthLine->minimum()) ? depthLine->posAtValue(depthLine->minimum()) : depthLine->posAtValue(ypos);

	verticalLine->setPos(xpos, fromPercent(0, Qt::Vertical));
	horizontalLine->setPos(fromPercent(0, Qt::Horizontal), ypos);

	depthString->setPos(fromPercent(1, Qt::Horizontal), ypos);
	timeString->setPos(xpos + 1, fromPercent(95, Qt::Vertical));

	if (isPointOutOfBoundaries(mappedPos))
		return;

	depthString->setText(get_depth_string(depthLine->valueAt(mappedPos), true, false));
	timeString->setText(QString::number(rint(timeLine->valueAt(mappedPos))) + "min");

	// calculate the correct color for the depthString.
	// QGradient doesn't returns it's interpolation, meh.
	double percent = depthLine->percentAt(mappedPos);
	QColor &startColor = profile_color[SAMPLE_SHALLOW].first();
	QColor &endColor = profile_color[SAMPLE_DEEP].first();
	short redDelta = (endColor.red() - startColor.red()) * percent + startColor.red();
	short greenDelta = (endColor.green() - startColor.green()) * percent + startColor.green();
	short blueDelta = (endColor.blue() - startColor.blue()) * percent + startColor.blue();
	depthString->setBrush(QColor(redDelta, greenDelta, blueDelta));

	if (activeDraggedHandler)
		moveActiveHandler(mappedPos, handles.indexOf(activeDraggedHandler));
	if (!handles.count())
		return;

	if (handles.last()->x() > mappedPos.x()) {
		verticalLine->setPen(QPen(QBrush(Qt::red), 0, Qt::SolidLine));
		horizontalLine->setPen(QPen(QBrush(Qt::red), 0, Qt::SolidLine));
	} else {
		verticalLine->setPen(QPen(Qt::DotLine));
		horizontalLine->setPen(QPen(Qt::DotLine));
	}
}

void DivePlannerGraphics::moveActiveHandler(const QPointF &mappedPos, const int pos)
{
	divedatapoint data = plannerModel->at(pos);
	int mintime = 0, maxtime = (timeLine->maximum() + 10) * 60;
	if (pos > 0)
		mintime = plannerModel->at(pos - 1).time;
	if (pos < plannerModel->size() - 1)
		maxtime = plannerModel->at(pos + 1).time;

	int minutes = rint(timeLine->valueAt(mappedPos));
	if (minutes * 60 <= mintime || minutes * 60 >= maxtime)
		return;

	int milimeters = rint(depthLine->valueAt(mappedPos) / M_OR_FT(1, 1)) * M_OR_FT(1, 1);
	double xpos = timeLine->posAtValue(minutes);
	double ypos = depthLine->posAtValue(milimeters);

	data.depth = milimeters;
	data.time = rint(timeLine->valueAt(mappedPos)) * 60;

	plannerModel->editStop(pos, data);

	activeDraggedHandler->setPos(QPointF(xpos, ypos));
	qDeleteAll(lines);
	lines.clear();
	drawProfile();
}

bool DivePlannerGraphics::isPointOutOfBoundaries(const QPointF &point)
{
	double xpos = timeLine->valueAt(point);
	double ypos = depthLine->valueAt(point);

	if (xpos > timeLine->maximum() ||
	    xpos < timeLine->minimum() ||
	    ypos > depthLine->maximum() ||
	    ypos < depthLine->minimum()) {
		return true;
	}
	return false;
}

void DivePlannerGraphics::mousePressEvent(QMouseEvent *event)
{
	if (event->modifiers()) {
		QGraphicsView::mousePressEvent(event);
		return;
	}

	QPointF mappedPos = mapToScene(event->pos());
	if (event->button() == Qt::LeftButton) {
		Q_FOREACH(QGraphicsItem * item, scene()->items(mappedPos, Qt::IntersectsItemBoundingRect, Qt::AscendingOrder, transform())) {
			if (DiveHandler *h = qgraphicsitem_cast<DiveHandler *>(item)) {
				activeDraggedHandler = h;
				activeDraggedHandler->setBrush(Qt::red);
				originalHandlerPos = activeDraggedHandler->pos();
			}
		}
	}
	QGraphicsView::mousePressEvent(event);
}

void DivePlannerGraphics::mouseReleaseEvent(QMouseEvent *event)
{
	if (activeDraggedHandler) {
		/* we already deal with all the positioning in the life update,
		 * so all we need to do here is change the color of the handler */
		activeDraggedHandler->setBrush(QBrush(Qt::white));
		activeDraggedHandler = 0;
		drawProfile();
	}
}

DiveHandler::DiveHandler() : QGraphicsEllipseItem()
{
	setRect(-5, -5, 10, 10);
	setFlag(QGraphicsItem::ItemIgnoresTransformations);
	setFlag(QGraphicsItem::ItemIsSelectable);
	setBrush(Qt::white);
	setZValue(2);
}

int DiveHandler::parentIndex()
{
	DivePlannerGraphics *view = qobject_cast<DivePlannerGraphics *>(scene()->views().first());
	return view->handles.indexOf(this);
}

void DiveHandler::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	QMenu m;
	GasSelectionModel *model = GasSelectionModel::instance();
	model->repopulate();
	int rowCount = model->rowCount();
	for (int i = 0; i < rowCount; i++) {
		QAction *action = new QAction(&m);
		action->setText(model->data(model->index(i, 0), Qt::DisplayRole).toString());
		connect(action, SIGNAL(triggered(bool)), this, SLOT(changeGas()));
		m.addAction(action);
	}
	m.addSeparator();
	m.addAction(QObject::tr("Remove this Point"), this, SLOT(selfRemove()));
	m.exec(event->screenPos());
}

void DiveHandler::selfRemove()
{
	setSelected(true);
	DivePlannerGraphics *view = qobject_cast<DivePlannerGraphics *>(scene()->views().first());
	view->keyDeleteAction();
}

void DiveHandler::changeGas()
{
	QAction *action = qobject_cast<QAction *>(sender());
	QModelIndex index = plannerModel->index(parentIndex(), DivePlannerPointsModel::GAS);
	plannerModel->setData(index, action->text());
}

void DiveHandler::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() != Qt::LeftButton)
		return;

	if (event->modifiers().testFlag(Qt::ControlModifier)) {
		setSelected(true);
	}
	// mousePressEvent 'grabs' the mouse and keyboard, annoying.
	ungrabMouse();

	/* hack. Sometimes the keyboard is grabbed, sometime it's not,
	so, let's force a grab and release, to get rid of a warning. */
	grabKeyboard();
	ungrabKeyboard();
}

void Ruler::setMaximum(double maximum)
{
	max = maximum;
}

void Ruler::setMinimum(double minimum)
{
	min = minimum;
}

void Ruler::setTextColor(const QColor &color)
{
	textColor = color;
}

void Ruler::eraseAll()
{
	qDeleteAll(ticks);
	ticks.clear();
	qDeleteAll(labels);
	labels.clear();
}

Ruler::Ruler() : unitSystem(0),
	orientation(Qt::Horizontal),
	min(0),
	max(0),
	interval(0),
	tickSize(0)
{
}

Ruler::~Ruler()
{
	eraseAll();
}

void Ruler::setOrientation(Qt::Orientation o)
{
	orientation = o;
	// position the elements on the screen.
	setMinimum(minimum());
	setMaximum(maximum());
}

void Ruler::updateTicks()
{
	eraseAll();

	QLineF m = line();
	QGraphicsLineItem *item = NULL;
	QGraphicsSimpleTextItem *label = NULL;

	double steps = (max - min) / interval;
	qreal pos;
	double currValue = min;

	if (orientation == Qt::Horizontal) {
		double stepSize = (m.x2() - m.x1()) / steps;
		for (pos = m.x1(); pos <= m.x2(); pos += stepSize, currValue += interval) {
			item = new QGraphicsLineItem(pos, m.y1(), pos, m.y1() + tickSize, this);
			item->setPen(pen());
			ticks.push_back(item);

			label = new QGraphicsSimpleTextItem(QString::number(currValue), this);
			label->setBrush(QBrush(textColor));
			label->setFlag(ItemIgnoresTransformations);
			label->setPos(pos - label->boundingRect().width() / 2, m.y1() + tickSize + 5);
			labels.push_back(label);
		}
	} else {
		double stepSize = (m.y2() - m.y1()) / steps;
		for (pos = m.y1(); pos <= m.y2(); pos += stepSize, currValue += interval) {
			item = new QGraphicsLineItem(m.x1(), pos, m.x1() - tickSize, pos, this);
			item->setPen(pen());
			ticks.push_back(item);

			label = new QGraphicsSimpleTextItem(get_depth_string(currValue, false, false), this);
			label->setBrush(QBrush(textColor));
			label->setFlag(ItemIgnoresTransformations);
			label->setPos(m.x2() - 80, pos);
			labels.push_back(label);
		}
	}
}

void Ruler::setTickSize(qreal size)
{
	tickSize = size;
}

void Ruler::setTickInterval(double i)
{
	interval = i;
}

qreal Ruler::valueAt(const QPointF &p)
{
	QLineF m = line();
	double retValue = orientation == Qt::Horizontal ?
			      max * (p.x() - m.x1()) / (m.x2() - m.x1()) :
			      max * (p.y() - m.y1()) / (m.y2() - m.y1());
	return retValue;
}

qreal Ruler::posAtValue(qreal value)
{
	QLineF m = line();
	double size = max - min;
	double percent = value / size;
	double realSize = orientation == Qt::Horizontal ?
			      m.x2() - m.x1() :
			      m.y2() - m.y1();
	double retValue = realSize * percent;
	retValue = (orientation == Qt::Horizontal) ?
		       retValue + m.x1() :
		       retValue + m.y1();
	return retValue;
}

qreal Ruler::percentAt(const QPointF &p)
{
	qreal value = valueAt(p);
	double size = max - min;
	double percent = value / size;
	return percent;
}

double Ruler::maximum() const
{
	return max;
}

double Ruler::minimum() const
{
	return min;
}

void Ruler::setColor(const QColor &color)
{
	QPen defaultPen(color);
	defaultPen.setJoinStyle(Qt::RoundJoin);
	defaultPen.setCapStyle(Qt::RoundCap);
	defaultPen.setWidth(2);
	defaultPen.setCosmetic(true);
	setPen(defaultPen);
}

Button::Button(QObject *parent, QGraphicsItem *itemParent) : QObject(parent),
	QGraphicsRectItem(itemParent),
	icon(new QGraphicsPixmapItem(this)),
	text(new QGraphicsSimpleTextItem(this))
{
	icon->setPos(0, 0);
	text->setPos(0, 0);
	setFlag(ItemIgnoresTransformations);
	setPen(QPen(QBrush(), 0));
}

void Button::setPixmap(const QPixmap &pixmap)
{
	icon->setPixmap(pixmap);
	if (pixmap.isNull())
		icon->hide();
	else
		icon->show();

	setRect(childrenBoundingRect());
}

void Button::setText(const QString &t)
{
	text->setText(t);
	if (icon->pixmap().isNull()) {
		icon->hide();
		text->setPos(0, 0);
	} else {
		icon->show();
		text->setPos(22, 0);
	}
	setRect(childrenBoundingRect());
}

void Button::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	event->ignore();
	emit clicked();
}

DivePlannerWidget::DivePlannerWidget(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f)
{
	ui.setupUi(this);
	ui.tableWidget->setTitle(tr("Dive Planner Points"));
	ui.tableWidget->setModel(DivePlannerPointsModel::instance());
	DivePlannerPointsModel::instance()->setRecalc(true);
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::GAS, new AirTypesDelegate(this));
	ui.cylinderTableWidget->setTitle(tr("Available Gases"));
	ui.cylinderTableWidget->setModel(CylindersModel::instance());
	QTableView *view = ui.cylinderTableWidget->view();
	view->setColumnHidden(CylindersModel::START, true);
	view->setColumnHidden(CylindersModel::END, true);
#ifdef ENABLE_PLANNER
	view->setColumnHidden(CylindersModel::DEPTH, false);
#endif
	view->setItemDelegateForColumn(CylindersModel::TYPE, new TankInfoDelegate(this));
	connect(ui.cylinderTableWidget, SIGNAL(addButtonClicked()), DivePlannerPointsModel::instance(), SLOT(addCylinder_clicked()));
	connect(ui.tableWidget, SIGNAL(addButtonClicked()), DivePlannerPointsModel::instance(), SLOT(addStop()));

	connect(CylindersModel::instance(), SIGNAL(dataChanged(QModelIndex, QModelIndex)),
		GasSelectionModel::instance(), SLOT(repopulate()));
	connect(CylindersModel::instance(), SIGNAL(rowsInserted(QModelIndex, int, int)),
		GasSelectionModel::instance(), SLOT(repopulate()));
	connect(CylindersModel::instance(), SIGNAL(rowsRemoved(QModelIndex, int, int)),
		GasSelectionModel::instance(), SLOT(repopulate()));
	connect(CylindersModel::instance(), SIGNAL(dataChanged(QModelIndex, QModelIndex)),
		plannerModel, SLOT(emitCylinderModelEdited()));
	connect(CylindersModel::instance(), SIGNAL(rowsInserted(QModelIndex, int, int)),
		plannerModel, SLOT(emitCylinderModelEdited()));
	connect(CylindersModel::instance(), SIGNAL(rowsRemoved(QModelIndex, int, int)),
		plannerModel, SLOT(emitCylinderModelEdited()));

	ui.tableWidget->setBtnToolTip(tr("add dive data point"));
	connect(ui.startTime, SIGNAL(timeChanged(QTime)), plannerModel, SLOT(setStartTime(QTime)));
	connect(ui.ATMPressure, SIGNAL(textChanged(QString)), this, SLOT(atmPressureChanged(QString)));
	connect(ui.bottomSAC, SIGNAL(textChanged(QString)), this, SLOT(bottomSacChanged(QString)));
	connect(ui.decoStopSAC, SIGNAL(textChanged(QString)), this, SLOT(decoSacChanged(QString)));
	connect(ui.gfhigh, SIGNAL(valueChanged(int)), plannerModel, SLOT(setGFHigh(int)));
	connect(ui.gflow, SIGNAL(valueChanged(int)), plannerModel, SLOT(setGFLow(int)));
	connect(ui.lastStop, SIGNAL(toggled(bool)), plannerModel, SLOT(setLastStop6m(bool)));

	// Creating the plan
	connect(ui.buttonBox, SIGNAL(accepted()), plannerModel, SLOT(createPlan()));
	connect(ui.buttonBox, SIGNAL(rejected()), plannerModel, SLOT(cancelPlan()));
	connect(plannerModel, SIGNAL(planCreated()), MainWindow::instance(), SLOT(showProfile()));
	connect(plannerModel, SIGNAL(planCreated()), MainWindow::instance(), SLOT(refreshDisplay()));
	connect(plannerModel, SIGNAL(planCanceled()), MainWindow::instance(), SLOT(showProfile()));

	/* set defaults. */
	ui.startTime->setTime(QTime(1, 0));
	ui.ATMPressure->setText("1013");
	ui.bottomSAC->setText("20");
	ui.decoStopSAC->setText("17");
	ui.gflow->setValue(prefs.gflow);
	ui.gfhigh->setValue(prefs.gfhigh);

	setMinimumWidth(0);
	setMinimumHeight(0);
}

void DivePlannerWidget::settingsChanged()
{
	ui.gflow->setValue(prefs.gflow);
	ui.gfhigh->setValue(prefs.gfhigh);
}

void DivePlannerPointsModel::addCylinder_clicked()
{
	CylindersModel::instance()->add();
}

void DivePlannerWidget::atmPressureChanged(const QString &pressure)
{
	plannerModel->setSurfacePressure(pressure.toInt());
}

void DivePlannerWidget::bottomSacChanged(const QString &bottomSac)
{
	plannerModel->setBottomSac(bottomSac.toInt());
}

void DivePlannerWidget::decoSacChanged(const QString &decosac)
{
	plannerModel->setDecoSac(decosac.toInt());
}

void DivePlannerPointsModel::setPlanMode(Mode m)
{
	mode = m;
}

bool DivePlannerPointsModel::isPlanner()
{
	return mode == PLAN;
}

/* When the planner adds deco stops to the model, adding those should not trigger a new deco calculation.
 * We thus start the planner only when recalc is true. */

bool DivePlannerPointsModel::setRecalc(bool rec)
{
	bool old = recalc;
	recalc = rec;
	return old;
}

bool DivePlannerPointsModel::recalcQ()
{
	return recalc;
}

void DivePlannerPointsModel::emitCylinderModelEdited()
{
	if (isPlanner())
		cylinderModelEdited();
}

int DivePlannerPointsModel::columnCount(const QModelIndex &parent) const
{
	return COLUMNS;
}

QVariant DivePlannerPointsModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::DisplayRole) {
		divedatapoint p = divepoints.at(index.row());
		switch (index.column()) {
		case CCSETPOINT:
			return (double)p.po2 / 1000;
		case DEPTH:
			return rint(get_depth_units(p.depth, NULL, NULL));
		case RUNTIME:
			return p.time / 60;
		case DURATION:
			if (index.row())
				return (p.time - divepoints.at(index.row() - 1).time) / 60;
			else
				return p.time / 60;
		case GAS:
			return dpGasToStr(p);
		}
	} else if (role == Qt::DecorationRole) {
		switch (index.column()) {
		case REMOVE:
			return QIcon(":trash");
		}
	} else if (role == Qt::FontRole) {
		if (divepoints.at(index.row()).entered) {
			return defaultModelFont();
		} else {
			QFont font = defaultModelFont();
			font.setBold(true);
			return font;
		}
	}
	return QVariant();
}

bool DivePlannerPointsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	int o2 = 0;
	int he = 0;
	if (role == Qt::EditRole) {
		divedatapoint &p = divepoints[index.row()];
		switch (index.column()) {
		case DEPTH:
			p.depth = units_to_depth(value.toInt());
			break;
		case RUNTIME:
			p.time = value.toInt() * 60;
			break;
		case DURATION:
			if (index.row())
				p.time = value.toInt() * 60 + divepoints[index.row() - 1].time;
			else
				p.time = value.toInt() * 60;
			break;
		case CCSETPOINT: {
			int po2 = 0;
			QByteArray gasv = value.toByteArray();
			if (validate_po2(gasv.data(), &po2))
				p.po2 = po2;
		} break;
		case GAS:
			QByteArray gasv = value.toByteArray();
			if (validate_gas(gasv.data(), &o2, &he)) {
				p.o2 = o2;
				p.he = he;
			}
			break;
		}
		editStop(index.row(), p);
	}
	return QAbstractItemModel::setData(index, value, role);
}

QVariant DivePlannerPointsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		switch (section) {
		case DEPTH:
			return tr("Final Depth");
		case RUNTIME:
			return tr("Run time");
		case DURATION:
			return tr("Duration");
		case GAS:
			return tr("Used Gas");
		case CCSETPOINT:
			return tr("CC Set Point");
		}
	} else if (role == Qt::FontRole) {
		return defaultModelFont();
	}
	return QVariant();
}

Qt::ItemFlags DivePlannerPointsModel::flags(const QModelIndex &index) const
{
	if (index.column() != DURATION)
		return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
	else
		return QAbstractItemModel::flags(index);
}

int DivePlannerPointsModel::rowCount(const QModelIndex &parent) const
{
	return divepoints.count();
}

DivePlannerPointsModel::DivePlannerPointsModel(QObject *parent) : QAbstractTableModel(parent), mode(NOTHING), tempDive(NULL), stagingDive(NULL)
{
	memset(&diveplan, 0, sizeof(diveplan));
	memset(&backupDive, 0, sizeof(backupDive));
}

DivePlannerPointsModel *DivePlannerPointsModel::instance()
{
	static QScopedPointer<DivePlannerPointsModel> self(new DivePlannerPointsModel());
	return self.data();
}

void DivePlannerPointsModel::setBottomSac(int sac)
{
	diveplan.bottomsac = sac * 1000;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setDecoSac(int sac)
{
	diveplan.decosac = sac * 1000;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setGFHigh(const int gfhigh)
{
	diveplan.gfhigh = gfhigh;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setGFLow(const int ghflow)
{
	diveplan.gflow = ghflow;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setSurfacePressure(int pressure)
{
	diveplan.surface_pressure = pressure;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setLastStop6m(bool value)
{
	set_last_stop(value);
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setStartTime(const QTime &t)
{
	diveplan.when = (t.msec() + QDateTime::currentMSecsSinceEpoch()) / 1000 - gettimezoneoffset();
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

bool divePointsLessThan(const divedatapoint &p1, const divedatapoint &p2)
{
	return p1.time <= p2.time;
}

bool DivePlannerPointsModel::addGas(int o2, int he)
{
	struct gasmix mix;

	mix.o2.permille = o2;
	mix.he.permille = he;
	sanitize_gasmix(&mix);

	if (is_air(o2, he))
		o2 = 0;

	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &stagingDive->cylinder[i];
		if (cylinder_nodata(cyl)) {
			fill_default_cylinder(cyl);
			cyl->gasmix.o2.permille = o2;
			cyl->gasmix.he.permille = he;
			sanitize_gasmix(&cyl->gasmix);
			/* The depth to change to that gas is given by the depth where its pO2 is 1.6 bar.
			 * The user should be able to change this depth manually. */
			cyl->depth.mm = 1600 * 1000 / get_o2(&mix) * 10 - 10000;
			CylindersModel::instance()->setDive(stagingDive);
			return true;
		}
		if (!gasmix_distance(&cyl->gasmix, &mix))
			return true;
	}
	qDebug("too many gases");
	return false;
}

int DivePlannerPointsModel::lastEnteredPoint()
{
	for (int i = divepoints.count() - 1; i >= 0; i--)
		if (divepoints.at(i).entered)
			return i;
	return -1;
}

int DivePlannerPointsModel::addStop(int milimeters, int seconds, int o2, int he, int ccpoint, bool entered)
{
	if (recalcQ())
		removeDeco();

	int row = divepoints.count();
	if (seconds == 0 && milimeters == 0 && row != 0) {
		/* this is only possible if the user clicked on the 'plus' sign on the DivePoints Table */
		const divedatapoint t = divepoints.at(lastEnteredPoint());
		milimeters = t.depth;
		seconds = t.time + 600; // 10 minutes.
		o2 = t.o2;
		he = t.he;
		ccpoint = t.po2;
	} else if (seconds == 0 && milimeters == 0 && row == 0) {
		milimeters = M_OR_FT(5, 15); // 5m / 15ft
		seconds = 600;		     // 10 min
		//Default to the first defined gas, if we got one.
		cylinder_t *cyl = &stagingDive->cylinder[0];
		if (cyl) {
			o2 = get_o2(&cyl->gasmix);
			he = get_he(&cyl->gasmix);
		}
	}
	if (o2 != -1)
		if (!addGas(o2, he))
			qDebug("addGas failed"); // FIXME add error propagation

	// check if there's already a new stop before this one:
	for (int i = 0; i < row; i++) {
		const divedatapoint &dp = divepoints.at(i);
		if (dp.time == seconds) {
			row = i;
			beginRemoveRows(QModelIndex(), row, row);
			divepoints.remove(row);
			endRemoveRows();
			break;
		}
		if (dp.time > seconds) {
			row = i;
			break;
		}
	}
	if (o2 == -1) {
		if (row > 0) {
			o2 = divepoints.at(row - 1).o2;
			he = divepoints.at(row - 1).he;
		} else {
			// when we add a first data point we need to make sure that there is a
			// tank for it to use;
			// first check to the right, then to the left, but if there's nothing,
			// we simply default to AIR
			if (row < divepoints.count()) {
				o2 = divepoints.at(row).o2;
				he = divepoints.at(row).he;
			} else {
				o2 = O2_IN_AIR;
				if (!addGas(o2, 0))
					qDebug("addGas failed"); // FIXME add error propagation
			}
		}
	}

	// add the new stop
	beginInsertRows(QModelIndex(), row, row);
	divedatapoint point;
	point.depth = milimeters;
	point.time = seconds;
	point.o2 = o2;
	point.he = he;
	point.po2 = ccpoint;
	point.entered = entered;
	divepoints.append(point);
	std::sort(divepoints.begin(), divepoints.end(), divePointsLessThan);
	endInsertRows();
	return row;
}

void DivePlannerPointsModel::editStop(int row, divedatapoint newData)
{
	divepoints[row] = newData;
	std::sort(divepoints.begin(), divepoints.end(), divePointsLessThan);
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

int DivePlannerPointsModel::size()
{
	return divepoints.size();
}

divedatapoint DivePlannerPointsModel::at(int row)
{
	return divepoints.at(row);
}

void DivePlannerPointsModel::remove(const QModelIndex &index)
{
	if (index.column() != REMOVE)
		return;

	beginRemoveRows(QModelIndex(), index.row(), index.row());
	divepoints.remove(index.row());
	endRemoveRows();
}

struct diveplan DivePlannerPointsModel::getDiveplan()
{
	return diveplan;
}

void DivePlannerPointsModel::cancelPlan()
{
	if (mode == PLAN && rowCount()) {
		if (QMessageBox::warning(MainWindow::instance(), TITLE_OR_TEXT(tr("Discard the Plan?"),
									       tr("You are about to discard your plan.")),
					 QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Discard) != QMessageBox::Discard) {
			return;
		}
	}
	clear();
	emit planCanceled();
	if (mode != ADD)
		free(stagingDive);
	setPlanMode(NOTHING);
	stagingDive = NULL;
	diveplan.dp = NULL;
	CylindersModel::instance()->setDive(current_dive);
	CylindersModel::instance()->update();
}

DivePlannerPointsModel::Mode DivePlannerPointsModel::currentMode() const
{
	return mode;
}

QVector<QPair<int, int> > DivePlannerPointsModel::collectGases(struct dive *d)
{
	QVector<QPair<int, int> > l;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &d->cylinder[i];
		if (!cylinder_nodata(cyl))
			l.push_back(qMakePair(get_o2(&cyl->gasmix), get_he(&cyl->gasmix)));
	}
	return l;
}
void DivePlannerPointsModel::rememberTanks()
{
	oldGases = collectGases(stagingDive);
}

bool DivePlannerPointsModel::tankInUse(int o2, int he)
{
	for (int j = 0; j < rowCount(); j++) {
		divedatapoint &p = divepoints[j];
		if (p.time == 0) // special entries that hold the available gases
			continue;
		if (!p.entered) // removing deco gases is ok
			continue;
		if ((p.o2 == o2 && p.he == he) ||
		    (is_air(p.o2, p.he) && is_air(o2, he)))
			return true;
	}
	return false;
}

void DivePlannerPointsModel::tanksUpdated()
{
	// we don't know exactly what changed - what we care about is
	// "did a gas change on us". So we look through the diveplan to
	// see if there is a gas that is now missing and if there is, we
	// replace it with the matching new gas.
	QVector<QPair<int, int> > gases = collectGases(stagingDive);
	if (gases.count() == oldGases.count()) {
		// either nothing relevant changed, or exactly ONE gasmix changed
		for (int i = 0; i < gases.count(); i++) {
			if (gases.at(i) != oldGases.at(i)) {
				if (oldGases.count(oldGases.at(i)) > 1) {
					// we had this gas more than once, so don't
					// change segments that used this gas as it still exists
					break;
				}
				for (int j = 0; j < rowCount(); j++) {
					divedatapoint &p = divepoints[j];
					int o2 = oldGases.at(i).first;
					int he = oldGases.at(i).second;
					if ((p.o2 == o2 && p.he == he) ||
					    (is_air(p.o2, p.he) && (is_air(o2, he) || (o2 == 0 && he == 0)))) {
						p.o2 = gases.at(i).first;
						p.he = gases.at(i).second;
					}
				}
				break;
			}
		}
	}
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::clear()
{
	if (mode == ADD) {
		stagingDive = current_dive;
	} else {
		if (!stagingDive)
			stagingDive = alloc_dive();
		memset(stagingDive->cylinder, 0, MAX_CYLINDERS * sizeof(cylinder_t));
	}
	CylindersModel::instance()->setDive(stagingDive);
	if (rowCount() > 0) {
		beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
		divepoints.clear();
		endRemoveRows();
	}
	CylindersModel::instance()->clear();
}


void DivePlannerPointsModel::createTemporaryPlan()
{
	// This needs to be done in the following steps:
	// Get the user-input and calculate the dive info
	// Not sure if this is the place to create the diveplan...
	// We just start with a surface node at time = 0
	if (!stagingDive)
		return;
	//TODO: this thingy looks like it could be a good C-based function
	diveplan.dp = NULL;
	int lastIndex = -1;
	for (int i = 0; i < rowCount(); i++) {
		divedatapoint p = at(i);
		int deltaT = lastIndex != -1 ? p.time - at(lastIndex).time : p.time;
		lastIndex = i;
		if (p.entered)
			plan_add_segment(&diveplan, deltaT, p.depth, p.o2, p.he, p.po2, true);
	}
	char *cache = NULL;
	tempDive = NULL;
	struct divedatapoint *dp = NULL;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &stagingDive->cylinder[i];
		if (cyl->depth.mm) {
			dp = create_dp(0, cyl->depth.mm, get_o2(&cyl->gasmix), get_he(&cyl->gasmix), 0);
			if (diveplan.dp) {
				dp->next = diveplan.dp->next;
				diveplan.dp->next = dp;
			} else {
				dp->next = NULL;
				diveplan.dp = dp;
			}
		}
	}
#if DEBUG_PLAN
	dump_plan(&diveplan);
#endif
	if (plannerModel->recalcQ())
		plan(&diveplan, &cache, &tempDive, isPlanner());
	if (mode == ADD) {
		// copy the samples and events, but don't overwrite the cylinders
		copy_samples(tempDive, current_dive);
		copy_events(tempDive, current_dive);
	}
	// throw away the cache
	free(cache);
#if DEBUG_PLAN
	dump_plan(&diveplan);
#endif
}

void DivePlannerPointsModel::deleteTemporaryPlan()
{
	deleteTemporaryPlan(diveplan.dp);
	delete_single_dive(get_divenr(tempDive));
	tempDive = NULL;
}

void DivePlannerPointsModel::deleteTemporaryPlan(struct divedatapoint *dp)
{
	if (!dp)
		return;

	deleteTemporaryPlan(dp->next);
	free(dp);
}

void DivePlannerPointsModel::createPlan()
{
	// Ok, so, here the diveplan creates a dive,
	// puts it on the dive list, and we need to remember
	// to not delete it later. mumble. ;p
	char *cache = NULL;
	tempDive = NULL;

	if (!diveplan.dp)
		return cancelPlan();

	bool oldRecalc = plannerModel->setRecalc(false);
	removeDeco();
	createTemporaryPlan();
	plannerModel->setRecalc(oldRecalc);

	//TODO: C-based function here?
	plan(&diveplan, &cache, &tempDive, isPlanner());
	copy_cylinders(stagingDive, tempDive);
	int mean[MAX_CYLINDERS], duration[MAX_CYLINDERS];
	per_cylinder_mean_depth(tempDive, select_dc(tempDive), mean, duration);
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = tempDive->cylinder + i;
		if (cylinder_none(cyl))
			continue;
		// FIXME: The epic assumption that all the cylinders after the first is deco
		int sac = i ? diveplan.decosac : diveplan.bottomsac;
		cyl->start.mbar = cyl->type.workingpressure.mbar;
		if (cyl->type.size.mliter) {
			int consumption = ((depth_to_mbar(mean[i], tempDive) * duration[i] / 60) * sac) / (cyl->type.size.mliter / 1000);
			cyl->end.mbar = cyl->start.mbar - consumption;
		} else {
			// Cylinders without a proper size are easily emptied.
			// Don't attempt to to calculate the infinite pressure drop.
			cyl->end.mbar = 0;
		}
	}
	mark_divelist_changed(true);

	// Remove and clean the diveplan, so we don't delete
	// the dive by mistake.
	diveplan.dp = NULL;
	clear();
	planCreated();
	setPlanMode(NOTHING);
	free(stagingDive);
	stagingDive = NULL;
	oldRecalc = plannerModel->setRecalc(false);
	CylindersModel::instance()->setDive(current_dive);
	CylindersModel::instance()->update();
	plannerModel->setRecalc(oldRecalc);
}

ExpanderGraphics::ExpanderGraphics(QGraphicsItem *parent) : QGraphicsRectItem(parent),
	icon(new QGraphicsPixmapItem(this)),
	increaseBtn(new Button(0, this)),
	decreaseBtn(new Button(0, this)),
	bg(new QGraphicsPixmapItem(this)),
	leftWing(new QGraphicsPixmapItem(this)),
	rightWing(new QGraphicsPixmapItem(this))
{
	QPixmap p;
#define CREATE(item, pixmap)          \
	p = QPixmap(QString(pixmap)); \
	item->setPixmap(p);

	CREATE(icon, ":icon_time");
	CREATE(bg, ":round_base");
	CREATE(leftWing, ":left_wing");
	CREATE(rightWing, ":right_wing");
#undef CREATE

	decreaseBtn->setPixmap(QPixmap(":arrow_down"));
	increaseBtn->setPixmap(QPixmap(":arrow_up"));

	setFlag(ItemIgnoresTransformations);
	leftWing->setZValue(-2);
	rightWing->setZValue(-2);
	bg->setZValue(-1);

	leftWing->setPos(0, 0);
	bg->setPos(leftWing->pos().x() + leftWing->boundingRect().width() - 60, 5);
	rightWing->setPos(leftWing->pos().x() + leftWing->boundingRect().width() - 20, 0);
	decreaseBtn->setPos(leftWing->pos().x(), leftWing->pos().y());
	increaseBtn->setPos(rightWing->pos().x(), rightWing->pos().y());
	icon->setPos(bg->pos().x(), bg->pos().y() - 5);

	//I need to bottom align the items, I need to make the 0,0 ( orgin ) to be
	// the bottom of this item, so shift everything up.
	QRectF r = childrenBoundingRect();
	Q_FOREACH(QGraphicsItem * i, childItems()) {
		i->setPos(i->pos().x(), i->pos().y() - r.height());
	}
	setScale(0.7);
}
