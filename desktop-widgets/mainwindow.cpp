// SPDX-License-Identifier: GPL-2.0
/*
 * mainwindow.cpp
 *
 * classes for the main UI window in Subsurface
 */
#include "mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QDesktopWidget>
#endif
#include <QSettings>
#include <QShortcut>
#include <QStatusBar>
#include <QNetworkProxy>
#include <QUndoStack>

#include "core/color.h"
#include "core/device.h"
#include "core/divelog.h"
#include "core/divesitehelpers.h"
#include "core/errorhelper.h"
#include "core/file.h"
#include "core/gettextfromc.h"
#include "core/git-access.h"
#include "core/import-csv.h"
#include "core/planner.h"
#include "core/qthelper.h"
#include "core/selection.h"
#include "core/subsurface-string.h"
#include "core/trip.h"
#include "core/version.h"
#include "core/windowtitleupdate.h"

#include "core/settings/qPrefCloudStorage.h"
#include "core/settings/qPrefDisplay.h"

#include "desktop-widgets/about.h"
#include "desktop-widgets/divelistview.h"
#include "desktop-widgets/divelogexportdialog.h"
#include "desktop-widgets/divelogimportdialog.h"
#include "desktop-widgets/diveplanner.h"
#include "desktop-widgets/divesiteimportdialog.h"
#include "desktop-widgets/divesitelistview.h"
#include "desktop-widgets/downloadfromdivecomputer.h"
#include "desktop-widgets/findmovedimagesdialog.h"
#include "desktop-widgets/locationinformation.h"
#include "desktop-widgets/mapwidget.h"
#include "desktop-widgets/subsurfacewebservices.h"
#include "desktop-widgets/tab-widgets/maintab.h"
#include "desktop-widgets/updatemanager.h"
#include "desktop-widgets/simplewidgets.h"
#include "desktop-widgets/statswidget.h"
#include "commands/command.h"

#include "profilewidget.h"
#include "profile-widget/profilewidget2.h"

#ifndef NO_PRINTING
#include "desktop-widgets/printdialog.h"
#include "desktop-widgets/templatelayout.h"
#endif

#include "qt-models/cylindermodel.h"
#include "qt-models/divepicturemodel.h"
#include "qt-models/diveplannermodel.h"
#include "qt-models/filtermodels.h"
#include "qt-models/tankinfomodel.h"
#include "qt-models/weightsysteminfomodel.h"
#include "qt-models/yearlystatisticsmodel.h"
#include "preferences/preferencesdialog.h"

#ifndef NO_USERMANUAL
#include "usermanual.h"
#endif

namespace {
	QProgressDialog *progressDialog = nullptr;
	bool progressDialogCanceled = false;
	int progressCounter = 0;
}

extern "C" int updateProgress(const char *text)
{
	if (verbose)
		qDebug() << "git storage:" << text;
	if (progressDialog) {
		// apparently we don't always get enough space to show the full label
		// so let's manually make enough space (but don't shrink the existing size)
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
		int width = QFontMetrics(qApp->font()).width(text) + 100;
#else // QT 5.11 or newer
		int width = QFontMetrics(qApp->font()).horizontalAdvance(text) + 100;
#endif
		if (width > progressDialog->width())
			progressDialog->resize(width + 20, progressDialog->height());
		progressDialog->setLabelText(text);
		progressDialog->setValue(++progressCounter);
		if (progressCounter == 100)
			progressCounter = 0; // yes this is silly, but we really don't know how long it will take
	}
	qApp->processEvents();
	return progressDialogCanceled;
}

MainWindow *MainWindow::m_Instance = nullptr;

extern "C" void showErrorFromC(char *buf)
{
	QString error(buf);
	free(buf);
	emit MainWindow::instance()->showError(error);
}

MainWindow::MainWindow() :
	appState((ApplicationState)-1), // Invalid state
	actionNextDive(nullptr),
	actionPreviousDive(nullptr),
#ifndef NO_USERMANUAL
	helpView(0),
#endif
	findMovedImagesDialog(nullptr)
{
	Q_ASSERT_X(m_Instance == NULL, "MainWindow", "MainWindow recreated!");
	m_Instance = this;
	ui.setupUi(this);
	read_hashes();
	Command::init();
	// Define the States of the Application Here, Currently the states are situations where the different
	// widgets will change on the mainwindow.

	topSplitter.reset(new QSplitter(Qt::Horizontal));
	bottomSplitter.reset(new QSplitter(Qt::Horizontal));

	// for the "default" mode
	mainTab.reset(new MainTab);
	diveList.reset(new DiveListView);
#ifdef MAP_SUPPORT
	mapWidget.reset(MapWidget::instance()); // Yes, this is ominous see comment in mapwidget.cpp.
#endif
	plannerWidgets.reset(new PlannerWidgets);
	statistics.reset(new StatsWidget);
	diveSites.reset(new DiveSiteListView);
	profile.reset(new ProfileWidget);

	diveSiteEdit.reset(new LocationInformationWidget);

	registerApplicationState(ApplicationState::Default, { true, { mainTab.get(), FLAG_NONE },  { profile.get(), FLAG_NONE },
								    { diveList.get(), FLAG_NONE }, { mapWidget.get(), FLAG_NONE } });
	registerApplicationState(ApplicationState::PlanDive, { false, { &plannerWidgets->plannerWidget, FLAG_NONE },         { profile.get(), FLAG_NONE },
								      { &plannerWidgets->plannerSettingsWidget, FLAG_NONE }, { &plannerWidgets->plannerDetails, FLAG_NONE } });
	registerApplicationState(ApplicationState::EditDiveSite, { false, { diveSiteEdit.get(), FLAG_NONE }, { profile.get(), FLAG_DISABLED },
									  { diveList.get(), FLAG_DISABLED }, { mapWidget.get(), FLAG_NONE } });
	registerApplicationState(ApplicationState::FilterDive, { true, { mainTab.get(), FLAG_NONE },  { profile.get(), FLAG_NONE },
								       { diveList.get(), FLAG_NONE }, { &filterWidget, FLAG_NONE } });
	registerApplicationState(ApplicationState::Statistics, { true, { statistics.get(), FLAG_NONE }, { nullptr, FLAG_NONE },
								       { diveList.get(), FLAG_DISABLED },   { &filterWidget, FLAG_NONE } });
	registerApplicationState(ApplicationState::DiveSites, { false, { diveSites.get(), FLAG_NONE },  { profile.get(), FLAG_NONE },
								       { diveList.get(), FLAG_NONE }, { mapWidget.get(), FLAG_NONE } });
	registerApplicationState(ApplicationState::MapMaximized, { true, { nullptr, FLAG_NONE }, { nullptr, FLAG_NONE },
									 { nullptr, FLAG_NONE }, { mapWidget.get(), FLAG_NONE } });
	registerApplicationState(ApplicationState::ProfileMaximized, { true, { nullptr, FLAG_NONE }, { profile.get(), FLAG_NONE },
									     { nullptr, FLAG_NONE }, { nullptr, FLAG_NONE } });
	registerApplicationState(ApplicationState::ListMaximized, { true, { nullptr, FLAG_NONE },        { nullptr, FLAG_NONE },
									  { diveList.get(), FLAG_NONE }, { nullptr, FLAG_NONE } });
	registerApplicationState(ApplicationState::InfoMaximized, { true, { mainTab.get(), FLAG_NONE }, { nullptr, FLAG_NONE },
									  { nullptr, FLAG_NONE },       { nullptr, FLAG_NONE } });
	restoreSplitterSizes();
	setApplicationState(ApplicationState::Default);

	setWindowIcon(QIcon(":subsurface-icon"));
	if (!QIcon::hasThemeIcon("window-close")) {
		QIcon::setThemeName("subsurface");
	}
	connect(diveList.get(), &DiveListView::divesSelected, this, &MainWindow::divesSelected);
	connect(&diveListNotifier, &DiveListNotifier::settingsChanged, this, &MainWindow::readSettings);
	for (int i = 0; i < NUM_RECENT_FILES; i++) {
		actionsRecent[i] = new QAction(this);
		actionsRecent[i]->setData(i);
		ui.menuFile->insertAction(ui.actionQuit, actionsRecent[i]);
		connect(actionsRecent[i], SIGNAL(triggered(bool)), this, SLOT(recentFileTriggered(bool)));
	}
	ui.menuFile->insertSeparator(ui.actionQuit);
	connect(DivePlannerPointsModel::instance(), SIGNAL(planCreated()), this, SLOT(planCreated()));
	connect(DivePlannerPointsModel::instance(), SIGNAL(planCanceled()), this, SLOT(planCanceled()));
	connect(this, &MainWindow::showError, ui.mainErrorMessage, &NotificationWidget::showError, Qt::AutoConnection);

	connect(&windowTitleUpdate, &WindowTitleUpdate::updateTitle, this, &MainWindow::setAutomaticTitle);
	connect(&diveListNotifier, &DiveListNotifier::numShownChanged, this, &MainWindow::setAutomaticTitle);
	// monitor when dives changed - but only in verbose mode
	// careful - changing verbose at runtime isn't enough (of course that could be added if we want it)
	if (verbose)
		connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &MainWindow::divesChanged);

#ifdef NO_PRINTING
	ui.menuFile->removeAction(ui.actionPrint);
#endif
	enableDisableCloudActions();

	ui.mainErrorMessage->hide();
	setEnabledToolbar(false);
	initialUiSetup();
	readSettings();
	diveList->setFocus();
	diveList->expand(diveList->model()->index(0, 0));
	diveList->scrollTo(diveList->model()->index(0, 0), QAbstractItemView::PositionAtCenter);
#ifdef NO_USERMANUAL
	ui.menuHelp->removeAction(ui.actionUserManual);
#endif
	memset(&copyPasteDive, 0, sizeof(copyPasteDive));
	memset(&what, 0, sizeof(what));

	updateManager = new UpdateManager(this);
	undoAction = Command::undoAction(this);
	redoAction = Command::redoAction(this);
	undoAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
	redoAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z));
	ui.menu_Edit->addActions({ undoAction, redoAction });

#ifndef NO_PRINTING
	// copy the bundled print templates to the user path
	QStringList templateBackupList;
	QString templatePathUser(getPrintingTemplatePathUser());
	copy_bundled_templates(getPrintingTemplatePathBundle(), templatePathUser, &templateBackupList);
	if (templateBackupList.length()) {
		QMessageBox msgBox(this);
		templatePathUser.replace("\\", "/");
		templateBackupList.replaceInStrings(templatePathUser + "/", "");
		msgBox.setWindowTitle(tr("Template backup created"));
		msgBox.setText(tr("The following backup printing templates were created:\n\n%1\n\n"
			"Location:\n%2\n\n"
			"Please note that as of this version of Subsurface the default templates\n"
			"are read-only and should not be edited directly, since the application\n"
			"can overwrite them on startup.").arg(templateBackupList.join("\n")).arg(templatePathUser));
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.exec();
	}
	set_bundled_templates_as_read_only();
	find_all_templates();
#endif

	setupSocialNetworkMenu();
	set_git_update_cb(&updateProgress);
	set_error_cb(&showErrorFromC);

// full screen support is buggy on Windows and Ubuntu.
// require the FULLSCREEN_SUPPORT macro to enable it!
#ifndef FULLSCREEN_SUPPORT
	ui.actionFullScreen->setEnabled(false);
	ui.actionFullScreen->setVisible(false);
	setWindowState(windowState() & ~Qt::WindowFullScreen);
#endif
}

static void clearSplitter(QSplitter &splitter)
{
	// Qt's ownership model is absolutely hare-brained.
	// To remove a widget from a splitter, you reparent it, which
	// informs the splitter via a signal. Wow.
	while (splitter.count() > 0)
		splitter.widget(0)->setParent(nullptr);
}

MainWindow::~MainWindow()
{
	// Remove widgets from the splitters so that they don't delete singletons.
	clearSplitter(*topSplitter);
	clearSplitter(*bottomSplitter);
	clearSplitter(*ui.mainSplitter);
	write_hashes();
	m_Instance = nullptr;
}

void MainWindow::setupSocialNetworkMenu()
{
}

void MainWindow::editDiveSite(dive_site *ds)
{
	if (!ds)
		return;
	diveSiteEdit->initFields(ds);
	state_stack.push_back(appState);
	setApplicationState(ApplicationState::EditDiveSite);
}

void MainWindow::startDiveSiteEdit()
{
	if (current_dive)
		editDiveSite(get_dive_site_for_dive(current_dive));
}

void MainWindow::enableDisableCloudActions()
{
	ui.actionCloudstorageopen->setEnabled(prefs.cloud_verification_status == qPrefCloudStorage::CS_VERIFIED);
	ui.actionCloudstoragesave->setEnabled(prefs.cloud_verification_status == qPrefCloudStorage::CS_VERIFIED);
}

void MainWindow::setDefaultState()
{
	setApplicationState(ApplicationState::Default);
}

MainWindow *MainWindow::instance()
{
	return m_Instance;
}

// This gets called after one or more dives were added, edited or downloaded for a dive computer
void MainWindow::refreshDisplay()
{
	setApplicationState(ApplicationState::Default);
	diveList->setEnabled(true);
	diveList->setFocus();
}

void MainWindow::updateAutogroup()
{
	ui.actionAutoGroup->setChecked(divelog.autogroup);
}

void MainWindow::divesSelected(const std::vector<dive *> &selection, dive *currentDive, int currentDC)
{
	// We call plotDive first, so that the profile can decide which
	// dive computer to plot. The plotted dive computer is then
	// used for displaying data in the tab-widgets.
	profile->plotDive(currentDive, currentDC);
	mainTab->updateDiveInfo(selection, profile->d, profile->dc);

	// Activate cursor keys to switch through DCs if there are more than one DC.
	if (currentDive) {
		bool nr = number_of_computers(current_dive) > 1;
		enableShortcuts();
		ui.actionNextDC->setEnabled(nr);
		ui.actionPreviousDC->setEnabled(nr);
	}
}

void MainWindow::on_actionNew_triggered()
{
	on_actionClose_triggered();
}

static QString lastUsedDir()
{
	QString lastDir = QDir::homePath();

	if (QDir(qPrefDisplay::lastDir()).exists())
		lastDir = qPrefDisplay::lastDir();
	return lastDir;
}

void MainWindow::on_actionOpen_triggered()
{
	if (!okToClose(tr("Please save or cancel the current dive edit before opening a new file.")))
		return;

	// yes, this look wrong to use getSaveFileName() for the open dialog, but we need to be able
	// to enter file names that don't exist in order to use our git syntax /path/to/dir[branch]
	// with is a potentially valid input, but of course won't exist. So getOpenFileName() wouldn't work
	QFileDialog dialog(this, tr("Open file"), lastUsedDir(), filter_open());
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setViewMode(QFileDialog::Detail);
	dialog.setLabelText(QFileDialog::Accept, tr("Open"));
	dialog.setLabelText(QFileDialog::Reject, tr("Cancel"));
	dialog.setAcceptMode(QFileDialog::AcceptOpen);
	QStringList filenames;
	if (dialog.exec())
		filenames = dialog.selectedFiles();
	if (filenames.isEmpty())
		return;
	updateLastUsedDir(QFileInfo(filenames.first()).dir().path());
	closeCurrentFile();
	// some file dialogs decide to add the default extension to a filename without extension
	// so we would get dir[branch].ssrf when trying to select dir[branch].
	// let's detect that and remove the incorrect extension
	QStringList cleanFilenames;
	QRegularExpression reg(".*\\[[^]]+]\\.ssrf", QRegularExpression::CaseInsensitiveOption);

	Q_FOREACH (QString filename, filenames) {
		if (reg.match(filename).hasMatch())
			filename.remove(QRegularExpression("\\.ssrf$", QRegularExpression::CaseInsensitiveOption));
		cleanFilenames << filename;
	}
	loadFiles(cleanFilenames);
}

void MainWindow::on_actionSave_triggered()
{
	mainTab->stealFocus(); // Make sure that any currently edited field is updated before saving.
	file_save();
}

void MainWindow::on_actionSaveAs_triggered()
{
	mainTab->stealFocus(); // Make sure that any currently edited field is updated before saving.
	file_save_as();
}

void MainWindow::on_actionCloudstorageopen_triggered()
{
	if (!okToClose(tr("Please save or cancel the current dive edit before opening a new file.")))
		return;

	QString filename;
	if (getCloudURL(filename))
		return;

	if (verbose)
		qDebug() << "Opening cloud storage from:" << filename;

	closeCurrentFile();

	showProgressBar();
	QByteArray fileNamePtr = QFile::encodeName(filename);
	if (!parse_file(fileNamePtr.data(), &divelog))
		setCurrentFile(fileNamePtr.data());
	process_loaded_dives();
	hideProgressBar();
	refreshDisplay();
	updateAutogroup();
}

// Return whether saving to cloud is OK. If it isn't, show an error return false.
static bool saveToCloudOK()
{
	if (!divelog.dives->nr) {
		report_error(qPrintable(gettextFromC::tr("Don't save an empty log to the cloud")));
		return false;
	}
	return true;
}

void MainWindow::on_actionCloudstoragesave_triggered()
{
	QString filename;
	if (!saveToCloudOK())
		return;
	if (getCloudURL(filename))
		return;

	if (verbose)
		qDebug() << "Saving cloud storage to:" << filename;
	mainTab->stealFocus(); // Make sure that any currently edited field is updated before saving.

	showProgressBar();
	int error = save_dives(qPrintable(filename));
	hideProgressBar();
	if (error)
		return;

	setCurrentFile(qPrintable(filename));
	Command::setClean();
}

void MainWindow::on_actionCloudOnline_triggered()
{
	bool isOffline = !ui.actionCloudOnline->isChecked();
	if (isOffline == git_local_only)
		return;

	// Refuse to go online if there is an edit in progress
	if (!isOffline && inPlanner()) {
		QMessageBox::warning(this, tr("Warning"), tr("Please save or cancel the current dive edit before going online"));
		// We didn't switch to online, therefore uncheck the checkbox
		ui.actionCloudOnline->setChecked(false);
		return;
	}

	git_local_only = isOffline;
	if (!isOffline) {
		// User requests to go online. Try to sync cloud storage
		if (!Command::isClean()) {
			// If there are unsaved changes, ask the user if they want to save them.
			// If they don't, they have to sync manually.
			if (QMessageBox::warning(this, tr("Save changes?"),
						 tr("You have unsaved changes. Do you want to commit them to the cloud storage?\n"
						    "If answering no, the cloud will only be synced on next call to "
						    "\"Open cloud storage\" or \"Save to cloud storage\"."),
						 QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
				on_actionCloudstoragesave_triggered();
		} else {
			// If there are no unsaved changes, let's just try to load the remote cloud
			on_actionCloudstorageopen_triggered();
		}
		if (git_local_only)
			report_error(qPrintable(tr("Failure taking cloud storage online")));
	}

	setTitle();
	updateCloudOnlineStatus();
}

bool MainWindow::okToClose(QString message)
{
	if (inPlanner()) {
		QMessageBox::warning(this, tr("Warning"), message);
		return false;
	}
	if (!Command::isClean() && askSaveChanges() == false)
		return false;

	return true;
}

void MainWindow::closeCurrentFile()
{
	/* free the dives and trips */
	clear_git_id();
	clear_dive_file_data(); // this clears all the core data structures and resets the models
	setCurrentFile(nullptr);
	diveList->setSortOrder(DiveTripModelBase::NR, Qt::DescendingOrder);
	if (!existing_filename)
		setTitle();
	disableShortcuts();
}

void MainWindow::updateCloudOnlineStatus()
{
	bool is_cloud = existing_filename && prefs.cloud_verification_status == qPrefCloudStorage::CS_VERIFIED &&
			strstr(existing_filename, prefs.cloud_base_url);
	ui.actionCloudOnline->setEnabled(is_cloud);
	ui.actionCloudOnline->setChecked(is_cloud && !git_local_only);
}

void MainWindow::setCurrentFile(const char *f)
{
	set_filename(f);
	setTitle();
	updateCloudOnlineStatus();
}

void MainWindow::on_actionClose_triggered()
{
	if (okToClose(tr("Please save or cancel the current dive edit before closing the file."))) {
		closeCurrentFile();
		setApplicationState(ApplicationState::Default);
	}
}

void MainWindow::updateLastUsedDir(const QString &dir)
{
	qPrefDisplay::set_lastDir(dir);
}

void MainWindow::on_actionPrint_triggered()
{
#ifndef NO_PRINTING
	// When in planner, only print the planned dive.
	dive *singleDive = appState == ApplicationState::PlanDive ? plannerWidgets->getDive()
								  : nullptr;
	PrintDialog dlg(singleDive, this);

	dlg.exec();
#endif
}

void MainWindow::disableShortcuts(bool disablePaste)
{
	undoAction->setEnabled(false);
	redoAction->setEnabled(false);
	ui.actionPreviousDC->setShortcut(QKeySequence());
	ui.actionNextDC->setShortcut(QKeySequence());
	ui.copy->setShortcut(QKeySequence());
	if (disablePaste)
		ui.paste->setShortcut(QKeySequence());
}

void MainWindow::enableShortcuts()
{
	undoAction->setEnabled(true);
	redoAction->setEnabled(true);
	ui.actionPreviousDC->setShortcut(Qt::Key_Left);
	ui.actionNextDC->setShortcut(Qt::Key_Right);
	ui.copy->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C));
	ui.paste->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_V));
}

void MainWindow::showProfile()
{
	enableShortcuts();
	profile->plotDive(current_dive, profile->dc);
	setApplicationState(ApplicationState::Default);
}

void MainWindow::on_actionPreferences_triggered()
{
	// the refreshPages() is currently done for just one
	// reason. Allow the user to define a default cylinder that
	// is not hardcoded but coming from the logbook.
	PreferencesDialog::instance()->refreshPages();

	PreferencesDialog::instance()->show();
	PreferencesDialog::instance()->raise();
}

void MainWindow::on_actionQuit_triggered()
{
	if (!okToClose(tr("Please save or cancel the current dive edit before quiting the application.")))
		return;

	writeSettings();
	QApplication::quit();
}

void MainWindow::on_actionDownloadDC_triggered()
{
	DownloadFromDCWidget dlg(this);
	dlg.exec();
}

void MainWindow::on_actionDivelogs_de_triggered()
{
	DivelogsDeWebServices::instance()->downloadDives();
}

bool MainWindow::inPlanner()
{
	return DivePlannerPointsModel::instance()->currentMode() == DivePlannerPointsModel::PLAN;
}

bool MainWindow::plannerStateClean()
{
	if (progressDialog)
		// we are accessing the cloud, so let's not switch into Add or Plan mode
		return false;

	if (inPlanner()) {
		QMessageBox::warning(this, tr("Warning"), tr("Please save or cancel the current dive edit before trying to add a dive."));
		return false;
	}
	return true;
}

void MainWindow::planCanceled()
{
	showProfile();
	refreshDisplay();
}

void MainWindow::planCreated()
{
	// make sure our UI is in a consistent state
	showProfile();
	setApplicationState(ApplicationState::Default);
	diveList->setEnabled(true);
	diveList->setFocus();
}

void MainWindow::on_actionReplanDive_triggered()
{
	if (!plannerStateClean() || !current_dive || !userMayChangeAppState())
		return;
	else if (!is_dc_planner(&current_dive->dc)) {
		if (QMessageBox::warning(this, tr("Warning"), tr("Trying to replan a dive that's not a planned dive."),
					 QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)
					return;
	}

	// put us in PLAN mode
	setApplicationState(ApplicationState::PlanDive);

	disableShortcuts(true);
	plannerWidgets->prepareReplanDive(current_dive);
	profile->setPlanState(plannerWidgets->getDive(), profile->dc);
	plannerWidgets->replanDive(profile->dc);
}

void MainWindow::on_actionDivePlanner_triggered()
{
	if (!plannerStateClean() || !userMayChangeAppState())
		return;

	// put us in PLAN mode
	setApplicationState(ApplicationState::PlanDive);

	disableShortcuts(true);
	plannerWidgets->preparePlanDive(current_dive);
	profile->setPlanState(plannerWidgets->getDive(), 0);
	plannerWidgets->planDive();
}

void MainWindow::on_actionAddDive_triggered()
{
	if (!plannerStateClean())
		return;

	// create a dive an hour from now with a default depth (15m/45ft) and duration (40 minutes)
	// as a starting point for the user to edit
	struct dive d = { 0 };
	d.id = dive_getUniqID();
	d.when = QDateTime::currentMSecsSinceEpoch() / 1000L + gettimezoneoffset() + 3600;
	d.dc.duration.seconds = 40 * 60;
	d.dc.maxdepth.mm = M_OR_FT(15, 45);
	d.dc.meandepth.mm = M_OR_FT(13, 39); // this creates a resonable looking safety stop
	make_manually_added_dc(&d.dc);
	fake_dc(&d.dc);
	fixup_dive(&d);

	Command::addDive(&d, divelog.autogroup, true);
}

void MainWindow::on_actionRenumber_triggered()
{
	RenumberDialog dialog(false, this);
	dialog.exec();
}

void MainWindow::on_actionAutoGroup_triggered()
{
	divelog.autogroup = ui.actionAutoGroup->isChecked();
	if (divelog.autogroup)
		Command::autogroupDives();
	else
		Command::removeAutogenTrips();
}

void MainWindow::on_actionYearlyStatistics_triggered()
{
	QDialog d;
	QVBoxLayout *l = new QVBoxLayout(&d);
	YearlyStatisticsModel *m = new YearlyStatisticsModel();
	QTreeView *view = new QTreeView();
	view->setModel(m);
	l->addWidget(view);
	d.resize(lrint(width() * .8), height() / 2);
	d.move(lrint(width() * .1), height() / 4);
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), &d);
	connect(close, SIGNAL(activated()), &d, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), &d);
	connect(quit, SIGNAL(activated()), this, SLOT(close()));
	d.setWindowFlags(Qt::Window | Qt::CustomizeWindowHint
		| Qt::WindowCloseButtonHint | Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint);
	d.setWindowTitle(tr("Yearly statistics"));
	d.setWindowIcon(QIcon(":subsurface-icon"));
	d.setSizeGripEnabled(true);
	d.exec();
}

void MainWindow::on_actionViewList_triggered()
{
	if (!userMayChangeAppState())
		return;
	setApplicationState(ApplicationState::ListMaximized);
}

void MainWindow::on_actionViewProfile_triggered()
{
	if (!userMayChangeAppState())
		return;
	setApplicationState(ApplicationState::ProfileMaximized);
}

void MainWindow::on_actionViewInfo_triggered()
{
	if (!userMayChangeAppState())
		return;
	setApplicationState(ApplicationState::InfoMaximized);
}

void MainWindow::on_actionViewMap_triggered()
{
	if (!userMayChangeAppState())
		return;
	setApplicationState(ApplicationState::MapMaximized);
}

void MainWindow::on_actionViewDiveSites_triggered()
{
	if (!userMayChangeAppState())
		return;
	state_stack.push_back(appState);
	setApplicationState(ApplicationState::DiveSites);
}

void MainWindow::on_actionViewAll_triggered()
{
	if (!userMayChangeAppState())
		return;
	setApplicationState(ApplicationState::Default);
}

void MainWindow::saveSplitterSizes()
{
	// Only save splitters if all four quadrants are shown
	if (ui.mainSplitter->count() < 2 || topSplitter->count() < 2 || bottomSplitter->count() < 2)
		return;

	QSettings settings;
	settings.beginGroup("MainWindow");
	settings.setValue("mainSplitter", ui.mainSplitter->saveState());
	settings.setValue("topSplitter", topSplitter->saveState());
	settings.setValue("bottomSplitter", bottomSplitter->saveState());
}

void MainWindow::restoreSplitterSizes()
{
	// Only restore splitters if all four quadrants are shown
	if (ui.mainSplitter->count() < 2 || topSplitter->count() < 2 || bottomSplitter->count() < 2)
		return;


	QSettings settings;
	settings.beginGroup("MainWindow");
	if (settings.value("mainSplitter").isValid()) {
		ui.mainSplitter->restoreState(settings.value("mainSplitter").toByteArray());

		topSplitter->restoreState(settings.value("topSplitter").toByteArray());
		bottomSplitter->restoreState(settings.value("bottomSplitter").toByteArray());
	} else {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
		const int appH = qApp->desktop()->size().height();
		const int appW = qApp->desktop()->size().width();
#else
		const int appH = screen()->size().height();
		const int appW = screen()->size().width();
#endif
		ui.mainSplitter->setSizes({ appH * 3 / 5, appH * 2 / 5 });
		topSplitter->setSizes({ appW / 2, appW / 2 });
		bottomSplitter->setSizes({ appW * 3 / 5, appW * 2 / 5 });
	}
}

void MainWindow::on_actionPreviousDC_triggered()
{
	profile->prevDC();
	// TODO: remove
	mainTab->updateDiveInfo(getDiveSelection(), profile->d, profile->dc);
}

void MainWindow::on_actionNextDC_triggered()
{
	profile->nextDC();
	// TODO: remove
	mainTab->updateDiveInfo(getDiveSelection(), profile->d, profile->dc);
}

void MainWindow::on_actionFullScreen_triggered(bool checked)
{
	if (checked) {
		setWindowState(windowState() | Qt::WindowFullScreen);
	} else {
		setWindowState(windowState() & ~Qt::WindowFullScreen);
	}
}

void MainWindow::on_actionAboutSubsurface_triggered()
{
	SubsurfaceAbout dlg(this);

	dlg.exec();
}

void MainWindow::on_action_Check_for_Updates_triggered()
{
	if (!updateManager)
		updateManager = new UpdateManager(this);

	updateManager->checkForUpdates();
}

void MainWindow::on_actionUserManual_triggered()
{
#ifndef NO_USERMANUAL
	if (!helpView)
		helpView = new UserManual(this);
	helpView->show();
#endif
}

void MainWindow::on_actionHash_images_triggered()
{
	if(!findMovedImagesDialog)
		findMovedImagesDialog = new FindMovedImagesDialog(this);
	findMovedImagesDialog->show();
}

QString MainWindow::filter_open()
{
	QString f = tr("Dive log files") +
		    " (*.ssrf"
		    " *.xml"
		    " *.can"
		    " *.db"
		    " *.sql"
		    " *.dld"
		    " *.jlb"
		    " *.lvd"
		    " *.sde"
		    " *.udcf"
		    " *.uddf"
		    " *.dlf"
		    " *.log"
		    " *.txt"
		    " *.apd"
		    " *.dive"
		    " *.zxu *.zxl"
		    ");;";

	f += tr("Subsurface files") + " (*.ssrf *.xml);;";
	f += tr("Cochran") + " (*.can);;";
	f += tr("DiveLogs.de") + " (*.dld);;";
	f += tr("JDiveLog") + " (*.jlb);;";
	f += tr("Liquivision") + " (*.lvd);;";
	f += tr("Suunto") + " (*.sde *.db);;";
	f += tr("UDCF") + " (*.udcf);;";
	f += tr("UDDF") + " (*.uddf);;";
	f += tr("XML") + " (*.xml);;";
	f += tr("Divesoft") + " (*.dlf);;";
	f += tr("Datatrak/WLog") + " (*.log);;";
	f += tr("MkVI files") + " (*.txt);;";
	f += tr("APD log viewer") + " (*.apd);;";
	f += tr("OSTCtools") + " (*.dive);;";
	f += tr("DAN DL7") + " (*.zxu *.zxl)";

	return f;
}

QString MainWindow::filter_import()
{
	QString f = tr("Dive log files") +
		    " (*.ssrf"
		    " *.xml"
		    " *.can"
		    " *.csv"
		    " *.db"
		    " *.sql"
		    " *.dld"
		    " *.jlb"
		    " *.lvd"
		    " *.sde"
		    " *.udcf"
		    " *.uddf"
		    " *.dlf"
		    " *.log"
		    " *.txt"
		    " *.apd"
		    " *.dive"
		    " *.zxu *.zxl"
		    ");;";

	f += tr("Subsurface files") + " (*.ssrf *.xml);;";
	f += tr("Cochran") + " (*.can);;";
	f += tr("CSV") + " (*.csv *.CSV);;";
	f += tr("DiveLogs.de") + " (*.dld);;";
	f += tr("JDiveLog") + " (*.jlb);;";
	f += tr("Liquivision") + " (*.lvd);;";
	f += tr("Suunto") + " (*.sde *.db);;";
	f += tr("UDCF") + " (*.udcf);;";
	f += tr("UDDF") + " (*.uddf);;";
	f += tr("XML") + " (*.xml);;";
	f += tr("Divesoft") + " (*.dlf);;";
	f += tr("Datatrak/WLog") + " (*.log);;";
	f += tr("MkVI files") + " (*.txt);;";
	f += tr("APD log viewer") + " (*.apd);;";
	f += tr("OSTCtools") + " (*.dive);;";
	f += tr("DAN DL7") + " (*.zxu *.zxl);;";
	f += tr("All files") + " (*.*)";

	return f;
}

QString MainWindow::filter_import_dive_sites()
{
	QString f = tr("Dive site files") +
		    " (*.ssrf"
		    " *.xml"
		    ");;";

	f += tr("All files") + " (*.*)";

	return f;
}

bool MainWindow::askSaveChanges()
{
	QMessageBox response(this);

	QString message = existing_filename ?
		tr("Do you want to save the changes that you made in the file %1?").arg(displayedFilename(existing_filename)) :
		tr("Do you want to save the changes that you made in the data file?");

	response.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
	response.setDefaultButton(QMessageBox::Save);
	response.setText(message);
	response.setWindowTitle(tr("Save changes?")); // Not displayed on MacOSX as described in Qt API
	response.setInformativeText(tr("Changes will be lost if you don't save them."));
	response.setIcon(QMessageBox::Warning);
	response.setWindowModality(Qt::WindowModal);
	int ret = response.exec();

	switch (ret) {
	case QMessageBox::Save:
		file_save();
		return true;
	case QMessageBox::Discard:
		return true;
	}
	return false;
}

void MainWindow::initialUiSetup()
{
	QSettings settings;
	settings.beginGroup("MainWindow");
	if (settings.value("maximized", isMaximized()).value<bool>()) {
		showMaximized();
	} else {
		restoreGeometry(settings.value("geometry").toByteArray());
		restoreState(settings.value("windowState", 0).toByteArray());
	}

	setApplicationState((ApplicationState)settings.value("lastAppState", 0).toInt());
	settings.endGroup();
	show();
}

void MainWindow::readSettings()
{
	init_proxy();

	// now make sure that the cloud menu items are enabled IFF cloud account is verified
	enableDisableCloudActions();

	loadRecentFiles();
}

#undef TOOLBOX_PREF_BUTTON

void MainWindow::writeSettings()
{
	QSettings settings;

	settings.beginGroup("MainWindow");
	settings.setValue("geometry", saveGeometry());
	settings.setValue("windowState", saveState());
	settings.setValue("maximized", isMaximized());
	settings.setValue("lastState", (int)appState);
	saveSplitterSizes();
	settings.endGroup();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (inPlanner()) {
		on_actionQuit_triggered();
		event->ignore();
		return;
	}

	if (!Command::isClean() && (askSaveChanges() == false)) {
		event->ignore();
		return;
	}
	event->accept();
	writeSettings();
	QApplication::closeAllWindows();
}

void MainWindow::loadRecentFiles()
{
	recentFiles.clear();
	QSettings s;
	s.beginGroup("Recent_Files");
	foreach (const QString &key, s.childKeys()) {
		// TODO Sorting only correct up to 9 entries. Currently, only 4 used, so no problem.
		if (!key.startsWith("File_"))
			continue;
		QString file = s.value(key).toString();

		// never add our cloud URL to the recent files
		if (file.startsWith(prefs.cloud_base_url))
			continue;
		// but allow local git repos
		QRegularExpression gitrepo("(.*)\\[[^]]+]");
		QRegularExpressionMatch match = gitrepo.match(file);
		if (match.hasMatch()) {
			const QFileInfo gitDirectory(match.captured(1) + "/.git");
			if ((gitDirectory.exists()) && (gitDirectory.isDir()))
				recentFiles.append(file);
		} else if (QFile::exists(file)) {
			recentFiles.append(file);
		}
		if (recentFiles.count() > NUM_RECENT_FILES)
			break;
	}
	s.endGroup();
	updateRecentFilesMenu();
}

void MainWindow::updateRecentFilesMenu()
{
	for (int c = 0; c < NUM_RECENT_FILES; c++) {
		QAction *action = actionsRecent[c];

		if (recentFiles.count() > c) {
			QFileInfo fi(recentFiles.at(c));
			action->setText(fi.fileName());
			action->setToolTip(fi.absoluteFilePath());
			action->setVisible(true);
		} else {
			action->setVisible(false);
		}
	}
}

void MainWindow::addRecentFile(const QString &file, bool update)
{
	// never add Subsurface cloud file to the recent files - it has its own menu entry
	if (file.startsWith(prefs.cloud_base_url))
		return;
	QString localFile = QDir::toNativeSeparators(file);
	int index = recentFiles.indexOf(localFile);
	if (index >= 0)
		recentFiles.removeAt(index);
	recentFiles.prepend(localFile);
	while (recentFiles.count() > NUM_RECENT_FILES)
		recentFiles.removeLast();
	if (update)
		updateRecentFiles();
}

void MainWindow::updateRecentFiles()
{
	QSettings s;

	s.beginGroup("Recent_Files");
	s.remove("");	// Remove all old entries
	for (int c = 1; c <= recentFiles.count(); c++) {
		QString key = QString("File_%1").arg(c);
		s.setValue(key, recentFiles.at(c - 1));
	}
	s.endGroup();
	s.sync();
	updateRecentFilesMenu();
}

void MainWindow::recentFileTriggered(bool)
{
	if (!okToClose(tr("Please save or cancel the current dive edit before opening a new file.")))
		return;

	int filenr = ((QAction *)sender())->data().toInt();
	if (filenr >= recentFiles.count())
		return;
	const QString &filename = recentFiles[filenr];

	updateLastUsedDir(QFileInfo(filename).dir().path());
	closeCurrentFile();
	loadFiles(QStringList() << filename);
}

int MainWindow::file_save_as(void)
{
	QString filename;
	const char *default_filename = existing_filename;

	// if the default is to save to cloud storage, pick something that will work as local file:
	// simply extract the branch name which should be the users email address
	if (default_filename && QString(default_filename).contains(QRegularExpression(CLOUD_HOST_PATTERN))) {
		QString filename(default_filename);
		filename.remove(0, filename.indexOf("[") + 1);
		filename.replace("]", ".ssrf");
		default_filename = copy_qstring(filename);
	}
	// create a file dialog that allows us to save to a new file
	QFileDialog selection_dialog(this, tr("Save file as"), default_filename,
					 tr("Subsurface files") + " (*.ssrf *.xml)");
	selection_dialog.setAcceptMode(QFileDialog::AcceptSave);
	selection_dialog.setFileMode(QFileDialog::AnyFile);
	selection_dialog.setDefaultSuffix("");
	if (empty_string(default_filename)) {
		QFileInfo defaultFile(system_default_filename());
		selection_dialog.setDirectory(qPrintable(defaultFile.absolutePath()));
	}
	/* if the exit/cancel button is pressed return */
	if (!selection_dialog.exec())
		return 0;

	/* get the first selected file */
	filename = selection_dialog.selectedFiles().at(0);

	/* now for reasons I don't understand we appear to add a .ssrf to
	 * git style filenames <path>/directory[branch]
	 * so let's remove that */
	QRegularExpression reg(".*\\[[^]]+]\\.ssrf", QRegularExpression::CaseInsensitiveOption);
	if (reg.match(filename).hasMatch())
		filename.remove(QRegularExpression("\\.ssrf$", QRegularExpression::CaseInsensitiveOption));
	if (filename.isNull() || filename.isEmpty())
		return report_error("No filename to save into");

	if (save_dives(qPrintable(filename)))
		return -1;

	setCurrentFile(qPrintable(filename));
	Command::setClean();
	addRecentFile(filename, true);
	return 0;
}

int MainWindow::file_save(void)
{
	const char *current_default;
	bool is_cloud = false;

	if (!existing_filename)
		return file_save_as();

	is_cloud = (strncmp(existing_filename, "http", 4) == 0);
	if (is_cloud && !saveToCloudOK())
		return -1;

	current_default = prefs.default_filename;
	if (strcmp(existing_filename, current_default) == 0) {
		/* if we are using the default filename the directory
		 * that we are creating the file in may not exist */
		QDir current_def_dir = QFileInfo(current_default).absoluteDir();
		if (!current_def_dir.exists())
			current_def_dir.mkpath(current_def_dir.absolutePath());
	}
	if (is_cloud)
		showProgressBar();
	if (save_dives(existing_filename)) {
		if (is_cloud)
			hideProgressBar();
		return -1;
	}
	if (is_cloud)
		hideProgressBar();
	Command::setClean();
	addRecentFile(QString(existing_filename), true);
	return 0;
}

NotificationWidget *MainWindow::getNotificationWidget()
{
	return ui.mainErrorMessage;
}

QString MainWindow::displayedFilename(QString fullFilename)
{
	QFile f(fullFilename);
	QFileInfo fileInfo(f);
	QString fileName(fileInfo.fileName());

	if (fullFilename.contains(prefs.cloud_base_url)) {
		QString email = fileName.left(fileName.indexOf('['));
		return git_local_only ?
			tr("[local cache for] %1").arg(email) :
			tr("[cloud storage for] %1").arg(email);
	} else {
		return fileName;
	}
}


void MainWindow::setAutomaticTitle()
{
	setTitle();
}

void MainWindow::setTitle()
{
	if (empty_string(existing_filename)) {
		setWindowTitle("Subsurface");
		return;
	}

	QString unsaved = (!Command::isClean() ? " *" : "");
	QString shown = QString(" (%1)").arg(DiveFilter::instance()->shownText());
	setWindowTitle("Subsurface: " + displayedFilename(existing_filename) + unsaved + shown);
}

void MainWindow::importFiles(const QStringList &fileNames)
{
	if (fileNames.isEmpty())
		return;

	QByteArray fileNamePtr;
	struct divelog log;

	for (int i = 0; i < fileNames.size(); ++i) {
		fileNamePtr = QFile::encodeName(fileNames.at(i));
		parse_file(fileNamePtr.data(), &log);
	}
	QString source = fileNames.size() == 1 ? fileNames[0] : tr("multiple files");
	Command::importDives(&log, IMPORT_MERGE_ALL_TRIPS, source);
}

void MainWindow::loadFiles(const QStringList &fileNames)
{
	if (fileNames.isEmpty()) {
		refreshDisplay();
		return;
	}
	QByteArray fileNamePtr;

	showProgressBar();
	for (int i = 0; i < fileNames.size(); ++i) {
		fileNamePtr = QFile::encodeName(fileNames.at(i));
		if (!parse_file(fileNamePtr.data(), &divelog)) {
			setCurrentFile(fileNamePtr.data());
			addRecentFile(fileNamePtr, false);
		}
	}
	hideProgressBar();
	updateRecentFiles();
	process_loaded_dives();

	refreshDisplay();
	updateAutogroup();

	int min_datafile_version = get_min_datafile_version();
	if (min_datafile_version >0 && min_datafile_version < DATAFORMAT_VERSION) {
		QMessageBox::warning(this, tr("Opening datafile from older version"),
				     tr("You opened a data file from an older version of Subsurface. We recommend "
					"you read the manual to learn about the changes in the new version, especially "
					"about dive site management which has changed significantly.\n"
					"Subsurface has already tried to pre-populate the data but it might be worth "
					"while taking a look at the new dive site management system and to make "
					"sure that everything looks correct."));
	}
}

static const char *csvExtensions[] = {
	".csv", ".apd", ".zxu", ".zxl", ".txt"
};

static bool isCsvFile(const QString &s)
{
	for (const char *ext: csvExtensions) {
		if (s.endsWith(ext, Qt::CaseInsensitive))
			return true;
	}
	return false;
}

void MainWindow::on_actionImportDiveLog_triggered()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open dive log file"), lastUsedDir(), filter_import());

	if (fileNames.isEmpty())
		return;
	updateLastUsedDir(QFileInfo(fileNames[0]).dir().path());

	QStringList logFiles;
	QStringList csvFiles;
	for (const QString &fn: fileNames) {
		if (isCsvFile(fn))
			csvFiles.append(fn);
		else
			logFiles.append(fn);
	}

	if (logFiles.size())
		importFiles(logFiles);

	if (csvFiles.size()) {
		DiveLogImportDialog diveLogImport(std::move(csvFiles), this);
		diveLogImport.exec();
	}
}

void MainWindow::on_actionImportDiveSites_triggered()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open dive site file"), lastUsedDir(), filter_import_dive_sites());

	if (fileNames.isEmpty())
		return;
	updateLastUsedDir(QFileInfo(fileNames[0]).dir().path());

	struct divelog log;

	for (const QString &s: fileNames) {
		QByteArray fileNamePtr = QFile::encodeName(s);
		parse_file(fileNamePtr.data(), &log);
	}
	// The imported dive sites still have pointers to imported dives - remove them
	for (int i = 0; i < log.sites->nr; ++i)
		log.sites->dive_sites[i]->dives.nr = 0;

	QString source = fileNames.size() == 1 ? fileNames[0] : tr("multiple files");

	DivesiteImportDialog divesiteImport(*log.sites, source, this);
	divesiteImport.exec();
}

void MainWindow::on_actionExport_triggered()
{
	DiveLogExportDialog diveLogExport;
	diveLogExport.exec();
}

void MainWindow::on_actionConfigure_Dive_Computer_triggered()
{
	ConfigureDiveComputerDialog *dcConfig = new ConfigureDiveComputerDialog(this);
	dcConfig->show();
}

void MainWindow::setEnabledToolbar(bool arg1)
{
	profile->setEnabledToolbar(arg1);
}

void MainWindow::on_copy_triggered()
{
	// open dialog to select what gets copied
	// copy the displayed dive
	DiveComponentSelection dialog(this, &copyPasteDive, &what);
	dialog.exec();
}

void MainWindow::on_paste_triggered()
{
	Command::pasteDives(&copyPasteDive, what);
}

void MainWindow::on_actionFilterTags_triggered()
{
	if (!userMayChangeAppState())
		return;
	setApplicationState(appState == ApplicationState::FilterDive ? ApplicationState::Default : ApplicationState::FilterDive);
}

void MainWindow::on_actionStats_triggered()
{
	if (!userMayChangeAppState())
		return;
	setApplicationState(appState == ApplicationState::Statistics ? ApplicationState::Default : ApplicationState::Statistics);
}

void MainWindow::registerApplicationState(ApplicationState state, Quadrants q)
{
	applicationState[(int)state] = q;
}

void MainWindow::setQuadrantWidget(QSplitter &splitter, const Quadrant &q, int pos)
{
	if (!q.widget)
		return;
	if (splitter.count() > pos)
		splitter.replaceWidget(pos, q.widget);
	else
		splitter.addWidget(q.widget);
	splitter.setCollapsible(pos, false);
	q.widget->setEnabled(!(q.flags & FLAG_DISABLED));
}

void MainWindow::setQuadrantWidgets(QSplitter &splitter, const Quadrant &left, const Quadrant &right)
{
	int num = (left.widget != nullptr) + (right.widget != nullptr);
	// Remove superfluous widgets by reparenting to null.
	while (splitter.count() > num)
		splitter.widget(splitter.count() - 1)->setParent(nullptr);

	setQuadrantWidget(splitter, left, 0);
	setQuadrantWidget(splitter, right, left.widget != nullptr ? 1 : 0);
}

bool MainWindow::userMayChangeAppState() const
{
	return applicationState[(int)appState].allowUserChange;
}

// For the dive-site list view and the dive-site edit states,
// we remember the previous state and then switch back to that.
void MainWindow::enterPreviousState()
{
	if (state_stack.empty())
		setApplicationState(ApplicationState::Default);
	ApplicationState prev = state_stack.back();
	state_stack.pop_back();
	setApplicationState(prev);
}

void MainWindow::setApplicationState(ApplicationState state)
{
	if (appState == state)
		return;

	saveSplitterSizes();

	appState = state;

	clearSplitter(*topSplitter);
	clearSplitter(*bottomSplitter);
	const Quadrants &quadrants = applicationState[(int)state];
	setQuadrantWidgets(*topSplitter, quadrants.topLeft, quadrants.topRight);
	setQuadrantWidgets(*bottomSplitter, quadrants.bottomLeft, quadrants.bottomRight);

	if (topSplitter->count() >= 1) {
		// Add topSplitter if it is not already shown
		if (ui.mainSplitter->count() == 0 ||
		    ui.mainSplitter->widget(0) != topSplitter.get()) {
			ui.mainSplitter->insertWidget(0, topSplitter.get());
			ui.mainSplitter->setCollapsible(ui.mainSplitter->count() - 1, false);
		}
	} else {
		// Remove topSplitter by reparenting it. So weird.
		topSplitter->setParent(nullptr);
	}

	if (bottomSplitter->count() >= 1) {
		// Add bottomSplitter if it is not already shown
		if (ui.mainSplitter->count() == 0 ||
		    ui.mainSplitter->widget(ui.mainSplitter->count() - 1) != bottomSplitter.get()) {
			ui.mainSplitter->addWidget(bottomSplitter.get());
			ui.mainSplitter->setCollapsible(ui.mainSplitter->count() - 1, false);
		}
	} else {
		// Remove bottomSplitter by reparenting it. So weird.
		bottomSplitter->setParent(nullptr);
	}

	restoreSplitterSizes();

	bool allowChange = userMayChangeAppState();
	ui.actionViewAll->setEnabled(allowChange);
	ui.actionViewList->setEnabled(allowChange);
	ui.actionViewProfile->setEnabled(allowChange);
	ui.actionViewInfo->setEnabled(allowChange);
	ui.actionViewMap->setEnabled(allowChange);
	ui.actionViewDiveSites->setEnabled(allowChange);
	ui.actionFilterTags->setEnabled(allowChange);
}

void MainWindow::showProgressBar()
{
	delete progressDialog;

	progressDialog = new QProgressDialog(tr("Contacting cloud service..."), tr("Cancel"), 0, 100, this);
	progressDialog->setWindowModality(Qt::WindowModal);
	progressDialog->setMinimumDuration(0);
	progressDialogCanceled = false;
	progressCounter = 0;
	connect(progressDialog, SIGNAL(canceled()), this, SLOT(cancelCloudStorageOperation()));
}

void MainWindow::cancelCloudStorageOperation()
{
	progressDialogCanceled = true;
}

void MainWindow::hideProgressBar()
{
	if (progressDialog) {
		progressDialog->setValue(100);
		delete progressDialog;
		progressDialog = nullptr;
	}
}

void MainWindow::divesChanged(const QVector<dive *> &dives, DiveField)
{
	for (struct dive *d: dives) {
		qDebug() << "dive #" << d->number << "changed, cache is" << (dive_cache_is_valid(d) ? "valid" : "invalidated");
		// a brute force way to deal with that would of course be to call
		// invalidate_dive_cache(d);
	}
}
