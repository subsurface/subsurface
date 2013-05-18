#ifndef STARWIDGET_H
#define STARWIDGET_H

#include <QWidget>

enum StarConfig {SPACING = 2, IMG_SIZE = 16, TOTALSTARS = 5};

class StarWidget : public QWidget
{
	Q_OBJECT
public:
	explicit StarWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
	int currentStars() const;

	/*reimp*/ QSize sizeHint() const;

	static QPixmap starActive();
	static QPixmap starInactive();

Q_SIGNALS:
	void valueChanged(int stars);

public Q_SLOTS:
	void setCurrentStars(int value);

protected:
	/*reimp*/ void mouseReleaseEvent(QMouseEvent* );
	/*reimp*/ void paintEvent(QPaintEvent* );

private:
	int current;
	static QPixmap* activeStar;
	static QPixmap* inactiveStar;
	QPixmap grayImage(QPixmap *coloredImg);
};

#endif // STARWIDGET_H
