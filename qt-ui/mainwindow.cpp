#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVBoxLayout>
#include <QtDebug>

MainWindow::MainWindow() : ui(new Ui::MainWindow())
{
	ui->setupUi(this);
}

void MainWindow::on_actionNew_triggered()
{
    qDebug() << "actionNew";
}
