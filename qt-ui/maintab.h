#ifndef MAINTAB_H
#define MAINTAB_H

#include <QTabWidget>

namespace Ui
{
	class MainTab;
}

class MainTab : public QTabWidget
{
	Q_OBJECT
public:
	MainTab(QWidget *parent);
private:
	Ui::MainTab *ui;
};

#endif