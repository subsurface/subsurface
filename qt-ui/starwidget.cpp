#include "starwidget.h"
#include <QSvgRenderer>
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <QMouseEvent>
#include <unistd.h>
#include <QStyle>
#include <QStyleOption>

QPixmap *StarWidget::activeStar = 0;
QPixmap *StarWidget::inactiveStar = 0;

QPixmap StarWidget::starActive()
{
	return (*activeStar);
}

QPixmap StarWidget::starInactive()
{
	return (*inactiveStar);
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

	int starClicked = event->pos().x() / IMG_SIZE + 1;
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

	for (int i = 0; i < current; i++)
		p.drawPixmap(i * IMG_SIZE + SPACING, 0, starActive());

	for (int i = current; i < TOTALSTARS; i++)
		p.drawPixmap(i * IMG_SIZE + SPACING, 0, starInactive());

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
	if (!activeStar) {
		activeStar = new QPixmap();
		QSvgRenderer render(QString(":star"));
		QPixmap renderedStar(IMG_SIZE, IMG_SIZE);

		renderedStar.fill(Qt::transparent);
		QPainter painter(&renderedStar);

		render.render(&painter, QRectF(0, 0, IMG_SIZE, IMG_SIZE));
		(*activeStar) = renderedStar;
	}
	if (!inactiveStar) {
		inactiveStar = new QPixmap();
		(*inactiveStar) = grayImage(activeStar);
	}
	setFocusPolicy(Qt::StrongFocus);
}

QPixmap StarWidget::grayImage(QPixmap *coloredImg)
{
	QImage img = coloredImg->toImage();
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

	return QPixmap::fromImage(img);
}

QSize StarWidget::sizeHint() const
{
	return QSize(IMG_SIZE * TOTALSTARS + SPACING * (TOTALSTARS - 1), IMG_SIZE);
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
