#ifndef FILTERWIDGET_2_H
#define FILTERWIDGET_2_H

#include <QWidget>

#include <memory>

#include "ui_filterwidget2.h"

namespace Ui {
	class FilterWidget2;
}

class FilterWidget2 : public QWidget {
	Q_OBJECT
public:
	explicit FilterWidget2(QWidget *parent = 0);
	void updateFilter();

signals:
	void filterUpdated();

private:
	std::unique_ptr<Ui::FilterWidget2> ui;
};

#endif
