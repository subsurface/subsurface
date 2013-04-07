#include "maintab.h"
#include "ui_maintab.h"

MainTab::MainTab(QWidget *parent) : QTabWidget(parent),
				    ui(new Ui::MainTab())
{
	ui->setupUi(this);
}

#include "maintab.moc"
