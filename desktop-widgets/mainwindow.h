// SPDX-License-Identifier: GPL-2.0
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
#include <QUuid>
#include <QProgressDialog>
#include <memory>

#include "ui_mainwindow.h"
#include "ui_plannerDetails.h"
#include "desktop-widgets/notificationwidget.h"
#include "desktop-widgets/filterwidget2.h"
#include "core/applicationstate.h"
#include "core/gpslocation.h"

#define NUM_RECENT_FILES 4

class QSortFilterProxyModel;
class DiveTripModel;
class QItemSelection;
class DiveListView;
class MainTab;
class QWebView;
class QSettings;
class UpdateManager;
class UserManual;
class DivePlannerWidget;
class ProfileWidget2;
class PlannerDetails;
class PlannerSettingsWidget;
class LocationInformationWidget;

typedef std::pair<QByteArray, QVariant> WidgetProperty;
typedef QVector<WidgetProperty> PropertyList;

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	enum {
		COLLAPSED,
		EXPANDED
	};

	enum CurrentState {
		VIEWALL,
		MAP_MAXIMIZED,
		INFO_MAXIMIZED,
		PROFILE_MAXIMIZED,
		LIST_MAXIMIZED,
		EDIT,
	};

	MainWindow();
	~MainWindow();
	static MainWindow *instance();
	void loadRecentFiles();
	void updateRecentFiles();
	void updateRecentFilesMenu();
	void addRecentFile(const QString &file, bool update);
	LocationInformationWidget *locationInformationWidget();
	void setTitle();

	void loadFiles(const QStringList files);
	void importFiles(const QStringList importFiles);
	void cleanUpEmpty();
	void setToolButtonsEnabled(bool enabled);
	void printPlan();
	void checkSurvey();
	void setApplicationState(ApplicationState state);
	bool inPlanner();
	NotificationWidget *getNotificationWidget();
	void enableDisableCloudActions();
	void enableDisableOtherDCsActions();
	void enterEditState();
	void exitEditState();
	void editDiveSite(dive_site *ds);

	std::unique_ptr<MainTab> mainTab;
	PlannerDetails *plannerDetails;
	PlannerSettingsWidget *divePlannerSettingsWidget;
	ProfileWidget2 *graphics;
	DivePlannerWidget *divePlannerWidget;
	DiveListView *diveList;
private
slots:
	/* file menu action */
	void recentFileTriggered(bool checked);
	void on_actionNew_triggered();
	void on_actionOpen_triggered();
	void on_actionSave_triggered();
	void on_actionSaveAs_triggered();
	void on_actionClose_triggered();
	void on_actionCloudstorageopen_triggered();
	void on_actionCloudstoragesave_triggered();
	void on_actionCloudOnline_triggered();
	void on_actionPrint_triggered();
	void on_actionPreferences_triggered();
	void on_actionQuit_triggered();
	void on_actionHash_images_triggered();

	/* log menu actions */
	void on_actionDownloadDC_triggered();
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
	void on_actionViewMap_triggered();
	void on_actionViewAll_triggered();
	void on_actionPreviousDC_triggered();
	void on_actionNextDC_triggered();
	void on_actionFullScreen_triggered(bool checked);

	/* other menu actions */
	void on_actionAboutSubsurface_triggered();
	void on_actionUserManual_triggered();
	void on_actionUserSurvey_triggered();
	void on_actionDivePlanner_triggered();
	void on_actionReplanDive_triggered();
	void on_action_Check_for_Updates_triggered();

	void selectionChanged();
	void initialUiSetup();

	void on_actionImportDiveLog_triggered();
	void on_actionImportDiveSites_triggered();

	/* TODO: Move those slots below to it's own class */
	void on_actionExport_triggered();
	void on_copy_triggered();
	void on_paste_triggered();
	void on_actionFilterTags_triggered();
	void on_actionConfigure_Dive_Computer_triggered();
	void setDefaultState();
	void setAutomaticTitle();
	void cancelCloudStorageOperation();
	void unsetProfHR();
	void unsetProfTissues();

protected:
	void closeEvent(QCloseEvent *);

signals:
	void showError(QString message);

public
slots:
	void turnOffNdlTts();
	void readSettings();
	void refreshDisplay(bool doRecreateDiveList = true);
	void recreateDiveList();
	void showProfile();
	void refreshProfile();
	void editCurrentDive();
	void planCanceled();
	void planCreated();
	void setEnabledToolbar(bool arg1);
	void setPlanNotes();
	// Some shortcuts like "change DC" or "copy/paste dive components"
	// should only be enabled when the profile's visible.
	void disableShortcuts(bool disablePaste = true);
	void enableShortcuts();
	void updateVariations(QString);
	void startDiveSiteEdit();

private:
	Ui::MainWindow ui;
	FilterWidget2 filterWidget2;
	QAction *actionNextDive;
	QAction *actionPreviousDive;
	QAction *undoAction;
	QAction *redoAction;
#ifndef NO_USERMANUAL
	UserManual *helpView;
#endif
	CurrentState state;
	CurrentState stateBeforeEdit;
	QString filter_open();
	QString filter_import();
	QString filter_import_dive_sites();
	static MainWindow *m_Instance;
	QString displayedFilename(QString fullFilename);
	bool askSaveChanges();
	bool okToClose(QString message);
	void closeCurrentFile();
	void setCurrentFile(const char *f);
	void updateCloudOnlineStatus();
	void showProgressBar();
	void hideProgressBar();
	void writeSettings();
	int file_save();
	int file_save_as();
	void setFileClean();
	void beginChangeState(CurrentState s);
	void saveSplitterSizes();
	void toggleCollapsible(bool toggle);
	void showFilterIfEnabled();
	void updateLastUsedDir(const QString &s);
	void enterState(CurrentState);
	bool filesAsArguments;
	UpdateManager *updateManager;
	LocationInformationWidget *diveSiteEdit;

	bool plannerStateClean();
	void setupForAddAndPlan(const char *model);
	void configureToolbar();
	void setupSocialNetworkMenu();
	QDialog *survey;
	QDialog *findMovedImagesDialog;
	struct dive copyPasteDive;
	struct dive_components what;
	QList<QAction *> profileToolbarActions;
	QStringList recentFiles;
	QAction *actionsRecent[NUM_RECENT_FILES];

	enum {
		FLAG_NONE = 0,
		FLAG_DISABLED = 1
	};

	struct Quadrant {
		QWidget *widget;
		int flags;
	};

	struct Quadrants {
		Quadrant topLeft;
		Quadrant topRight;
		Quadrant bottomLeft;
		Quadrant bottomRight;
	};

	Quadrants applicationState[(size_t)ApplicationState::Count];
	static void setQuadrant(const Quadrant &, QStackedWidget *);
	static void addWidgets(const Quadrant &, QStackedWidget *);
	void registerApplicationState(ApplicationState state, Quadrants q);

	GpsLocation *locationProvider;
	QMenu *connections;
	QAction *share_on_fb;
};

#endif // MAINWINDOW_H
