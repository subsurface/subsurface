#ifndef STARWIDGET_H
#define STARWIDGET_H

#include <QWidget>


class StarWidget : public QWidget
{
	Q_OBJECT
public:
	explicit StarWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);

	int maxStars() const;
	int currentStars() const;
	bool halfStarsEnabled() const;

	/*reimp*/ QSize sizeHint() const;

	enum {SPACING = 2, IMG_SIZE = 16};

Q_SIGNALS:
	void valueChanged(int stars);

public Q_SLOTS:
	void setCurrentStars(int value);
	void setMaximumStars(int maximum);
	void enableHalfStars(bool enabled);

protected:
	/*reimp*/ void mouseReleaseEvent(QMouseEvent* );
	/*reimp*/ void paintEvent(QPaintEvent* );

private:
	int stars;
	int current;
	bool halfStar;
	QPixmap activeStar;
	QPixmap inactiveStar;
	QPixmap grayImage(QPixmap *coloredImg);
};

#endif // STARWIDGET_H
