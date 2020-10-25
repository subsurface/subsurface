// SPDX-License-Identifier: GPL-2.0
#ifndef STARWIDGET_H
#define STARWIDGET_H

#include <QWidget>

enum StarConfig {
	TOTALSTARS = 5
};

class StarWidget : public QWidget {
	Q_OBJECT
public:
	explicit StarWidget(QWidget *parent = 0);
	int currentStars() const;

	QSize sizeHint() const override;

	static const QImage& starActive();
	static const QImage& starInactive();

signals:
	void valueChanged(int stars);

public
slots:
	void setCurrentStars(int value);
	void setReadOnly(bool readOnly);

protected:
	void mouseReleaseEvent(QMouseEvent *) override;
	void paintEvent(QPaintEvent *) override;
	void focusInEvent(QFocusEvent *) override;
	void focusOutEvent(QFocusEvent *) override;
	void keyPressEvent(QKeyEvent *) override;

private:
	int current;
	bool readOnly;

	static QImage activeStar;
	static QImage inactiveStar;
};

#endif // STARWIDGET_H
