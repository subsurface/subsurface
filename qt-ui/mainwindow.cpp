#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVBoxLayout>

MainWindow::MainWindow() : ui(new Ui::MainWindow())
{
	ui->setupUi(this);
}

#include "mainwindow.moc"
