// SPDX-License-Identifier: GPL-2.0
#ifndef TAB_BASE_H
#define TAB_BASE_H

#include <QWidget>

struct dive;

class TabBase : public QWidget {
	Q_OBJECT

public:
	using QWidget::QWidget;
	virtual void updateData(const std::vector<dive *> &selection, dive *currentDive, int currentDC) = 0;
	virtual void clear() = 0;
	virtual void updateUi(QString titleColor);
};

#endif
