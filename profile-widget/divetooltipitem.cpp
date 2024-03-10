// SPDX-License-Identifier: GPL-2.0
#include "profile-widget/divetooltipitem.h"
#include "profile-widget/divecartesianaxis.h"
#include "core/membuffer.h"
#include "core/metrics.h"
#include "core/settings/qPrefDisplay.h"
#include "core/settings/qPrefTechnicalDetails.h"
#include <QPropertyAnimation>
#include <QGraphicsView>
#include "core/qthelper.h"

void ToolTipItem::addToolTip(const QString &toolTip, const QPixmap &pixmap)
{
	const IconMetrics &iconMetrics = defaultIconMetrics();

	QGraphicsPixmapItem *iconItem = 0;
	double yValue = title->boundingRect().height() + iconMetrics.spacing;
	for (ToolTip t: toolTips)
		yValue += t.second->boundingRect().height();
	if (entryToolTip.second)
		yValue += entryToolTip.second->boundingRect().height();
	iconItem = new QGraphicsPixmapItem(this);
	if (!pixmap.isNull())
		iconItem->setPixmap(pixmap);
	const int sp2 = iconMetrics.spacing * 2;
	iconItem->setPos(sp2, yValue);

	QGraphicsTextItem *textItem = new QGraphicsTextItem(this);
	textItem->setHtml(toolTip);
	textItem->setPos(sp2 + iconMetrics.sz_small + sp2, yValue);
	textItem->setDefaultTextColor(Qt::white);
	textItem->setFlag(ItemIgnoresTransformations);
	toolTips.push_back(qMakePair(iconItem, textItem));
}

void ToolTipItem::clear()
{
	for (ToolTip t: toolTips) {
		delete t.first;
		delete t.second;
	}
	toolTips.clear();
}

void ToolTipItem::setRect(const QRectF &r)
{
	if (r == rect())
		return;

	QGraphicsRectItem::setRect(r);
	updateTitlePosition();
}

void ToolTipItem::collapse()
{
	int dim = defaultIconMetrics().sz_small;

	if (qPrefDisplay::animation_speed()) {
		QPropertyAnimation *animation = new QPropertyAnimation(this, "rect");
		animation->setDuration(100);
		animation->setStartValue(nextRectangle);
		animation->setEndValue(QRect(0, 0, dim, dim));
		animation->start(QAbstractAnimation::DeleteWhenStopped);
	} else {
		setRect(nextRectangle);
	}
	clear();

	status = COLLAPSED;
}

void ToolTipItem::expand()
{
	if (!title)
		return;

	const IconMetrics &iconMetrics = defaultIconMetrics();

	double width = 0, height = title->boundingRect().height() + iconMetrics.spacing;
	for (const ToolTip &t: toolTips) {
		QRectF sRect = t.second->boundingRect();
		if (sRect.width() > width)
			width = sRect.width();
		height += sRect.height();
	}

	if (entryToolTip.first) {
		QRectF sRect = entryToolTip.second->boundingRect();
		if (sRect.width() > width)
			width = sRect.width();
		height += sRect.height();
	}

	const int sp2 = iconMetrics.spacing * 2;
	// pixmap left padding, icon, pixmap right padding, right padding
	width += sp2 + iconMetrics.sz_small + sp2 + sp2 * 2;
	// bottom padding
	height += sp2;

	// clip the tooltip width
	if (width < title->boundingRect().width() + sp2)
		width = title->boundingRect().width() + sp2;
	// clip the height
	if (entryToolTip.first) {
		const int minH = lrint(entryToolTip.first->y() + entryToolTip.first->pixmap().height() + sp2);
		if (height < minH)
			height = minH;
	} else if (height < iconMetrics.sz_small) {
		height = iconMetrics.sz_small;
	}

	nextRectangle.setWidth(width);
	nextRectangle.setHeight(height);

	if (nextRectangle != rect()) {
		if (qPrefDisplay::animation_speed()) {
			QPropertyAnimation *animation = new QPropertyAnimation(this, "rect", this);
			animation->setDuration(prefs.animation_speed);
			animation->setStartValue(rect());
			animation->setEndValue(nextRectangle);
			animation->start(QAbstractAnimation::DeleteWhenStopped);
		} else {
			setRect(nextRectangle);
		}
	}

	status = EXPANDED;
}

ToolTipItem::ToolTipItem(QGraphicsItem *parent) : RoundRectItem(8.0, parent),
	title(new QGraphicsSimpleTextItem(tr("Information"), this)),
	status(COLLAPSED),
	tissues(16,60),
	painter(&tissues),
	timeAxis(0),
	lastTime(-1)
{
	clearPlotInfo();
	entryToolTip.first = NULL;
	entryToolTip.second = NULL;
	setFlags(ItemIgnoresTransformations | ItemIsMovable | ItemClipsChildrenToShape);

	QColor c = QColor(Qt::black);
	c.setAlpha(155);
	setBrush(c);

	setZValue(99);

	addToolTip(QString(), QPixmap(16,60));
	entryToolTip = toolTips.first();
	toolTips.clear();

	title->setFlag(ItemIgnoresTransformations);
	title->setPen(QPen(Qt::white, 1));
	title->setBrush(Qt::white);

	setPen(QPen(Qt::white, 2));

	connect(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::infoboxChanged, this, &ToolTipItem::settingsChanged);

	refreshTime.start();
}

ToolTipItem::~ToolTipItem()
{
	clear();
}

void ToolTipItem::updateTitlePosition()
{
	const IconMetrics &iconMetrics = defaultIconMetrics();
	if (rect().width() < title->boundingRect().width() + iconMetrics.spacing * 4) {
		QRectF newRect = rect();
		newRect.setWidth(title->boundingRect().width() + iconMetrics.spacing * 4);
		newRect.setHeight((newRect.height() && isExpanded()) ? newRect.height() : iconMetrics.sz_small);
		setRect(newRect);
	}

	title->setPos(rect().width() / 2 - title->boundingRect().width() / 2 - 1, 0);
}

bool ToolTipItem::isExpanded() const
{
	return status == EXPANDED;
}

void ToolTipItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	persistPos();
	QGraphicsRectItem::mouseReleaseEvent(event);
	for (QGraphicsItem *item: oldSelection)
		item->setSelected(true);
}

void ToolTipItem::persistPos() const
{
	qPrefDisplay::set_tooltip_position(pos());
}

void ToolTipItem::readPos()
{
	QPointF value = qPrefDisplay::tooltip_position();
	if (!scene()->sceneRect().contains(value))
		value = QPointF(0, 0);
	setPos(value);
}

void ToolTipItem::setPlotInfo(const plot_info &plot)
{
	pInfo = plot;
}

void ToolTipItem::clearPlotInfo()
{
	memset(&pInfo, 0, sizeof(pInfo));
}

void ToolTipItem::setTimeAxis(DiveCartesianAxis *axis)
{
	timeAxis = axis;
}

void ToolTipItem::refresh(const dive *d, const QPointF &pos, bool inPlanner)
{
	if(refreshTime.elapsed() < 40)
		return;
	refreshTime.start();

	int time = lrint(timeAxis->valueAt(pos));

	lastTime = time;
	clear();

	auto [idx, lines] = get_plot_details_new(d, &pInfo, time);

	tissues.fill();
	painter.setPen(QColor(0, 0, 0, 0));
	painter.setBrush(QColor(LIMENADE1));
	painter.drawRect(0, 10 + (100 - AMB_PERCENTAGE) / 2, 16, AMB_PERCENTAGE / 2);
	painter.setBrush(QColor(SPRINGWOOD1));
	painter.drawRect(0, 10, 16, (100 - AMB_PERCENTAGE) / 2);
	painter.setBrush(QColor(Qt::red));
	painter.drawRect(0,0,16,10);
	if (idx) {
		const struct plot_data *entry = &pInfo.entry[idx];
		painter.setPen(QColor(0, 0, 0, 255));
		if (decoMode(inPlanner) == BUEHLMANN)
			painter.drawLine(0, lrint(60 - entry->gfline / 2), 16, lrint(60 - entry->gfline / 2));
		painter.drawLine(0, lrint(60 - AMB_PERCENTAGE * (entry->pressures.n2 + entry->pressures.he) / entry->ambpressure / 2),
				16, lrint(60 - AMB_PERCENTAGE * (entry->pressures.n2 + entry->pressures.he) / entry->ambpressure /2));
		painter.setPen(QColor(0, 0, 0, 127));
		for (int i = 0; i < 16; i++)
			painter.drawLine(i, 60, i, 60 - entry->percentages[i] / 2);
		QString text;
		for (const std::string &s: lines) {
			if (!text.isEmpty())
				text += '\n';
			text += QString::fromStdString(s);
		}
		entryToolTip.second->setPlainText(text);
	}
	entryToolTip.first->setPixmap(tissues);

	const auto l = scene()->items(pos, Qt::IntersectsItemBoundingRect, Qt::DescendingOrder,
			scene()->views().first()->transform());
	for (QGraphicsItem *item: l) {
		if (!item->toolTip().isEmpty())
			addToolTip(item->toolTip(), QPixmap());
	}
	expand();
}

void ToolTipItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	oldSelection = scene()->selectedItems();
	scene()->clearSelection();
	QGraphicsItem::mousePressEvent(event);
}

void ToolTipItem::settingsChanged(bool value)
{
	setVisible(value);
}
