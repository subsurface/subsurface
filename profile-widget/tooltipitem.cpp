// SPDX-License-Identifier: GPL-2.0
#include "tooltipitem.h"
#include "profiletranslations.h"
#include "zvalues.h"

#include "core/color.h"
#include "core/membuffer.h"
#include "core/profile.h"
#include "core/qthelper.h" // for decoMode
#include "core/settings/qPrefDisplay.h"

#include <cmath>
#include <QApplication>
#include <QFontMetrics>

static const int tooltipBorder = 2;
static const double tooltipBorderRadius = 4.0; // Radius of rounded corners

static QColor tooltipBorderColor(Qt::white);
static QColor tooltipColor(0, 0, 0, 155);
static QColor tooltipFontColor(Qt::white);

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

ToolTipItem::ToolTipItem(ChartView &view, double dpr) :
	ChartRectItem(view, ProfileZValue::ToolTipItem,
		      QPen(tooltipBorderColor, tooltipBorder),
		      QBrush(tooltipColor), tooltipBorderRadius,
		      true),
	font(makeFont(dpr)),
	fm(font),
	fontHeight(fm.height())
{
	title = stringToPixmap(ProfileTranslations::tr("Information"));

	QPointF pos = qPrefDisplay::tooltip_position();
	setPos(pos);
}

QPixmap ToolTipItem::stringToPixmap(const QString &str) const
{
	QSize s = fm.size(Qt::TextSingleLine, str);
	if (s.width() <= 0 || s.height() <= 0)
		return QPixmap(1,1);
	QPixmap res(s);
	res.fill(Qt::transparent);
	QPainter painter(&res);
	painter.setFont(font);
	painter.setPen(tooltipFontColor);
	painter.drawText(QRect(QPoint(), s), str);
	return res;
}

// Split a membuffer into strings, skip empty lines.
// Return string / width (in pixels) pairs.
static std::vector<std::pair<QString, int>> split_mb_into_strings(const membuffer &mb, const QFontMetrics &fm)
{
	std::vector<std::pair<QString, int>> res;
	for (size_t i = 0; i < mb.len; ++i) {
		size_t j;
		for (j = i; j < mb.len && mb.buffer[j] != '\n'; ++j)
			;
		if (j > i) {
			QString s = QString::fromUtf8(mb.buffer + i, j - i);
			int width = fm.size(Qt::TextSingleLine, s).width();
			res.emplace_back(s, width);
		}
		i = j; // Note: loop iteration will skip over '\n'
	}
	return res;
}

static QPixmap drawTissues(const plot_info &pInfo, double dpr, int idx, bool inPlanner)
{
	QPixmap tissues(16,60);
	QPainter painter(&tissues);
	tissues.fill();
	painter.setPen(QColor(0, 0, 0, 0));
	painter.setBrush(QColor(LIMENADE1));
	painter.drawRect(0, 10 + (100 - AMB_PERCENTAGE) / 2, 16, AMB_PERCENTAGE / 2);
	painter.setBrush(QColor(SPRINGWOOD1));
	painter.drawRect(0, 10, 16, (100 - AMB_PERCENTAGE) / 2);
	painter.setBrush(QColor(Qt::red));
	painter.drawRect(0,0,16,10);

	if (!idx)
		return tissues;

	const struct plot_data *entry = &pInfo.entry[idx];
	painter.setPen(QColor(0, 0, 0, 255));
	if (decoMode(inPlanner) == BUEHLMANN)
		painter.drawLine(0, lrint(60 - entry->gfline / 2), 16, lrint(60 - entry->gfline / 2));
	painter.drawLine(0, lrint(60 - AMB_PERCENTAGE * (entry->pressures.n2 + entry->pressures.he) / entry->ambpressure / 2),
			16, lrint(60 - AMB_PERCENTAGE * (entry->pressures.n2 + entry->pressures.he) / entry->ambpressure /2));
	painter.setPen(QColor(0, 0, 0, 127));
	for (int i = 0; i < 16; i++)
		painter.drawLine(i, 60, i, 60 - entry->percentages[i] / 2);

	if (dpr == 1.0)
		return tissues;

	// Scale according to DPR
	int new_width = lrint(tissues.width() * dpr);
	int new_height = lrint(tissues.width() * dpr);
	return tissues.scaled(new_width, new_height);
}

void ToolTipItem::update(const dive *d, double dpr, int time, const plot_info &pInfo,
			 const std::vector<std::pair<QString, QPixmap>> &events, bool inPlanner)
{
	struct membufferpp mb;

	int idx = get_plot_details_new(d, &pInfo, time, &mb);

	QPixmap tissues = drawTissues(pInfo, dpr, idx, inPlanner);
	std::vector<std::pair<QString, int>> strings = split_mb_into_strings(mb, fm);
	std::vector<QSizeF> event_sizes;

	width = title.size().width();
	for (auto &[s,w]: strings)
		width = std::max(width, static_cast<double>(w));

	height = (static_cast<double>(strings.size()) + 1.0) * fontHeight;

	for (auto &[s, pixmap]: events) {
		double text_width = fm.size(Qt::TextSingleLine, s).width();
		double total_width = pixmap.width() + text_width;
		double h = std::max(static_cast<double>(pixmap.height()), fontHeight);
		width = std::max(width, total_width);
		height += h;
		event_sizes.emplace_back(text_width, h);
	}

	width += tissues.width();
	width += 6.0 * tooltipBorder;
	height = std::max(height, static_cast<double>(tissues.height()));
	height += 4.0 * tooltipBorder + title.height();

	ChartRectItem::resize(QSizeF(width, height));
	painter->setFont(font);
	painter->setPen(QPen(tooltipFontColor)); // QPainter uses QPen to set text color!
	double x = 4.0 * tooltipBorder + tissues.width();
	double y = 2.0 * tooltipBorder;
	double titleOffset = (width - title.width()) / 2.0;
	painter->drawPixmap(lrint(titleOffset), lrint(y), title, 0, 0, title.width(), title.height());
	y += round(fontHeight);
	painter->drawPixmap(lrint(2.0 * tooltipBorder), lrint(y), tissues, 0, 0, tissues.width(), tissues.height());
	y += round(fontHeight);
	for (auto &[s,w]: strings) {
		QRectF rect(x, y, w, fontHeight);
		painter->drawText(rect, s);
		y += fontHeight;
	}
	for (size_t i = 0; i < events.size(); ++i) {
		QSizeF size = event_sizes[i];
		auto &[s, pixmap] = events[i];
		painter->drawPixmap(lrint(x), lrint(y + (size.height() - pixmap.height())/2.0), pixmap,
				    0, 0, pixmap.width(), pixmap.height());
		QRectF rect(x + pixmap.width(), round(y + (size.height() - fontHeight) / 2.0), size.width(), fontHeight);
		painter->drawText(rect, s);
		y += size.height();
	}
	setTextureDirty();
}

void ToolTipItem::stopDrag(QPointF pos)
{
	qPrefDisplay::set_tooltip_position(pos);
}
