#include "starwidget.h"
#include <QSvgRenderer>
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <QMouseEvent>

int StarWidget::currentStars() const
{
	return current;
}

void StarWidget::enableHalfStars(bool enabled)
{
	halfStar = enabled;
	update();
}

bool StarWidget::halfStarsEnabled() const
{
	return halfStar;
}

int StarWidget::maxStars() const
{
	return stars;
}

void StarWidget::mouseReleaseEvent(QMouseEvent* event)
{
	int starClicked = event->pos().x() / IMG_SIZE + 1;
	if (starClicked > stars)
		starClicked = stars;

	if (current == starClicked)
		current -= 1;
	else
		current = starClicked;

	update();
}

void StarWidget::paintEvent(QPaintEvent* event)
{
	QPainter p(this);

	for(int i = 0; i < current; i++)
		p.drawPixmap(i * IMG_SIZE + SPACING, 0, activeStar);

	for(int i = current; i < stars; i++)
		p.drawPixmap(i * IMG_SIZE + SPACING, 0, inactiveStar);
}

void StarWidget::setCurrentStars(int value)
{
	current = value;
	update();
	Q_EMIT valueChanged(current);
}

void StarWidget::setMaximumStars(int maximum)
{
	stars = maximum;
	update();
}

StarWidget::StarWidget(QWidget* parent, Qt::WindowFlags f):
	QWidget(parent, f),
	stars(5),
	current(0),
	halfStar(false)
{
	QSvgRenderer render(QString("star.svg"));
	QPixmap renderedStar(IMG_SIZE, IMG_SIZE);

	renderedStar.fill(Qt::transparent);
	QPainter painter(&renderedStar);

	render.render(&painter, QRectF(0, 0, IMG_SIZE, IMG_SIZE));
	activeStar = renderedStar;
	inactiveStar = grayImage(&renderedStar);
}

QPixmap StarWidget::grayImage(QPixmap* coloredImg)
{
	QImage img = coloredImg->toImage();
	for (int i = 0; i < img.width(); ++i) {
		for (int j = 0; j < img.height(); ++j) {
			QRgb col = img.pixel(i, j);
			if (!col)
				continue;
			int gray = QColor(Qt::darkGray).rgb();
			img.setPixel(i, j, qRgb(gray, gray, gray));
		}
	}
	return QPixmap::fromImage(img);
}

QSize StarWidget::sizeHint() const
{
	return QSize(IMG_SIZE * stars + SPACING * (stars-1), IMG_SIZE);
}
