// SPDX-License-Identifier: GPL-2.0
#include "ruleritem.h"
#include "profilescene.h"
#include "profileview.h"
#include "zvalues.h"
#include "core/settings/qPrefTechnicalDetails.h"

#include "core/profile.h"

#include <QApplication>

static QColor handleBorderColor(Qt::red);
static QColor handleColor(0xff, 0, 0, 0x80);
static constexpr double handleRadius = 8.0;

static QColor lineColor(Qt::black);
static constexpr double lineWidth = 1.0;

static constexpr int tooltipBorder = 1;
static constexpr double tooltipBorderRadius = 2.0; // Radius of rounded corners
static QColor tooltipBorderColor(Qt::black);
static QColor tooltipColor(0xff, 0xff, 0xff, 190);
static QColor tooltipFontColor(Qt::black);

class RulerItemHandle : public ChartDiskItem
{
public:
	ProfileView &profileView;
	double xpos;
	// The first argument is passed twice, to avoid an downcast. Yes, that's silly.
	RulerItemHandle(ChartView &view, ProfileView &profileView, double dpr) :
		ChartDiskItem(view, ProfileZValue::RulerItem,
			      QPen(handleBorderColor, dpr),
			      QBrush(handleColor),
			      true),
		profileView(profileView),
		xpos(0.0)
	{
	}
	// The call chain here is weird: this calls into the ProfileScene, which then calls
	// back into the RulerItem. The reason is that the ProfileScene knows the current
	// dive etc. This seems more robust than storing the current dive in the subobject.
	void drag(QPointF pos) override
	{
		xpos = pos.x();
		profileView.rulerDragged();
	}
};

// duplicate code in tooltipitem.cpp
static QFont makeFont(double dpr)
{
	QFont font(qApp->font());
	if (dpr != 1.0) {
		int pixelSize = font.pixelSize();
		if (pixelSize > 0) {
			pixelSize = lrint(static_cast<double>(pixelSize) * dpr);
			font.setPixelSize(pixelSize);
		} else {
			font.setPointSizeF(font.pointSizeF() * dpr);
		}
	}
	return font;
}

static ChartItemPtr<RulerItemHandle> makeHandle(ProfileView &view, double dpr)
{
	auto res = view.createChartItem<RulerItemHandle>(view, dpr);
	res->resize(handleRadius * dpr);
	return res;
}

RulerItem::RulerItem(ProfileView &view, double dpr) :
	line(view.createChartItem<ChartLineItem>(ProfileZValue::RulerItem,
						   lineColor, lineWidth * dpr)),
	handle1(makeHandle(view, dpr)),
	handle2(makeHandle(view, dpr)),
	tooltip(view.createChartItem<AnimatedChartRectItem>(ProfileZValue::RulerItem,
			      QPen(tooltipBorderColor, lrint(tooltipBorder * dpr)),
			      QBrush(tooltipColor), tooltipBorderRadius * dpr,
			      false)),
	font(makeFont(dpr)),
	fm(font),
	fontHeight(fm.height())
{
}

void RulerItem::setVisible(bool visible)
{
	line->setVisible(visible);
	handle1->setVisible(visible);
	handle2->setVisible(visible);
	tooltip->setVisible(visible);
}

// Binary search to find index at time stamp
static int get_idx_at_time(const plot_info &info, int time)
{
	auto entry = std::lower_bound(info.entry, info.entry + info.nr, time,
				      [](const plot_data &d, int time)
				      { return d.sec < time; });
	return entry < info.entry + info.nr ? entry - info.entry
					    : info.nr - 1;
}

void RulerItem::update(const dive *d, double dpr, const ProfileScene &scene, const plot_info &info, int animspeed)
{
	if (info.nr == 0)
		return; // Nothing to display

	auto [minX, maxX] = scene.minMaxX();
	auto [minY, maxY] = scene.minMaxY();
	double x1 = std::clamp(handle1->xpos, minX, maxX);
	double x2 = std::clamp(handle2->xpos, minX, maxX);

	int time1 = lrint(scene.timeAt(QPointF(x1, 0.0)));
	int time2 = lrint(scene.timeAt(QPointF(x2, 0.0)));

	int idx1 = get_idx_at_time(info, time1);
	int idx2 = get_idx_at_time(info, time2);

	double y1 = scene.posAtDepth(info.entry[idx1].depth);
	double y2 = scene.posAtDepth(info.entry[idx2].depth);

	QPointF pos1(x1, y1);
	QPointF pos2(x2, y2);
	line->setLine(pos1, pos2);
	handle1->setPos(pos1);
	handle2->setPos(pos2);

	if (idx1 == idx2) {
		tooltip->setVisible(false);
		return;
	}

	auto strings = compare_samples(d, &info, idx1, idx2, true);
	if (strings.empty()) {
		tooltip->setVisible(false);
		return;
	}
	tooltip->setVisible(true);

	double width = 0;
	std::vector<int> string_widths;
	string_widths.reserve(strings.size());
	for (const QString &s: strings) {
		int w = fm.size(Qt::TextSingleLine, s).width();
		width = std::max(width, static_cast<double>(w));
		string_widths.push_back(w);
	}

	width += 6.0 * tooltipBorder * dpr;

	double height = static_cast<double>(strings.size()) * fontHeight +
			4.0 * tooltipBorder * dpr;

	QPixmap pixmap(lrint(width), lrint(height));
	pixmap.fill(Qt::transparent);
	QPainter painter(&pixmap);

	painter.setFont(font);
	painter.setPen(QPen(tooltipFontColor)); // QPainter uses QPen to set text color!
	double x = 4.0 * tooltipBorder * dpr;
	double y = 2.0 * tooltipBorder * dpr;
	for (size_t i = 0; i < strings.size(); ++i) {
		QRectF rect(x, y, string_widths[i], fontHeight);
		painter.drawText(rect, strings[i]);
		y += fontHeight;
	}

	tooltip->setPixmap(pixmap, animspeed);

	if (pos2.x() < pos1.x())
		std::swap(pos1, pos2);
	double xpos = width < maxX - minX ?
		std::clamp(pos1.x() + handleRadius * dpr, 0.0, maxX - width) : 0.0;

	double ypos = height < maxY - minY ?
		std::clamp(pos1.y() + handleRadius * dpr, 0.0, maxY - height) : 0.0;

	tooltip->setPos(QPointF(xpos, ypos));
}

void RulerItem::anim(double progress)
{
	tooltip->anim(progress);
}
