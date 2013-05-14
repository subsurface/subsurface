/*
 * mainwindow.h
 *
 * header file for the main window of Subsurface
 *
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>
#include <QAction>

class QSortFilterProxyModel;
class DiveTripModel;

namespace Ui
{
	class MainWindow;
}

class DiveInfo;
class DiveNotes;
class Stats;
class Equipment;
class QItemSelection;

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
	MainWindow();

private Q_SLOTS:

	/* file menu action */
	void on_actionNew_triggered();
	void on_actionOpen_triggered();
	void on_actionSave_triggered();
	void on_actionSaveAs_triggered();
	void on_actionClose_triggered();
	void on_actionImport_triggered();
	void on_actionExportUDDF_triggered();
	void on_actionPrint_triggered();
	void on_actionPreferences_triggered();
	void on_actionQuit_triggered();

	/* log menu actions */
	void on_actionDownloadDC_triggered();
	void on_actionDownloadWeb_triggered();
	void on_actionEditDeviceNames_triggered();
	void on_actionAddDive_triggered();
	void on_actionRenumber_triggered();
	void on_actionAutoGroup_triggered();
	void on_actionToggleZoom_triggered();
	void on_actionYearlyStatistics_triggered();

	/* view menu actions */
	void on_actionViewList_triggered();
	void on_actionViewProfile_triggered();
	void on_actionViewInfo_triggered();
	void on_actionViewAll_triggered();
	void on_actionPreviousDC_triggered();
	void on_actionNextDC_triggered();

	/* other menu actions */
	void on_actionSelectEvents_triggered();
	void on_actionInputPlan_triggered();
	void on_actionAboutSubsurface_triggered();
	void on_actionUserManual_triggered();

	void current_dive_changed(int divenr);

protected:
	void closeEvent(QCloseEvent *);

private:
	Ui::MainWindow *ui;
	DiveTripModel *model;
	QSortFilterProxyModel *sortModel;
	QAction *actionNextDive;
	QAction *actionPreviousDive;

	QString filter();
	bool askSaveChanges();
	void readSettings();
	void writeSettings();
	void redrawProfile();
};

#endif
