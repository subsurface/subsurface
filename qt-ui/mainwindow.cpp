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
	qDebug("actionNew");
}

void MainWindow::on_actionOpen_triggered()
{
	qDebug("actionOpen");
}

void MainWindow::on_actionSave_triggered()
{
	qDebug("actionSave");
}

void MainWindow::on_actionSaveAs_triggered()
{
	qDebug("actionSaveAs");
}
void MainWindow::on_actionClose_triggered()
{
	qDebug("actionClose");
}

void MainWindow::on_actionImport_triggered()
{
	qDebug("actionImport");
}

void MainWindow::on_actionExportUDDF_triggered()
{
	qDebug("actionExportUDDF");
}

void MainWindow::on_actionPrint_triggered()
{
	qDebug("actionPrint");
}

void MainWindow::on_actionPreferences_triggered()
{
	qDebug("actionPreferences");
}

void MainWindow::on_actionQuit_triggered()
{
	qDebug("actionQuit");
}

void MainWindow::on_actionDownloadDC_triggered()
{
	qDebug("actionDownloadDC");
}

void MainWindow::on_actionDownloadWeb_triggered()
{
	qDebug("actionDownloadWeb");}

void MainWindow::on_actionEditDeviceNames_triggered()
{
	qDebug("actionEditDeviceNames");}

void MainWindow::on_actionAddDive_triggered()
{
	qDebug("actionAddDive");
}

void MainWindow::on_actionRenumber_triggered()
{
	qDebug("actionRenumber");
}

void MainWindow::on_actionAutoGroup_triggered()
{
	qDebug("actionAutoGroup");
}

void MainWindow::on_actionToggleZoom_triggered()
{
	qDebug("actionToggleZoom");
}

void MainWindow::on_actionYearlyStatistics_triggered()
{
	qDebug("actionYearlyStatistics");
}

void MainWindow::on_actionViewList_triggered()
{
	qDebug("actionViewList");

	ui->InfoWidget->setVisible(false);
	ui->ListWidget->setVisible(true);
	ui->ProfileWidget->setVisible(false);
}

void MainWindow::on_actionViewProfile_triggered()
{
	qDebug("actionViewProfile");

	ui->InfoWidget->setVisible(false);
	ui->ListWidget->setVisible(false);
	ui->ProfileWidget->setVisible(true);
}

void MainWindow::on_actionViewInfo_triggered()
{
	qDebug("actionViewInfo");

	ui->InfoWidget->setVisible(true);
	ui->ListWidget->setVisible(false);
	ui->ProfileWidget->setVisible(false);
}

void MainWindow::on_actionViewAll_triggered()
{
	qDebug("actionViewAll");

	ui->InfoWidget->setVisible(true);
	ui->ListWidget->setVisible(true);
	ui->ProfileWidget->setVisible(true);
}

void MainWindow::on_actionPreviousDC_triggered()
{
	qDebug("actionPreviousDC");
}

void MainWindow::on_actionNextDC_triggered()
{
	qDebug("actionNextDC");
}

void MainWindow::on_actionSelectEvents_triggered()
{
	qDebug("actionSelectEvents");
}

void MainWindow::on_actionInputPlan_triggered()
{
	qDebug("actionInputPlan");
}

void MainWindow::on_actionAboutSubsurface_triggered()
{
	qDebug("actionAboutSubsurface");
}

void MainWindow::on_actionUserManual_triggered()
{
	qDebug("actionUserManual");
}



#include "mainwindow.moc"
