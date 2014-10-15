#include "starwidget.h"
#include <QSvgRenderer>
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <QMouseEvent>
#include <unistd.h>
#include <QStyle>
#include <QStyleOption>
#include "simplewidgets.h"

QImage StarWidget::activeStar;
QImage StarWidget::inactiveStar;
StarMetrics StarWidget::imgMetrics = { -1, -1 };

const QImage& StarWidget::starActive()
{
	return activeStar;
}

const QImage& StarWidget::starInactive()
{
	return inactiveStar;
}

const StarMetrics& StarWidget::metrics()
{
	return imgMetrics;
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

	int starClicked = event->pos().x() / imgMetrics.size + 1;
	if (starClicked > TOTALSTARS)
		starClicked = TOTALSTARS;

	if (current == starClicked)
		current -= 1;
	else
		current = starClicked;

	Q_EMIT valueChanged(current);
	update();
}

void StarWidget::paintEvent(QPaintEvent *event)
{
	QPainter p(this);
	QPixmap active = QPixmap::fromImage(starActive());
	QPixmap inactive = QPixmap::fromImage(starInactive());

	for (int i = 0; i < current; i++)
		p.drawPixmap(i * imgMetrics.size + imgMetrics.spacing, 0, active);

	for (int i = current; i < TOTALSTARS; i++)
		p.drawPixmap(i * imgMetrics.size + imgMetrics.spacing, 0, inactive);

	if (hasFocus()) {
		QStyleOptionFocusRect option;
		option.initFrom(this);
		option.backgroundColor = palette().color(QPalette::Background);
		style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &p, this);
	}
}

void StarWidget::setCurrentStars(int value)
{
	current = value;
	update();
	Q_EMIT valueChanged(current);
}

StarWidget::StarWidget(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f),
	current(0),
	readOnly(false)
{
	// compute image size, by rounding the font height to the nearest multiple of 16
	if (imgMetrics.size == -1) {
		int height = QFontMetrics(parent->font()).height();
		imgMetrics.size = (height + 8)/16;
		imgMetrics.size *= 16;
		// enforce a minimum size
		if (imgMetrics.size < 16)
			imgMetrics.size = 16;
		imgMetrics.spacing = imgMetrics.size/8;
	}

	if (activeStar.isNull()) {
		QSvgRenderer render(QString(":star"));
		QPixmap renderedStar(imgMetrics.size, imgMetrics.size);

		renderedStar.fill(Qt::transparent);
		QPainter painter(&renderedStar);

		render.render(&painter, QRectF(0, 0, imgMetrics.size, imgMetrics.size));
		activeStar = renderedStar.toImage();
	}
	if (inactiveStar.isNull()) {
		inactiveStar = grayImage(activeStar);
	}
	setFocusPolicy(Qt::StrongFocus);
}

QImage grayImage(const QImage& coloredImg)
{
	QImage img = coloredImg;
	for (int i = 0; i < img.width(); ++i) {
		for (int j = 0; j < img.height(); ++j) {
			QRgb rgb = img.pixel(i, j);
			if (!rgb)
				continue;

			QColor c(rgb);
			int gray = (c.red() + c.green() + c.blue()) / 3;
			img.setPixel(i, j, qRgb(gray, gray, gray));
		}
	}

	return img;
}

QSize StarWidget::sizeHint() const
{
	return QSize(imgMetrics.size * TOTALSTARS + imgMetrics.spacing * (TOTALSTARS - 1), imgMetrics.size);
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
		if (currentStars() < TOTALSTARS) {
			setCurrentStars(currentStars() + 1);
		}
	} else if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Left) {
		if (currentStars() > 0) {
			setCurrentStars(currentStars() - 1);
		}
	}
}
