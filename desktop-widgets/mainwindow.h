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
#include "desktop-widgets/filterwidget.h"
#include "core/dive.h"
#include "core/subsurface-qt/divelistnotifier.h"

#define NUM_RECENT_FILES 4

class QSortFilterProxyModel;
class DiveTripModel;
class QItemSelection;
class DiveListView;
class DiveSiteListView;
class MainTab;
class MapWidget;
class QWebView;
class QSettings;
class UpdateManager;
class UserManual;
class PlannerWidgets;
class ProfileWidget;
class StatsWidget;
class LocationInformationWidget;

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	MainWindow();
	~MainWindow();
	static MainWindow *instance();
	void loadRecentFiles();
	void updateRecentFiles();
	void updateRecentFilesMenu();
	void addRecentFile(const QString &file, bool update);
	LocationInformationWidget *locationInformationWidget();
	void setTitle();

	enum class ApplicationState {
		Default,
		PlanDive,
		EditDiveSite,
		FilterDive,
		Statistics,
		DiveSites,
		MapMaximized,
		ProfileMaximized,
		ListMaximized,
		InfoMaximized,
		Count
	};

	void loadFiles(const QStringList &files);
	void importFiles(const QStringList &importFiles);
	void setToolButtonsEnabled(bool enabled);
	void setApplicationState(ApplicationState state);
	void enterPreviousState();
	NotificationWidget *getNotificationWidget();
	void enableDisableCloudActions();
	void editDiveSite(dive_site *ds);
	void setEnabledToolbar(bool arg1);

	std::unique_ptr<MainTab> mainTab;
	std::unique_ptr<PlannerWidgets> plannerWidgets;
	std::unique_ptr<StatsWidget> statistics;
	std::unique_ptr<DiveSiteListView> diveSites;
	std::unique_ptr<DiveListView> diveList;
	std::unique_ptr<ProfileWidget> profile;
	std::unique_ptr<MapWidget> mapWidget;
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
	void on_actionAddDive_triggered();
	void on_actionRenumber_triggered();
	void on_actionAutoGroup_triggered();
	void on_actionYearlyStatistics_triggered();

	/* view menu actions */
	void on_actionViewList_triggered();
	void on_actionViewProfile_triggered();
	void on_actionViewInfo_triggered();
	void on_actionViewMap_triggered();
	void on_actionViewDiveSites_triggered();
	void on_actionViewAll_triggered();
	void on_actionPreviousDC_triggered();
	void on_actionNextDC_triggered();
	void on_actionFullScreen_triggered(bool checked);

	/* other menu actions */
	void on_actionAboutSubsurface_triggered();
	void on_actionUserManual_triggered();
	void on_actionDivePlanner_triggered();
	void on_actionReplanDive_triggered();
	void on_action_Check_for_Updates_triggered();

	void divesSelected(const std::vector<dive *> &selection, dive *currentDive, int currentDC);
	void initialUiSetup();

	void on_actionImportDiveLog_triggered();
	void on_actionImportDiveSites_triggered();

	/* TODO: Move those slots below to it's own class */
	void on_actionExport_triggered();
	void on_copy_triggered();
	void on_paste_triggered();
	void on_actionFilterTags_triggered();
	void on_actionStats_triggered();
	void on_actionConfigure_Dive_Computer_triggered();
	void setDefaultState();
	void setAutomaticTitle();
	void cancelCloudStorageOperation();

protected:
	void closeEvent(QCloseEvent *);

signals:
	void showError(QString message);

public
slots:
	void readSettings();
	void planCanceled();
	void planCreated();
	// Some shortcuts like "change DC" or "copy/paste dive components"
	// should only be enabled when the profile's visible.
	void startDiveSiteEdit();

private:
	ApplicationState appState;
	std::vector<ApplicationState> state_stack;
	Ui::MainWindow ui;
	FilterWidget filterWidget;
	std::unique_ptr<QSplitter> topSplitter;
	std::unique_ptr<QSplitter> bottomSplitter;
	QAction *actionNextDive;
	QAction *actionPreviousDive;
	QAction *undoAction;
	QAction *redoAction;
#ifndef NO_USERMANUAL
	UserManual *helpView;
#endif
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
	void refreshDisplay();
	void updateAutogroup();
	void showProfile();
	int file_save();
	int file_save_as();
	void saveSplitterSizes();
	void restoreSplitterSizes();
	void updateLastUsedDir(const QString &s);
	bool filesAsArguments;
	UpdateManager *updateManager;
	std::unique_ptr<LocationInformationWidget> diveSiteEdit;

	bool inPlanner();
	bool plannerStateClean();
	void setupSocialNetworkMenu();
	QDialog *findMovedImagesDialog;
	struct dive copyPasteDive;
	struct dive_components what;
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
		bool allowUserChange; // Allow the user to change away from this state
		Quadrant topLeft;
		Quadrant topRight;
		Quadrant bottomLeft;
		Quadrant bottomRight;
	};

	Quadrants applicationState[(size_t)ApplicationState::Count];
	static void addWidgets(const Quadrant &);
	bool userMayChangeAppState() const;
	void setQuadrantWidget(QSplitter &splitter, const Quadrant &q, int pos);
	void setQuadrantWidgets(QSplitter &splitter, const Quadrant &left, const Quadrant &right);
	void registerApplicationState(ApplicationState state, Quadrants q);
	void disableShortcuts(bool disablePaste = true);
	void enableShortcuts();

	QMenu *connections;
	QAction *share_on_fb;
	void divesChanged(const QVector<dive *> &dives, DiveField field);
};

#endif // MAINWINDOW_H
