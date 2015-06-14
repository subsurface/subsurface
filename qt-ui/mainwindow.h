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

#include "ui_mainwindow.h"
#include "notificationwidget.h"

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
class QSettings;
class UpdateManager;
class UserManual;
class DivePlannerWidget;
class ProfileWidget2;
class PlannerDetails;
class PlannerSettingsWidget;
class QUndoStack;
class LocationInformationWidget;

enum MainWindowTitleFormat {
	MWTF_DEFAULT,
	MWTF_FILENAME
};

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	enum {
		COLLAPSED,
		EXPANDED
	};

	enum CurrentState {
		VIEWALL,
		GLOBE_MAXIMIZED,
		INFO_MAXIMIZED,
		PROFILE_MAXIMIZED,
		LIST_MAXIMIZED
	};

	MainWindow();
	virtual ~MainWindow();
	static MainWindow *instance();
	MainTab *information();
	void loadRecentFiles(QSettings *s);
	void addRecentFile(const QStringList &newFiles);
	void removeRecentFile(QStringList failedFiles);
	DiveListView *dive_list();
	GlobeGPS *globe();
	DivePlannerWidget *divePlannerWidget();
	PlannerSettingsWidget *divePlannerSettingsWidget();
	LocationInformationWidget *locationInformationWidget();
	void setTitle(enum MainWindowTitleFormat format = MWTF_FILENAME);

	// Some shortcuts like "change DC" or "copy/paste dive components"
	// should only be enabled when the profile's visible.
	void disableShortcuts(bool disablePaste = true);
	void enableShortcuts();
	void loadFiles(const QStringList files);
	void importFiles(const QStringList importFiles);
	void importTxtFiles(const QStringList fileNames);
	void cleanUpEmpty();
	void setToolButtonsEnabled(bool enabled);
	ProfileWidget2 *graphics() const;
	PlannerDetails *plannerDetails() const;
	void setLoadedWithFiles(bool filesFromCommandLine);
	bool filesFromCommandLine() const;
	void setPlanNotes(const char *notes);
	void printPlan();
	void checkSurvey(QSettings *s);
	void setApplicationState(const QByteArray& state);
	void showV2Dialog();
	QUndoStack *undoStack;
	NotificationWidget *getNotificationWidget();
	void enableDisableCloudActions();
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
	void on_actionPrint_triggered();
	void on_actionPreferences_triggered();
	void on_actionQuit_triggered();
	void on_actionHash_images_triggered();

	/* log menu actions */
	void on_actionDownloadDC_triggered();
	void on_actionDownloadWeb_triggered();
	void on_actionDivelogs_de_triggered();
	void on_actionEditDeviceNames_triggered();
	void on_actionAddDive_triggered();
	void on_actionEditDive_triggered();
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
	void on_actionAboutSubsurface_triggered();
	void on_actionUserManual_triggered();
	void on_actionUserSurvey_triggered();
	void on_actionDivePlanner_triggered();
	void on_actionReplanDive_triggered();
	void on_action_Check_for_Updates_triggered();
	void on_actionManage_dive_sites_triggered();

	void current_dive_changed(int divenr);
	void initialUiSetup();

	void on_actionImportDiveLog_triggered();

	/* TODO: Move those slots below to it's own class */
	void on_profCalcAllTissues_triggered(bool triggered);
	void on_profCalcCeiling_triggered(bool triggered);
	void on_profDcCeiling_triggered(bool triggered);
	void on_profEad_triggered(bool triggered);
	void on_profIncrement3m_triggered(bool triggered);
	void on_profMod_triggered(bool triggered);
	void on_profNdl_tts_triggered(bool triggered);
	void on_profPO2_triggered(bool triggered);
	void on_profPhe_triggered(bool triggered);
	void on_profPn2_triggered(bool triggered);
	void on_profHR_triggered(bool triggered);
	void on_profRuler_triggered(bool triggered);
	void on_profSAC_triggered(bool triggered);
	void on_profScaled_triggered(bool triggered);
	void on_profTogglePicture_triggered(bool triggered);
	void on_profTankbar_triggered(bool triggered);
	void on_profTissues_triggered(bool triggered);
	void on_actionExport_triggered();
	void on_copy_triggered();
	void on_paste_triggered();
	void on_actionFilterTags_triggered();
	void on_actionConfigure_Dive_Computer_triggered();
	void enableDiveSiteEdit(uint32_t id);
	void setDefaultState();

protected:
	void closeEvent(QCloseEvent *);

public
slots:
	void turnOffNdlTts();
	void readSettings();
	void refreshDisplay(bool doRecreateDiveList = true);
	void recreateDiveList();
	void showProfile();
	void editCurrentDive();
	void planCanceled();
	void planCreated();
	void setEnabledToolbar(bool arg1);
	void enableDiveSiteCreation();

private:
	Ui::MainWindow ui;
	QAction *actionNextDive;
	QAction *actionPreviousDive;
	UserManual *helpView;
	CurrentState state;
	QString filter();
	static MainWindow *m_Instance;
	QString displayedFilename(QString fullFilename);
	bool askSaveChanges();
	bool okToClose(QString message);
	void closeCurrentFile();
	void writeSettings();
	int file_save();
	int file_save_as();
	void beginChangeState(CurrentState s);
	void saveSplitterSizes();
	QString lastUsedDir();
	void updateLastUsedDir(const QString &s);
	void registerApplicationState(const QByteArray& state, QWidget *topLeft, QWidget *topRight, QWidget *bottomLeft, QWidget *bottomRight);
	bool filesAsArguments;
	UpdateManager *updateManager;

	bool plannerStateClean();
	void setupForAddAndPlan(const char *model);
	QDialog *survey;
	struct dive copyPasteDive;
	struct dive_components what;
	QList<QAction *> profileToolbarActions;

	struct WidgetForQuadrant {
		WidgetForQuadrant(QWidget *tl = 0, QWidget *tr = 0, QWidget *bl = 0, QWidget *br = 0) :
			topLeft(tl), topRight(tr), bottomLeft(bl), bottomRight(br) {}
		QWidget *topLeft;
		QWidget *topRight;
		QWidget *bottomLeft;
		QWidget *bottomRight;
	};
	QHash<QByteArray, WidgetForQuadrant> applicationState;
	QByteArray currentApplicationState;
};

#endif // MAINWINDOW_H
