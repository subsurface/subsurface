#include "diveplanner.h"
#include "graphicsview-common.h"
#include "models.h"
#include "modeldelegates.h"
#include "mainwindow.h"
#include "maintab.h"
#include "tableview.h"
#include "graphicsview-common.h"

#include "../dive.h"
#include "../divelist.h"
#include "../planner.h"
#include "helpers.h"

#include <QMouseEvent>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QMessageBox>
#include <QStringListModel>
#include <QListView>
#include <QModelIndex>
#include <QSettings>
#include <QTableView>
#include <QColor>

#include <algorithm>

#define TIME_INITIAL_MAX 30

#define MAX_DEPTH M_OR_FT(150, 450)
#define MIN_DEPTH M_OR_FT(20, 60)

#define M_OR_FT(_m,_f) ((prefs.units.length == units::METERS) ? ((_m) * 1000) : ((_f) * 304.8))

QString gasToStr(const int o2Permille, const int hePermille) {
	uint o2 = (o2Permille + 5) / 10, he = (hePermille + 5) / 10;
	QString result = is_air(o2Permille, hePermille) ? QObject::tr("AIR")
							: he == 0 ? QString("EAN%1").arg(o2, 2, 10, QChar('0'))
								  : QString("%1/%2").arg(o2).arg(he);
	return result;
}

QString dpGasToStr(const divedatapoint& p)
{
	return gasToStr(p.o2, p.he);
}

QColor getColor(const color_indice_t i)
{
	return profile_color[i].at(0);
}

static DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();

DivePlannerGraphics::DivePlannerGraphics(QWidget* parent): QGraphicsView(parent), activeDraggedHandler(0)
{
	fill_profile_color();
	setBackgroundBrush(profile_color[BACKGROUND].at(0));
	setMouseTracking(true);
	setScene(new QGraphicsScene());
	scene()->setSceneRect(0,0,1920,1080);

	verticalLine = new QGraphicsLineItem(
		fromPercent(0, Qt::Horizontal),
		fromPercent(0, Qt::Vertical),
		fromPercent(0, Qt::Horizontal),
		fromPercent(100, Qt::Vertical)
	);

	verticalLine->setPen(QPen(Qt::DotLine));
	scene()->addItem(verticalLine);

	horizontalLine = new QGraphicsLineItem(
		fromPercent(0, Qt::Horizontal),
		fromPercent(0, Qt::Vertical),
		fromPercent(100, Qt::Horizontal),
		fromPercent(0, Qt::Vertical)
	);

	horizontalLine->setPen(QPen(Qt::DotLine));
	scene()->addItem(horizontalLine);

	timeLine = new Ruler();
	timeLine->setMinimum(0);
	timeLine->setMaximum(TIME_INITIAL_MAX);
	timeLine->setTickInterval(10);
	timeLine->setColor(getColor(TIME_GRID));
	timeLine->setLine(
		fromPercent(10, Qt::Horizontal),
		fromPercent(90, Qt::Vertical),
		fromPercent(90, Qt::Horizontal),
		fromPercent(90, Qt::Vertical)
	);
	timeLine->setOrientation(Qt::Horizontal);
	timeLine->setTickSize(fromPercent(1, Qt::Vertical));
	timeLine->setTextColor(getColor(TIME_TEXT));
	timeLine->updateTicks();
	scene()->addItem(timeLine);

	depthLine = new Ruler();
	depthLine->setMinimum(0);
	depthLine->setMaximum(M_OR_FT(40,120));
	depthLine->setTickInterval(M_OR_FT(10,30));
	depthLine->setLine(
		fromPercent(10, Qt::Horizontal),
		fromPercent(10, Qt::Vertical),
		fromPercent(10, Qt::Horizontal),
		fromPercent(90, Qt::Vertical)
	);
	depthLine->setOrientation(Qt::Vertical);
	depthLine->setTickSize(fromPercent(1, Qt::Horizontal));
	depthLine->setColor(getColor(DEPTH_GRID));
	depthLine->setTextColor(getColor(SAMPLE_DEEP));
	depthLine->updateTicks();
	scene()->addItem(depthLine);

	timeString = new QGraphicsSimpleTextItem();
	timeString->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	timeString->setBrush(profile_color[TIME_TEXT].at(0));
	scene()->addItem(timeString);

	depthString = new QGraphicsSimpleTextItem();
	depthString->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	depthString->setBrush(profile_color[SAMPLE_DEEP].at(0));
	scene()->addItem(depthString);

	diveBg = new QGraphicsPolygonItem();
	diveBg->setPen(QPen(QBrush(),0));
	scene()->addItem(diveBg);

#define ADDBTN(obj, icon, text, horizontal, vertical, tooltip, value, slot) \
	obj = new Button(); \
	obj->setPixmap(QPixmap(icon)); \
	obj->setPos(fromPercent(horizontal, Qt::Horizontal), fromPercent(vertical, Qt::Vertical)); \
	obj->setToolTip(QString(tooltip.arg(value))); \
	scene()->addItem(obj); \
	connect(obj, SIGNAL(clicked()), this, SLOT(slot));

	QString incrText;
	if (prefs.units.length == units::METERS)
		incrText = tr("10m");
	else
		incrText = tr("30ft");
	ADDBTN(plusDepth, ":plus",   ""  , 5,  5, tr("Increase maximum depth by %1"), incrText, increaseDepth());
	ADDBTN(lessDepth, ":minimum",""  , 2,  5, tr("Decreases maximum depth by %1"), incrText, decreaseDepth());
	ADDBTN(plusTime,  ":plus",   ""  , 95, 95, tr("Increase minimum time by %1"), tr("10min"), increaseTime());
	ADDBTN(lessTime,  ":minimum",""  , 92, 95, tr("Decreases minimum time by %1"), tr("10min"), decreaseTime());
#undef ADDBTN
	minMinutes = TIME_INITIAL_MAX;

	QAction *action = NULL;

#define ADD_ACTION( SHORTCUT, Slot ) \
	action = new QAction(this); \
	action->setShortcut( SHORTCUT ); \
	action->setShortcutContext(Qt::WindowShortcut); \
	addAction(action); \
	connect(action, SIGNAL(triggered(bool)), this, SLOT( Slot ))

	ADD_ACTION(Qt::Key_Escape, keyEscAction());
	ADD_ACTION(Qt::Key_Delete, keyDeleteAction());
	ADD_ACTION(Qt::Key_Up, keyUpAction());
	ADD_ACTION(Qt::Key_Down, keyDownAction());
	ADD_ACTION(Qt::Key_Left, keyLeftAction());
	ADD_ACTION(Qt::Key_Right, keyRightAction());
#undef ADD_ACTION

	// Prepare the stuff for the gas-choices.
	gasListView = new QListView();
	gasListView->setWindowFlags(Qt::Popup);
	gasListView->setModel(GasSelectionModel::instance());
	gasListView->hide();
	gasListView->installEventFilter(this);

	connect(gasListView, SIGNAL(activated(QModelIndex)), this, SLOT(selectGas(QModelIndex)));
	connect(plannerModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(drawProfile()));

	connect(plannerModel, SIGNAL(rowsInserted(const QModelIndex&,int,int)),
			this, SLOT(pointInserted(const QModelIndex&, int, int)));
	connect(plannerModel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)),
			this, SLOT(pointsRemoved(const QModelIndex&, int, int)));
	setRenderHint(QPainter::Antialiasing);
}

bool DivePlannerGraphics::eventFilter(QObject *object, QEvent* event)
{
	if (object != gasListView)
		return false;
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *ke =  static_cast<QKeyEvent *>(event);
		if (ke->key() == Qt::Key_Escape)
			gasListView->hide();
	}
	if (event->type() == QEvent::MouseButtonPress){
		QMouseEvent *me = static_cast<QMouseEvent *>(event);
		if (!gasListView->geometry().contains(me->pos()))
			gasListView->hide();
	}
	return false;
}

void DivePlannerGraphics::pointInserted(const QModelIndex& parent, int start , int end)
{
	DiveHandler *item = new DiveHandler ();
	scene()->addItem(item);
	handles << item;

	Button *gasChooseBtn = new Button();
	scene()->addItem(gasChooseBtn);
	gasChooseBtn->setZValue(10);
	connect(gasChooseBtn, SIGNAL(clicked()), this, SLOT(prepareSelectGas()));

	gases << gasChooseBtn;
	drawProfile();
}

void DivePlannerGraphics::keyDownAction()
{
	Q_FOREACH(QGraphicsItem *i, scene()->selectedItems()) {
		if (DiveHandler *handler = qgraphicsitem_cast<DiveHandler*>(i)) {
			int row = handles.indexOf(handler);
			divedatapoint dp = plannerModel->at(row);
			if (dp.depth >= depthLine->maximum())
				continue;

			dp.depth += M_OR_FT(1,5);
			plannerModel->editStop(row, dp);
		}
	}
}

void DivePlannerGraphics::keyUpAction()
{
	Q_FOREACH(QGraphicsItem *i, scene()->selectedItems()) {
		if (DiveHandler *handler = qgraphicsitem_cast<DiveHandler*>(i)) {
			int row = handles.indexOf(handler);
			divedatapoint dp = plannerModel->at(row);

			if (dp.depth <= 0)
				continue;

			dp.depth -= M_OR_FT(1,5);
			plannerModel->editStop(row, dp);
		}
	}
	drawProfile();
}

void DivePlannerGraphics::keyLeftAction()
{
	Q_FOREACH(QGraphicsItem *i, scene()->selectedItems()) {
		if (DiveHandler *handler = qgraphicsitem_cast<DiveHandler*>(i)) {
			int row = handles.indexOf(handler);
			divedatapoint dp = plannerModel->at(row);

			if (dp.time / 60 <= 0)
				continue;

			// don't overlap positions.
			// maybe this is a good place for a 'goto'?
			double xpos = timeLine->posAtValue((dp.time - 60) / 60);
			bool nextStep = false;
			Q_FOREACH(DiveHandler *h, handles) {
				if (h->pos().x() == xpos) {
					nextStep = true;
					break;
				}
			}
			if(nextStep)
				continue;

			dp.time -= 60;
			plannerModel->editStop(row, dp);
		}
	}
}

void DivePlannerGraphics::keyRightAction()
{
	Q_FOREACH(QGraphicsItem *i, scene()->selectedItems()) {
		if (DiveHandler *handler = qgraphicsitem_cast<DiveHandler*>(i)) {
			int row = handles.indexOf(handler);
			divedatapoint dp = plannerModel->at(row);
			if (dp.time / 60 >= timeLine->maximum())
				continue;

			// don't overlap positions.
			// maybe this is a good place for a 'goto'?
			double xpos = timeLine->posAtValue((dp.time + 60) / 60);
			bool nextStep = false;
			Q_FOREACH(DiveHandler *h, handles) {
				if (h->pos().x() == xpos) {
					nextStep = true;
					break;
				}
			}
			if(nextStep)
				continue;

			dp.time += 60;
			plannerModel->editStop(row, dp);
		}
	}
}

void DivePlannerGraphics::keyDeleteAction()
{
	int selCount = scene()->selectedItems().count();
	if(selCount) {
		QVector<int> selectedIndexes;
		Q_FOREACH(QGraphicsItem *i, scene()->selectedItems()) {
			if (DiveHandler *handler = qgraphicsitem_cast<DiveHandler*>(i)) {
				selectedIndexes.push_back(handles.indexOf(handler));
			}
		}
		plannerModel->removeSelectedPoints(selectedIndexes);
	}
}

void DivePlannerGraphics::pointsRemoved(const QModelIndex& , int start, int end)
{	// start and end are inclusive.
	int num = (end - start) + 1;
	for(int i = num; i != 0; i--) {
		delete handles.back();
		handles.pop_back();
		delete gases.back();
		gases.pop_back();
	}
	scene()->clearSelection();
	drawProfile();
}

bool intLessThan(int a, int b) {
	return a <= b;
}
void DivePlannerPointsModel::removeSelectedPoints(const QVector< int >& rows)
{
	int firstRow = rowCount() - rows.count();
	QVector<int> v2 = rows;
	std::sort(v2.begin(), v2.end(), intLessThan);
	beginRemoveRows(QModelIndex(), firstRow, rowCount()-1);
	for(int i = v2.count()-1; i >= 0; i--) {
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
	if (depthLine->maximum() + M_OR_FT(10,30) > MAX_DEPTH)
		return;
	depthLine->setMaximum( depthLine->maximum() + M_OR_FT(10,30));
	depthLine->updateTicks();
	drawProfile();
}

void DivePlannerGraphics::increaseTime()
{
	minMinutes += 10;
	timeLine->setMaximum( minMinutes );
	timeLine->updateTicks();
	drawProfile();
}

void DivePlannerGraphics::decreaseDepth()
{
	if (depthLine->maximum() - M_OR_FT(10,30) < MIN_DEPTH)
		return;

	Q_FOREACH(DiveHandler *d, handles) {
		if (depthLine->valueAt(d->pos()) > depthLine->maximum() - M_OR_FT(10,30)) {
			QMessageBox::warning(mainWindow(),
				tr("Handler Position Error"),
				tr("One or more of your stops will be lost with this operations, \n"
					"Please, remove them first."));
			return;
		}
	}
	depthLine->setMaximum(depthLine->maximum() - M_OR_FT(10,30));
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

void DivePlannerGraphics::mouseDoubleClickEvent(QMouseEvent* event)
{
	QPointF mappedPos = mapToScene(event->pos());
	if (isPointOutOfBoundaries(mappedPos))
		return;

	int minutes = rint(timeLine->valueAt(mappedPos));
	int milimeters = rint(depthLine->valueAt(mappedPos) / M_OR_FT(1,1)) * M_OR_FT(1,1);
	plannerModel->addStop(milimeters, minutes * 60, -1, 0, 0);
}

void DivePlannerPointsModel::createSimpleDive()
{
//	plannerModel->addStop(0, 0, O2_IN_AIR, 0, 0);
	plannerModel->addStop(M_OR_FT(15,45), 1 * 60, O2_IN_AIR, 0, 0);
	plannerModel->addStop(M_OR_FT(15,45), 40 * 60, O2_IN_AIR, 0, 0);
	plannerModel->addStop(M_OR_FT(5,15), 42 * 60, O2_IN_AIR, 0, 0);
	plannerModel->addStop(M_OR_FT(5,15), 45 * 60, O2_IN_AIR, 0, 0);
}

void DivePlannerPointsModel::loadFromDive(dive* d)
{
	/* We need to make a copy, because
	 * as soon as the model is modified, it will
	 * remove all samples from the current dive.
	 * */
	struct dive *backupDive = alloc_dive();
	backupDive->when = current_dive->when; // do we need anything else?
	copy_samples(current_dive, backupDive);
	copy_cylinders(current_dive, backupDive);
	copy_events(current_dive, backupDive);
	backupSamples.clear();
	for(int i = 0; i < d->dc.samples-1; i++){
		backupSamples.push_back( d->dc.sample[i]);
	}
	copy_cylinders(current_dive, stagingDive); // this way the correct cylinder data is shown
	CylindersModel::instance()->setDive(stagingDive);
	int lasttime = 0;
	Q_FOREACH(const sample &s, backupSamples){
		int o2 = 0, he = 0;
		get_gas_from_events(&backupDive->dc, lasttime, &o2, &he);
		plannerModel->addStop(s.depth.mm, s.time.seconds, o2, he, 0);
		lasttime = s.time.seconds;
	}
}

void DivePlannerPointsModel::copyCylinders(dive *d)
{
	copy_cylinders(stagingDive, d);
}

QStringList& DivePlannerPointsModel::getGasList()
{
	static QStringList list;
	list.clear();
	if (!stagingDive) {
		list.push_back(tr("AIR"));
	} else {
		for (int i = 0; i < MAX_CYLINDERS; i++) {
			cylinder_t *cyl = &stagingDive->cylinder[i];
			if (cylinder_nodata(cyl))
				break;
			list.push_back(gasToStr(cyl->gasmix.o2.permille, cyl->gasmix.he.permille));
		}
	}
	return list;
}

void DivePlannerGraphics::prepareSelectGas()
{
	QStringListModel *model = qobject_cast<QStringListModel*>(gasListView->model());
	currentGasChoice = static_cast<Button*>(sender());
	QPoint c = QCursor::pos();
	gasListView->setGeometry(c.x(), c.y(), 150, 100);
	gasListView->show();
}

void DivePlannerGraphics::selectGas(const QModelIndex& index)
{
	QString gasSelected = gasListView->model()->data(index, Qt::DisplayRole).toString();
	int idx = gases.indexOf(currentGasChoice);
	plannerModel->setData(plannerModel->index(idx, DivePlannerPointsModel::GAS), gasSelected);
	gasListView->hide();
}

void DivePlannerGraphics::drawProfile()
{
	qDeleteAll(lines);
	lines.clear();

	plannerModel->createTemporaryPlan();
	struct diveplan diveplan = plannerModel->getDiveplan();
	struct divedatapoint *dp = diveplan.dp;
	if (!dp) {
		plannerModel->deleteTemporaryPlan();
		return;
	}
	while(dp->next)
		dp = dp->next;

	if (!activeDraggedHandler && (timeLine->maximum() < dp->time / 60.0 + 5 || dp->time / 60.0 + 15 < timeLine->maximum())) {
		double newMax = fmax(dp->time / 60.0 + 5, minMinutes);
		timeLine->setMaximum(newMax);
		timeLine->updateTicks();
	}

	// Re-position the user generated dive handlers
	int last = 0;
	for (int i = 0; i < plannerModel->rowCount(); i++) {
		divedatapoint dp = plannerModel->at(i);
		if (dp.time == 0) // those are the magic entries for tanks
			continue;
		DiveHandler *h = handles.at(i);
		h->setPos(timeLine->posAtValue(dp.time / 60), depthLine->posAtValue(dp.depth));
		QPointF p1 = handles[last]->pos();
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

	for (dp = diveplan.dp; dp != NULL; dp = dp->next) {
		if (dp->time == 0) // magic entry for available tank
			continue;
		double xpos = timeLine->posAtValue(dp->time / 60.0);
		double ypos = depthLine->posAtValue(dp->depth);
		if (!dp->entered) {
			QGraphicsLineItem *item = new QGraphicsLineItem(lastx, lasty, xpos, ypos);
			item->setPen(QPen(QBrush(Qt::red),0));
			scene()->addItem(item);
			lines << item;
		}
		lastx = xpos;
		lasty = ypos;
		poly.append(QPointF(lastx, lasty));
	}

	diveBg->setPolygon(poly);
	QRectF b = poly.boundingRect();
	QLinearGradient pat(
		b.x(),
		b.y(),
		b.x(),
		b.height() + b.y()
	);

	pat.setColorAt(1, profile_color[DEPTH_BOTTOM].first());
	pat.setColorAt(0, profile_color[DEPTH_TOP].first());
	diveBg->setBrush(pat);

	plannerModel->deleteTemporaryPlan();
}

void DivePlannerGraphics::resizeEvent(QResizeEvent* event)
{
	QGraphicsView::resizeEvent(event);
	fitInView(sceneRect(), Qt::IgnoreAspectRatio);
}

void DivePlannerGraphics::showEvent(QShowEvent* event)
{
	QGraphicsView::showEvent(event);
	fitInView(sceneRect(), Qt::IgnoreAspectRatio);
}

void DivePlannerGraphics::mouseMoveEvent(QMouseEvent* event)
{
	QPointF mappedPos = mapToScene(event->pos());
	if (isPointOutOfBoundaries(mappedPos))
		return;

	verticalLine->setPos(mappedPos.x(), fromPercent(0, Qt::Vertical));
	horizontalLine->setPos(fromPercent(0, Qt::Horizontal), mappedPos.y());

	depthString->setText(get_depth_string(depthLine->valueAt(mappedPos), true, false));
	depthString->setPos(fromPercent(1, Qt::Horizontal), mappedPos.y());
	timeString->setText(QString::number(rint(timeLine->valueAt(mappedPos))) + "min");
	timeString->setPos(mappedPos.x()+1, fromPercent(95, Qt::Vertical));

	// calculate the correct color for the depthString.
	// QGradient doesn't returns it's interpolation, meh.
	double percent = depthLine->percentAt(mappedPos);
	QColor& startColor = profile_color[SAMPLE_SHALLOW].first();
	QColor& endColor = profile_color[SAMPLE_DEEP].first();
	short redDelta = (endColor.red() - startColor.red()) * percent + startColor.red();
	short greenDelta = (endColor.green() - startColor.green()) * percent + startColor.green();
	short blueDelta = (endColor.blue() - startColor.blue()) * percent + startColor.blue();
	depthString->setBrush( QColor(redDelta, greenDelta, blueDelta));

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

void DivePlannerGraphics::moveActiveHandler(const QPointF& mappedPos, const int pos)
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

	int milimeters = rint(depthLine->valueAt(mappedPos) / M_OR_FT(1,1)) * M_OR_FT(1,1);
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

bool DivePlannerGraphics::isPointOutOfBoundaries(const QPointF& point)
{
	double xpos = timeLine->valueAt(point);
	double ypos = depthLine->valueAt(point);

	if (xpos > timeLine->maximum() ||
	    xpos < timeLine->minimum() ||
	    ypos > depthLine->maximum() ||
	    ypos < depthLine->minimum())
	{
		return true;
	}
	return false;
}

void DivePlannerGraphics::mousePressEvent(QMouseEvent* event)
{
	if (event->modifiers()) {
		QGraphicsView::mousePressEvent(event);
		return;
	}

	QPointF mappedPos = mapToScene(event->pos());
	if (event->button() == Qt::LeftButton){
		Q_FOREACH(QGraphicsItem *item, scene()->items(mappedPos, Qt::IntersectsItemBoundingRect, Qt::AscendingOrder, transform())) {
			if (DiveHandler *h = qgraphicsitem_cast<DiveHandler*>(item)) {
				activeDraggedHandler = h;
				activeDraggedHandler->setBrush(Qt::red);
				originalHandlerPos = activeDraggedHandler->pos();
			}
		}
	}
	QGraphicsView::mousePressEvent(event);
}

void DivePlannerGraphics::mouseReleaseEvent(QMouseEvent* event)
{
	if (activeDraggedHandler) {
		/* we already deal with all the positioning in the life update,
		 * so all we need to do here is change the color of the handler */
		activeDraggedHandler->setBrush(QBrush(Qt::white));
		activeDraggedHandler = 0;
		drawProfile();
	}
}

DiveHandler::DiveHandler(): QGraphicsEllipseItem()
{
	setRect(-5,-5,10,10);
	setFlag(QGraphicsItem::ItemIgnoresTransformations);
	setFlag(QGraphicsItem::ItemIsSelectable);
	setBrush(Qt::white);
	setZValue(2);
}

void DiveHandler::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
	QMenu m;
	m.addAction(QObject::tr("Remove this Point"), this, SLOT(selfRemove()));
	m.exec(event->screenPos());
}

void DiveHandler::selfRemove()
{
	setSelected(true);
	DivePlannerGraphics *view = qobject_cast<DivePlannerGraphics*>(scene()->views().first());
	view->keyDeleteAction();
}

void DiveHandler::mousePressEvent(QGraphicsSceneMouseEvent* event)
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

void Ruler::setTextColor(const QColor& color)
{
	textColor = color;
}

Ruler::Ruler() : orientation(Qt::Horizontal)
{
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
	qDeleteAll(ticks);
	ticks.clear();
	qDeleteAll(labels);
	labels.clear();

	QLineF m = line();
	QGraphicsLineItem *item = NULL;
	QGraphicsSimpleTextItem *label = NULL;

	double steps = (max - min) / interval;
	qreal pos;
	double currValue = min;

	if (orientation == Qt::Horizontal) {
		double stepSize = (m.x2() - m.x1()) / steps;
		for (pos = m.x1(); pos < m.x2(); pos += stepSize, currValue += interval) {
			item = new QGraphicsLineItem(pos, m.y1(), pos, m.y1() + tickSize, this);
			item->setPen(pen());
			ticks.push_back(item);

			label = new QGraphicsSimpleTextItem(QString::number(currValue), this);
			label->setBrush(QBrush(textColor));
			label->setFlag(ItemIgnoresTransformations);
			label->setPos(pos - label->boundingRect().width()/2, m.y1() + tickSize + 5);
			labels.push_back(label);
		}
		item = new QGraphicsLineItem(pos, m.y1(), pos, m.y1() + tickSize, this);
		item->setPen(pen());
		ticks.push_back(item);

		label = new QGraphicsSimpleTextItem(QString::number(currValue), this);
		label->setBrush(QBrush(textColor));
		label->setFlag(ItemIgnoresTransformations);
		label->setPos(pos - label->boundingRect().width()/2, m.y1() + tickSize + 5);
		labels.push_back(label);
	} else {
		double stepSize = (m.y2() - m.y1()) / steps;
		for (pos = m.y1(); pos < m.y2(); pos += stepSize, currValue += interval) {
			item = new QGraphicsLineItem(m.x1(), pos, m.x1() - tickSize, pos, this);
			item->setPen(pen());
			ticks.push_back(item);

			label = new QGraphicsSimpleTextItem(get_depth_string(currValue, false, false), this);
			label->setBrush(QBrush(textColor));
			label->setFlag(ItemIgnoresTransformations);
			label->setPos(m.x2() - 80, pos);
			labels.push_back(label);
		}
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

void Ruler::setTickSize(qreal size)
{
	tickSize = size;
}

void Ruler::setTickInterval(double i)
{
	interval = i;
}

qreal Ruler::valueAt(const QPointF& p)
{
	QLineF m = line();
	double retValue =  orientation == Qt::Horizontal ?
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
	retValue =  (orientation == Qt::Horizontal) ?
				retValue + m.x1() :
				retValue + m.y1();
	return retValue;
}

qreal Ruler::percentAt(const QPointF& p)
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

void Ruler::setColor(const QColor& color)
{
	QPen defaultPen(color);
	defaultPen.setJoinStyle(Qt::RoundJoin);
	defaultPen.setCapStyle(Qt::RoundCap);
	defaultPen.setWidth(2);
	defaultPen.setCosmetic(true);
	setPen(defaultPen);
}

Button::Button(QObject* parent): QObject(parent), QGraphicsRectItem()
{
	icon = new QGraphicsPixmapItem(this);
	text = new QGraphicsSimpleTextItem(this);
	icon->setPos(0,0);
	text->setPos(0,0);
	setFlag(ItemIgnoresTransformations);
	setPen(QPen(QBrush(), 0));
}

void Button::setPixmap(const QPixmap& pixmap)
{
	icon->setPixmap(pixmap.scaled(20,20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	if(pixmap.isNull())
		icon->hide();
	else
		icon->show();

	setRect(childrenBoundingRect());
}

void Button::setText(const QString& t)
{
	text->setText(t);
	if(icon->pixmap().isNull()) {
		icon->hide();
		text->setPos(0,0);
	} else {
		icon->show();
		text->setPos(22,0);
	}
	setRect(childrenBoundingRect());
}

void Button::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	event->ignore();
	emit clicked();
}

DivePlannerWidget::DivePlannerWidget(QWidget* parent, Qt::WindowFlags f): QWidget(parent, f)
{
	ui.setupUi(this);
	ui.tableWidget->setTitle(tr("Dive Planner Points"));
	ui.tableWidget->setModel(DivePlannerPointsModel::instance());
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::GAS, new AirTypesDelegate(this));
	ui.cylinderTableWidget->setTitle(tr("Available Gases"));
	ui.cylinderTableWidget->setModel(CylindersModel::instance());
	QTableView *view = ui.cylinderTableWidget->view();
	view->setColumnHidden(CylindersModel::START, true);
	view->setColumnHidden(CylindersModel::END, true);
	view->setColumnHidden(CylindersModel::DEPTH, false);
	view->setItemDelegateForColumn(CylindersModel::TYPE, new TankInfoDelegate());
	connect(ui.cylinderTableWidget, SIGNAL(addButtonClicked()), DivePlannerPointsModel::instance(), SLOT(addCylinder_clicked()));
	connect(ui.tableWidget, SIGNAL(addButtonClicked()), DivePlannerPointsModel::instance(), SLOT(addStop()));

	connect(CylindersModel::instance(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
		GasSelectionModel::instance(), SLOT(repopulate()));
	connect(CylindersModel::instance(), SIGNAL(rowsInserted(QModelIndex,int,int)),
		GasSelectionModel::instance(), SLOT(repopulate()));
	connect(CylindersModel::instance(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
		GasSelectionModel::instance(), SLOT(repopulate()));

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
	connect(plannerModel, SIGNAL(planCreated()), mainWindow(), SLOT(showProfile()));
	connect(plannerModel, SIGNAL(planCreated()), mainWindow(), SLOT(refreshDisplay()));
	connect(plannerModel, SIGNAL(planCanceled()), mainWindow(), SLOT(showProfile()));

	/* set defaults. */
	ui.startTime->setTime( QTime(1, 0) );
	ui.ATMPressure->setText( "1013" );
	ui.bottomSAC->setText("20");
	ui.decoStopSAC->setText("17");
	ui.gflow->setValue(prefs.gflow);
	ui.gfhigh->setValue(prefs.gfhigh);

	setMinimumWidth(0);
	setMinimumHeight(0);
}

void DivePlannerPointsModel::addCylinder_clicked()
{
	CylindersModel::instance()->add();
}

void DivePlannerWidget::atmPressureChanged(const QString& pressure)
{
	plannerModel->setSurfacePressure(pressure.toInt());
}

void DivePlannerWidget::bottomSacChanged(const QString& bottomSac)
{
	plannerModel->setBottomSac(bottomSac.toInt());
}

void DivePlannerWidget::decoSacChanged(const QString& decosac)
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

int DivePlannerPointsModel::columnCount(const QModelIndex& parent) const
{
	return COLUMNS;
}

QVariant DivePlannerPointsModel::data(const QModelIndex& index, int role) const
{
	if(role == Qt::DisplayRole) {
		divedatapoint p = divepoints.at(index.row());
		switch(index.column()) {
		case CCSETPOINT: return p.po2;
		case DEPTH: return rint(get_depth_units(p.depth, NULL, NULL));
		case DURATION: return p.time / 60;
		case GAS: return dpGasToStr(p);
		}
	} else if (role == Qt::DecorationRole) {
		switch(index.column()) {
			case REMOVE : return QIcon(":trash");
		}
	} else if (role == Qt::FontRole) {
		return  defaultModelFont();
	}
	return QVariant();
}

bool DivePlannerPointsModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	int o2 = 0;
	int he = 0;
	if(role == Qt::EditRole) {
		divedatapoint& p = divepoints[index.row()];
		switch(index.column()) {
		case DEPTH: p.depth = units_to_depth(value.toInt()); break;
		case DURATION: p.time = value.toInt() * 60; break;
		case CCSETPOINT: {
			int po2 = 0;
			QByteArray gasv = value.toByteArray();
			if (validate_po2(gasv.data(), &po2))
				p.po2 = po2;
			}
			break;
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
		switch(section) {
			case DEPTH: return tr("Final Depth");
			case DURATION: return tr("Duration");
			case GAS: return tr("Used Gas");
			case CCSETPOINT: return tr("CC Set Point");
		}
	} else if (role == Qt::FontRole) {
		return  defaultModelFont();
	}
	return QVariant();
}

Qt::ItemFlags DivePlannerPointsModel::flags(const QModelIndex& index) const
{
    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

int DivePlannerPointsModel::rowCount(const QModelIndex& parent) const
{
	return divepoints.count();
}

DivePlannerPointsModel::DivePlannerPointsModel(QObject* parent): QAbstractTableModel(parent), mode(NOTHING)
{
}

DivePlannerPointsModel* DivePlannerPointsModel::instance()
{
	static DivePlannerPointsModel* self = new DivePlannerPointsModel();
	return self;
}

void DivePlannerPointsModel::setBottomSac(int sac)
{
	diveplan.bottomsac = sac;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount()-1, COLUMNS-1));
}

void DivePlannerPointsModel::setDecoSac(int sac)
{
	diveplan.decosac = sac;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount()-1, COLUMNS-1));
}

void DivePlannerPointsModel::setGFHigh(const int gfhigh)
{
	diveplan.gfhigh = gfhigh;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount()-1, COLUMNS-1));
}

void DivePlannerPointsModel::setGFLow(const int ghflow)
{
	diveplan.gflow = ghflow;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount()-1, COLUMNS-1));
}

void DivePlannerPointsModel::setSurfacePressure(int pressure)
{
	diveplan.surface_pressure = pressure;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount()-1, COLUMNS-1));
}

void DivePlannerPointsModel::setLastStop6m(bool value)
{
	set_last_stop(value);
	emit dataChanged(createIndex(0, 0), createIndex(rowCount()-1, COLUMNS-1));
}

void DivePlannerPointsModel::setStartTime(const QTime& t)
{
	diveplan.when = (t.msec() + QDateTime::currentMSecsSinceEpoch()) / 1000 - gettimezoneoffset();
	emit dataChanged(createIndex(0, 0), createIndex(rowCount()-1, COLUMNS-1));
}

bool divePointsLessThan(const divedatapoint& p1, const divedatapoint& p2)
{
	return p1.time <= p2.time;
}

bool DivePlannerPointsModel::addGas(int o2, int he)
{
	if (is_air(o2, he))
		o2 = 0;

	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &stagingDive->cylinder[i];
		if (cylinder_nodata(cyl)) {
			fill_default_cylinder(cyl);
			cyl->gasmix.o2.permille = o2;
			cyl->gasmix.he.permille = he;
			CylindersModel::instance()->setDive(stagingDive);
			return true;
		}
		if (cyl->gasmix.o2.permille == o2 && cyl->gasmix.he.permille == he)
			return true;
	}
	qDebug("too many gases");
	return false;
}

int DivePlannerPointsModel::addStop(int milimeters, int minutes, int o2, int he, int ccpoint)
{
	int row = divepoints.count();
	if (minutes == 0 && milimeters == 0 && row != 0){
		/* this is only possible if the user clicked on the 'plus' sign on the DivePoints Table */
		struct divedatapoint& t = divepoints.last();
		milimeters = t.depth;
		minutes = t.time + 600; // 10 minutes.
	} else if (minutes == 0 && milimeters == 0 && row == 0) {
		milimeters = M_OR_FT(5, 15); // 5m / 15ft
		minutes = 600; // 10 min
	}
	if (o2 != -1)
		if (!addGas(o2, he))
			qDebug("addGas failed"); // FIXME add error propagation

	// check if there's already a new stop before this one:
	for (int i = 0; i < row; i++) {
		const divedatapoint& dp = divepoints.at(i);
		if (dp.time == minutes) {
			row = i;
			beginRemoveRows(QModelIndex(), row, row);
			divepoints.remove(row);
			endRemoveRows();
			break;
		}
		if (dp.time > minutes ) {
			row = i;
			break;
		}
	}
	if (o2 == -1) {
		qDebug() << "default Gas";
		if (row > 0) {
			qDebug() << "from left";
			o2 = divepoints.at(row - 1).o2;
			he = divepoints.at(row - 1).he;
		} else {
			// when we add a first data point we need to make sure that there is a
			// tank for it to use;
			// first check to the right, then to the left, but if there's nothing,
			// we simply default to AIR
			if (row < divepoints.count()) {
				qDebug() << "from right";
				o2 = divepoints.at(row).o2;
				he = divepoints.at(row).he;
			} else {
				qDebug() << "have to create tank of AIR" << row << divepoints.count();
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
	point.time = minutes;
	point.o2 = o2;
	point.he = he;
	point.po2 = ccpoint;
	divepoints.append( point );
	std::sort(divepoints.begin(), divepoints.end(), divePointsLessThan);
	endInsertRows();
	return row;
}

void DivePlannerPointsModel::editStop(int row, divedatapoint newData)
{
	divepoints[row] = newData;
	std::sort(divepoints.begin(), divepoints.end(), divePointsLessThan);
	emit dataChanged(createIndex(0, 0), createIndex(rowCount()-1, COLUMNS-1));
}

int DivePlannerPointsModel::size()
{
	return divepoints.size();
}

divedatapoint DivePlannerPointsModel::at(int row)
{
	return divepoints.at(row);
}

void DivePlannerPointsModel::remove(const QModelIndex& index)
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
		if (QMessageBox::warning(mainWindow(), tr("Discard the Plan?"),
			tr("You are about to discard your plan."),
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
	CylindersModel::instance()->setDive(current_dive);
	CylindersModel::instance()->update();
}

DivePlannerPointsModel::Mode DivePlannerPointsModel::currentMode() const
{
	return mode;
}

QList<QPair<int, int> > DivePlannerPointsModel::collectGases(struct dive *d)
{
	QList<QPair<int, int> > l;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &d->cylinder[i];
		if (!cylinder_nodata(cyl))
			l.push_back(QPair<int, int>(cyl->gasmix.o2.permille, cyl->gasmix.he.permille));
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
		divedatapoint& p = divepoints[j];
		if (p.time == 0) // special entries that hold the available gases
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
	QList<QPair<int,int> > gases = collectGases(stagingDive);
	if (gases.length() == oldGases.length()) {
		// either nothing relevant changed, or exactly ONE gasmix changed
		for (int i = 0; i < gases.length(); i++) {
			if (gases.at(i) != oldGases.at(i)) {
				if (oldGases.count(oldGases.at(i)) > 1) {
					// we had this gas more than once, so don't
					// change segments that used this gas as it still exists
					break;
				}
				for (int j = 0; j < rowCount(); j++) {
					divedatapoint& p = divepoints[j];
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
	emit dataChanged(createIndex(0, 0), createIndex(rowCount()-1, COLUMNS-1));
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
	beginRemoveRows(QModelIndex(), 0, rowCount()-1);
	divepoints.clear();
	endRemoveRows();
	CylindersModel::instance()->clear();
}


void DivePlannerPointsModel::createTemporaryPlan()
{
	// This needs to be done in the following steps:
	// Get the user-input and calculate the dive info
	// Not sure if this is the place to create the diveplan...
	// We just start with a surface node at time = 0
	struct divedatapoint *dp = create_dp(0, 0, 209, 0, 0);
	dp->entered = TRUE;
	diveplan.dp = dp;
	int lastIndex = -1;
	for (int i = 0; i < rowCount(); i++) {
		divedatapoint p = at(i);
		int deltaT = lastIndex != -1 ? p.time - at(lastIndex).time : p.time;
		lastIndex = i;
		dp = plan_add_segment(&diveplan, deltaT, p.depth, p.o2, p.he, p.po2);
	}
	char *cache = NULL;
	tempDive = NULL;
	const char *errorString = NULL;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &stagingDive->cylinder[i];
		if (cyl->depth.mm) {
			dp = create_dp(0, cyl->depth.mm, cyl->gasmix.o2.permille, cyl->gasmix.he.permille, 0);
			dp->next = diveplan.dp->next;
			diveplan.dp->next = dp;
		}
	}
#if DEBUG_PLAN
	dump_plan(&diveplan);
#endif
	plan(&diveplan, &cache, &tempDive, isPlanner(), &errorString);
	if (mode == ADD) {
		// copy the samples and events, but don't overwrite the cylinders
		copy_samples(tempDive, current_dive);
		copy_events(tempDive, current_dive);
	}
#if DEBUG_PLAN
	dump_plan(&diveplan);
#endif
}

void DivePlannerPointsModel::undoEdition()
{
	clear();
	Q_FOREACH(const sample &s, backupSamples){
		int o2, he;
		get_gas_from_events(&current_dive->dc, s.time.seconds, &o2, &he);
		plannerModel->addStop(s.depth.mm, s.time.seconds, o2, he, 0);
	}
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
	const char *errorString = NULL;

	createTemporaryPlan();
	plan(&diveplan, &cache, &tempDive, isPlanner(), &errorString);
	mark_divelist_changed(TRUE);

	// Remove and clean the diveplan, so we don't delete
	// the dive by mistake.
	diveplan.dp = NULL;
	clear();
	planCreated();
	setPlanMode(NOTHING);
	free(stagingDive);
	stagingDive = NULL;
	CylindersModel::instance()->setDive(current_dive);
	CylindersModel::instance()->update();
}
