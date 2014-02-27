/*
 * mainwindow.h
 *
 * header file for the main window of Subsurface
 *
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAction>
#include <QUrl>

#include "ui_mainwindow.h"
#include "usermanual.h"

struct DiveList;
class QSortFilterProxyModel;
class DiveTripModel;

class DiveInfo;
class DiveNotes;
class Stats;
class Equipment;
class QItemSelection;
class DiveListView;
class GlobeGPS;
class MainTab;
class ProfileGraphicsView;
class QWebView;

enum MainWindowTitleFormat { MWTF_DEFAULT, MWTF_FILENAME };

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
	enum {COLLAPSED, EXPANDED};
	enum StackWidgetIndexes{ PROFILE, PLANNERPROFILE};
	enum InfoWidgetIndexes{ MAINTAB, PLANNERWIDGET};
	enum CurrentState{ VIEWALL, GLOBE_MAXIMIZED, INFO_MAXIMIZED, PROFILE_MAXIMIZED, LIST_MAXIMIZED};

	MainWindow();
	virtual ~MainWindow();
	static MainWindow *instance();
	ProfileGraphicsView *graphics();
	MainTab *information();
	void loadRecentFiles(QSettings *s);
	void addRecentFile(const QStringList &newFiles);
	DiveListView *dive_list();
	GlobeGPS *globe();
	void showError(QString message);
	void setTitle(enum MainWindowTitleFormat format);

	// The 'Change DC Shortcuts' should only be enabled
	// when the profile's visible.
	void disableDcShortcuts();
	void enableDcShortcuts();
	void loadFiles(const QStringList files);
	void importFiles(const QStringList importFiles);
	void cleanUpEmpty();
	QTabWidget *tabWidget();
private slots:
	/* file menu action */
	void recentFileTriggered(bool checked);
	void on_actionNew_triggered();
	void on_actionOpen_triggered();
	void on_actionSave_triggered();
	void on_actionSaveAs_triggered();
	void on_actionClose_triggered();
	void on_actionExportUDDF_triggered();
	void on_actionPrint_triggered();
	void on_actionPreferences_triggered();
	void on_actionQuit_triggered();

	/* log menu actions */
	void on_actionDownloadDC_triggered();
	void on_actionDownloadWeb_triggered();
	void on_actionDivelogs_de_triggered();
	void on_actionEditDeviceNames_triggered();
	void on_actionAddDive_triggered();
	void on_actionRenumber_triggered();
	void on_actionAutoGroup_triggered();
	void on_actionYearlyStatistics_triggered();

	/* view menu actions */
	void on_actionViewList_triggered();
	void on_actionViewProfile_triggered();
	void on_actionViewInfo_triggered();
	void on_actionViewGlobe_triggered();
	void on_actionViewAll_triggered();
	void on_actionPreviousDC_triggered();
	void on_actionNextDC_triggered();
	void on_actionFullScreen_triggered(bool checked);

	/* other menu actions */
	void on_actionSelectEvents_triggered();
	void on_actionInputPlan_triggered();
	void on_actionAboutSubsurface_triggered();
	void on_actionUserManual_triggered();
	void on_actionDivePlanner_triggered();

	/* monitor resize of the info-profile splitter */
	void on_mainSplitter_splitterMoved(int pos, int idx);
	void on_infoProfileSplitter_splitterMoved(int pos, int idx);

	void current_dive_changed(int divenr);
	void initialUiSetup();

	void on_actionImportDiveLog_triggered();

	/* TODO: Move those slots below to it's own class */
	void on_profCalcAllTissues_toggled(bool triggered);
	void on_profCalcCeiling_toggled(bool triggered);
	void on_profDcCeiling_toggled(bool triggered);
	void on_profEad_toggled(bool triggered);
	void on_profIncrement3m_toggled(bool triggered);
	void on_profMod_toggled(bool triggered);
	void on_profNtl_tts_toggled(bool triggered);
	void on_profPO2_toggled(bool triggered);
	void on_profPhe_toggled(bool triggered);
	void on_profPn2_toggled(bool triggered);
	void on_profRuler_toggled(bool triggered);
	void on_profSAC_toggled(bool triggered);
	void on_profScaled_toggled(bool triggered);

protected:
	void closeEvent(QCloseEvent *);

public slots:
	void readSettings();
	void refreshDisplay(bool recreateDiveList = true);
	void showProfile();
	void editCurrentDive();

private:
	Ui::MainWindow ui;
	QAction *actionNextDive;
	QAction *actionPreviousDive;
	UserManual *helpView;
	CurrentState state;
	QString filter();
	static MainWindow *m_Instance;
	bool askSaveChanges();
	void writeSettings();
	void redrawProfile();
	void file_save();
	void file_save_as();
	void beginChangeState(CurrentState s);
	void saveSplitterSizes();
	QString lastUsedDir();
	void updateLastUsedDir(const QString& s);
};

MainWindow *mainWindow();

#endif // MAINWINDOW_H
