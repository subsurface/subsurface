// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/starwidget.h"
#include "core/metrics.h"
#include <QSvgRenderer>
#include <QMouseEvent>
#include "desktop-widgets/simplewidgets.h"

QImage StarWidget::activeStar;
QImage StarWidget::inactiveStar;

const QImage& StarWidget::starActive()
{
	return activeStar;
}

const QImage& StarWidget::starInactive()
{
	return inactiveStar;
}

int StarWidget::currentStars() const
{
	return current;
}

void StarWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (readOnly) {
		return;
	}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	int starClicked = event->position().toPoint().x() / defaultIconMetrics().sz_small + 1;
#else
	int starClicked = event->pos().x() / defaultIconMetrics().sz_small + 1;
#endif
	if (starClicked > TOTALSTARS)
		starClicked = TOTALSTARS;

	if (current == starClicked)
		current -= 1;
	else
		current = starClicked;

	emit valueChanged(current);
	update();
}

void StarWidget::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	QPixmap selected = QPixmap::fromImage(starActive());
	QPixmap inactive = QPixmap::fromImage(starInactive());
	const IconMetrics& metrics = defaultIconMetrics();
	int dx = (metrics.sz_small - inactive.width()) / 2;
	int dy = (metrics.sz_small - inactive.height()) / 2;

	for (int i = 0; i < current; i++)
		p.drawPixmap(i * metrics.sz_small + metrics.spacing, 0, selected);

	for (int i = current; i < TOTALSTARS; i++)
		p.drawPixmap(i * metrics.sz_small + metrics.spacing + dx, dy, inactive);

	if (hasFocus()) {
		QStyleOptionFocusRect option;
		option.initFrom(this);
		option.backgroundColor = palette().color(QPalette::Window);
		style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &p, this);
	}
}

void StarWidget::setCurrentStars(int value)
{
	current = value;
	update();
	emit valueChanged(current);
}

static QImage grayImage(const QImage &coloredImg)
{
	QImage img = coloredImg;
	for (int i = 0; i < img.width(); ++i) {
		for (int j = 0; j < img.height(); ++j) {
			QRgb rgb = img.pixel(i, j);
			if (!rgb)
				continue;

			int gray = qGray(rgb) * 2 / 5 + 40;
			img.setPixel(i, j, qRgb(gray, gray, gray));
		}
	}

	return img;
}

StarWidget::StarWidget(QWidget *parent) : QWidget(parent, QFlag(0)),
	current(0),
	readOnly(false)
{
	int dim = defaultIconMetrics().sz_small;

	if (activeStar.isNull()) {
		QSvgRenderer render(QString(":star-icon"));
		QPixmap renderedStar(dim, dim);

		renderedStar.fill(Qt::transparent);
		QPainter painter(&renderedStar);

		render.render(&painter, QRectF(0, 0, dim, dim));
		activeStar = renderedStar.toImage();
	}
	if (inactiveStar.isNull())
		inactiveStar = grayImage(activeStar.scaled(dim / 2, dim / 2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	setFocusPolicy(Qt::StrongFocus);
}

QSize StarWidget::sizeHint() const
{
	const IconMetrics& metrics = defaultIconMetrics();
	return QSize(metrics.sz_small * TOTALSTARS + metrics.spacing * (TOTALSTARS - 1), metrics.sz_small);
}

void StarWidget::setReadOnly(bool r)
{
	readOnly = r;
}

void StarWidget::focusInEvent(QFocusEvent *event)
{
	setFocus();
	QWidget::focusInEvent(event);
}

void StarWidget::focusOutEvent(QFocusEvent *event)
{
	QWidget::focusOutEvent(event);
}

void StarWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Right) {
		if (currentStars() < TOTALSTARS)
			setCurrentStars(currentStars() + 1);
	} else if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Left) {
		if (currentStars() > 0)
			setCurrentStars(currentStars() - 1);
	}
}
