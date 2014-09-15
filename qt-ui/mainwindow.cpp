/*
 * mainwindow.cpp
 *
 * classes for the main UI window in Subsurface
 */
#include "mainwindow.h"

#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QtDebug>
#include <QDateTime>
#include <QSettings>
#include <QCloseEvent>
#include <QApplication>
#include <QFontMetrics>
#include <QTableView>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QStringList>
#include <QSettings>
#include <QShortcut>
#include <QToolBar>
#include <fcntl.h>
#include "divelistview.h"
#include "starwidget.h"
#include "ssrf-version.h"
#include "dive.h"
#include "display.h"
#include "divelist.h"
#include "pref.h"
#include "helpers.h"
#include "modeldelegates.h"
#include "models.h"
#include "downloadfromdivecomputer.h"
#include "preferences.h"
#include "subsurfacewebservices.h"
#include "divecomputermanagementdialog.h"
#include "simplewidgets.h"
#include "diveplanner.h"
#include "about.h"
#include "worldmap-save.h"
#include "updatemanager.h"
#include "planner.h"
#include "configuredivecomputerdialog.h"
#include "statistics/statisticswidget.h"
#ifndef NO_PRINTING
#include <QPrintDialog>
#include "printdialog.h"
#endif
#include "divelogimportdialog.h"
#include "divelogexportdialog.h"
#include "usersurvey.h"
#ifndef NO_USERMANUAL
#include "usermanual.h"
#endif
#include <QNetworkProxy>

MainWindow *MainWindow::m_Instance = NULL;

MainWindow::MainWindow() : QMainWindow(),
	actionNextDive(0),
	actionPreviousDive(0),
	helpView(0),
	state(VIEWALL),
	updateManager(0),
	survey(0)
{
	Q_ASSERT_X(m_Instance == NULL, "MainWindow", "MainWindow recreated!");
	m_Instance = this;
	ui.setupUi(this);
	ui.tagFilter->hide();
	profileToolbarActions << ui.profCalcAllTissues << ui.profCalcCeiling << ui.profDcCeiling << ui.profEad <<
		    ui.profHR << ui.profIncrement3m << ui.profMod << ui.profNdl_tts << ui.profNdl_tts <<
		    ui.profPhe << ui.profPn2 << ui.profPO2 << ui.profRuler << ui.profSAC << ui.profScaled <<
		    ui.profTogglePicture << ui.profTankbar << ui.profTissues;
	setWindowIcon(QIcon(":subsurface-icon"));
	if (!QIcon::hasThemeIcon("window-close")) {
		QIcon::setThemeName("subsurface");
	}
	connect(ui.ListWidget, SIGNAL(currentDiveChanged(int)), this, SLOT(current_dive_changed(int)));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), this, SLOT(readSettings()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), ui.ListWidget, SLOT(update()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), ui.ListWidget, SLOT(reloadHeaderActions()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), ui.InfoWidget, SLOT(updateDiveInfo()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), ui.divePlannerWidget, SLOT(settingsChanged()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), ui.plannerSettingsWidget, SLOT(settingsChanged()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), TankInfoModel::instance(), SLOT(update()));
	connect(ui.actionRecent1, SIGNAL(triggered(bool)), this, SLOT(recentFileTriggered(bool)));
	connect(ui.actionRecent2, SIGNAL(triggered(bool)), this, SLOT(recentFileTriggered(bool)));
	connect(ui.actionRecent3, SIGNAL(triggered(bool)), this, SLOT(recentFileTriggered(bool)));
	connect(ui.actionRecent4, SIGNAL(triggered(bool)), this, SLOT(recentFileTriggered(bool)));
	connect(information(), SIGNAL(addDiveFinished()), ui.newProfile, SLOT(setProfileState()));
	connect(DivePlannerPointsModel::instance(), SIGNAL(planCreated()), this, SLOT(planCreated()));
	connect(DivePlannerPointsModel::instance(), SIGNAL(planCanceled()), this, SLOT(planCanceled()));
	connect(ui.printPlan, SIGNAL(pressed()), ui.divePlannerWidget, SLOT(printDecoPlan()));
#ifdef NO_PRINTING
	ui.printPlan->hide();
#endif

	ui.mainErrorMessage->hide();
	ui.newProfile->setEmptyState();
	initialUiSetup();
	readSettings();
	ui.ListWidget->reload(DiveTripModel::TREE);
	ui.ListWidget->reloadHeaderActions();
	ui.ListWidget->setFocus();
	ui.globe->reload();
	ui.ListWidget->expand(ui.ListWidget->model()->index(0, 0));
	ui.ListWidget->scrollTo(ui.ListWidget->model()->index(0, 0), QAbstractItemView::PositionAtCenter);
	ui.divePlannerWidget->settingsChanged();
	ui.plannerSettingsWidget->settingsChanged();
#ifdef NO_MARBLE
	ui.globePane->hide();
	ui.menuView->removeAction(ui.actionViewGlobe);
#endif
#ifdef NO_USERMANUAL
	ui.menuHelp->removeAction(ui.actionUserManual);
#endif
#ifdef NO_PRINTING
	ui.menuFile->removeAction(ui.actionPrint);
#endif
	memset(&copyPasteDive, 0, sizeof(copyPasteDive));
	memset(&what, 0, sizeof(what));

	QToolBar *toolBar = new QToolBar();
	Q_FOREACH (QAction *a, profileToolbarActions)
		toolBar->addAction(a);
	toolBar->setOrientation(Qt::Vertical);

	// since I'm adding the toolBar by hand, because designer
	// has no concept of "toolbar" for a non-mainwindow widget (...)
	// I need to take the current item that's in the toolbar Position
	// and reposition it alongside the grid layout.
	QLayoutItem *p = ui.gridLayout->takeAt(0);
	ui.gridLayout->addWidget(toolBar, 0, 0);
	ui.gridLayout->addItem(p, 0, 1);
}

MainWindow::~MainWindow()
{
	m_Instance = NULL;
}

void MainWindow::setLoadedWithFiles(bool f)
{
	filesAsArguments = f;
}

bool MainWindow::filesFromCommandLine() const
{
	return filesAsArguments;
}

MainWindow *MainWindow::instance()
{
	return m_Instance;
}

// this gets called after we download dives from a divecomputer
void MainWindow::refreshDisplay(bool doRecreateDiveList)
{
	showError(get_error_string());
	ui.InfoWidget->reload();
	TankInfoModel::instance()->update();
	ui.globe->reload();
	if (doRecreateDiveList)
		recreateDiveList();
	ui.diveListPane->setCurrentIndex(0); // switch to the dive list
#ifdef NO_MARBLE
	ui.globePane->hide();
#endif
	ui.globePane->setCurrentIndex(0);
	ui.ListWidget->setEnabled(true);
	ui.ListWidget->setFocus();
	WSInfoModel::instance()->updateInfo();
	if (amount_selected == 0)
		cleanUpEmpty();
}

void MainWindow::recreateDiveList()
{
	ui.ListWidget->reload(DiveTripModel::CURRENT);
	TagFilterModel::instance()->repopulate();
}

void MainWindow::current_dive_changed(int divenr)
{
	if (divenr >= 0) {
		select_dive(divenr);
		ui.globe->centerOnCurrentDive();
	}
	ui.newProfile->plotDive();
	ui.InfoWidget->updateDiveInfo();
}

void MainWindow::on_actionNew_triggered()
{
	on_actionClose_triggered();
}

void MainWindow::on_actionOpen_triggered()
{
	if (!okToClose(tr("Please save or cancel the current dive edit before opening a new file.")))
		return;

	QString filename = QFileDialog::getOpenFileName(this, tr("Open file"), lastUsedDir(), filter());
	if (filename.isEmpty())
		return;
	updateLastUsedDir(QFileInfo(filename).dir().path());
	closeCurrentFile();
	loadFiles(QStringList() << filename);
}

void MainWindow::on_actionSave_triggered()
{
	file_save();
}

void MainWindow::on_actionSaveAs_triggered()
{
	file_save_as();
}

ProfileWidget2 *MainWindow::graphics() const
{
	return ui.newProfile;
}

void MainWindow::cleanUpEmpty()
{
	ui.InfoWidget->clearStats();
	ui.InfoWidget->clearInfo();
	ui.InfoWidget->clearEquipment();
	ui.InfoWidget->updateDiveInfo(true);
	ui.newProfile->setEmptyState();
	ui.ListWidget->reload(DiveTripModel::TREE);
	ui.globe->reload();
	if (!existing_filename)
		setTitle(MWTF_DEFAULT);
	disableDcShortcuts();
}

bool MainWindow::okToClose(QString message)
{
	if (DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING ||
	    ui.InfoWidget->isEditing()) {
		QMessageBox::warning(this, tr("Warning"), message);
		return false;
	}
	if (unsaved_changes() && askSaveChanges() == false)
		return false;

	return true;
}

void MainWindow::closeCurrentFile()
{
	ui.newProfile->setEmptyState();
	/* free the dives and trips */
	clear_git_id();
	while (dive_table.nr)
		delete_single_dive(0);

	free((void *)existing_filename);
	existing_filename = NULL;

	cleanUpEmpty();
	mark_divelist_changed(false);

	clear_events();
}

void MainWindow::on_actionClose_triggered()
{
	if (okToClose(tr("Please save or cancel the current dive edit before closing the file.")))
		closeCurrentFile();
}

QString MainWindow::lastUsedDir()
{
	QSettings settings;
	QString lastDir = QDir::homePath();

	settings.beginGroup("FileDialog");
	if (settings.contains("LastDir"))
		if (QDir::setCurrent(settings.value("LastDir").toString()))
			lastDir = settings.value("LastDir").toString();
	return lastDir;
}

void MainWindow::updateLastUsedDir(const QString &dir)
{
	QSettings s;
	s.beginGroup("FileDialog");
	s.setValue("LastDir", dir);
}

void MainWindow::on_actionPrint_triggered()
{
#ifndef NO_PRINTING
	PrintDialog dlg(this);

	dlg.exec();
#endif
}

void MainWindow::disableDcShortcuts()
{
	ui.actionPreviousDC->setShortcut(QKeySequence());
	ui.actionNextDC->setShortcut(QKeySequence());
}

void MainWindow::enableDcShortcuts()
{
	ui.actionPreviousDC->setShortcut(Qt::Key_Left);
	ui.actionNextDC->setShortcut(Qt::Key_Right);
}

void MainWindow::showProfile()
{
	enableDcShortcuts();
	ui.newProfile->setProfileState();
	ui.infoPane->setCurrentIndex(MAINTAB);
}

void MainWindow::on_actionPreferences_triggered()
{
	PreferencesDialog::instance()->show();
}

void MainWindow::on_actionQuit_triggered()
{
	if (ui.InfoWidget->isEditing()) {
		ui.InfoWidget->rejectChanges();
		if (ui.InfoWidget->isEditing())
			// didn't discard the edits
			return;
	}
	if (DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING) {
		DivePlannerPointsModel::instance()->cancelPlan();
		if (DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING)
			// The planned dive was not discarded
			return;
	}

	if (unsaved_changes() && (askSaveChanges() == false))
		return;
	writeSettings();
	QApplication::quit();
}

void MainWindow::on_actionDownloadDC_triggered()
{
	DownloadFromDCWidget dlg(this);

	dlg.exec();
}

void MainWindow::on_actionDownloadWeb_triggered()
{
	SubsurfaceWebServices dlg(this);

	dlg.exec();
}

void MainWindow::on_actionDivelogs_de_triggered()
{
	DivelogsDeWebServices::instance()->downloadDives();
}

void MainWindow::on_actionEditDeviceNames_triggered()
{
	DiveComputerManagementDialog::instance()->init();
	DiveComputerManagementDialog::instance()->update();
	DiveComputerManagementDialog::instance()->show();
}

bool MainWindow::plannerStateClean()
{
	if (DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING ||
	    ui.InfoWidget->isEditing()) {
		QMessageBox::warning(this, tr("Warning"), tr("Please save or cancel the current dive edit before trying to add a dive."));
		return false;
	}
	return true;
}

void MainWindow::planCanceled()
{
	// while planning we might have modified the displayed_dive
	// let's refresh what's shown on the profile
	showProfile();
	ui.newProfile->replot();
	refreshDisplay(false);
	ui.newProfile->plotDive(get_dive(selected_dive));
	DivePictureModel::instance()->updateDivePictures();
}

void MainWindow::planCreated()
{
	// get the new dive selected and assign a number if reasonable
	ui.newProfile->setProfileState();
	if (displayed_dive.id == 0) {
		// we might have added a new dive (so displayed_dive was cleared out by clone_dive()
		dive_list()->unselectDives();
		select_dive(dive_table.nr - 1);
		dive_list()->selectDive(selected_dive);
		set_dive_nr_for_current_dive();
	}
	showProfile();
	refreshDisplay();
}

void MainWindow::setPlanNotes(const char *notes)
{
	ui.divePlanOutput->setHtml(notes);
}

void MainWindow::printPlan()
{
#ifndef NO_PRINTING
	QString diveplan = ui.divePlanOutput->toHtml();
	QString withDisclaimer = QString("<img height=50 src=\":subsurface-icon\"> ") + diveplan + QString(disclaimer);

	QPrinter printer;
	QPrintDialog *dialog = new QPrintDialog(&printer, this);
	dialog->setWindowTitle(tr("Print runtime table"));
	if (dialog->exec() != QDialog::Accepted)
		return;

	ui.divePlanOutput->setHtml(withDisclaimer);
	ui.divePlanOutput->print(&printer);
	ui.divePlanOutput->setHtml(diveplan);
#endif
}

void MainWindow::setupForAddAndPlan(const char *model)
{
	// clean out the dive and give it an id and the correct dc model
	clear_dive(&displayed_dive);
	displayed_dive.id = dive_getUniqID(&displayed_dive);
	displayed_dive.when = QDateTime::currentMSecsSinceEpoch() / 1000L + gettimezoneoffset() + 3600;
	displayed_dive.dc.model = model; // don't translate! this is stored in the XML file
	// setup the dive cylinders
	DivePlannerPointsModel::instance()->clear();
	DivePlannerPointsModel::instance()->setupCylinders();
}

void MainWindow::on_actionReplanDive_triggered()
{
	if (!plannerStateClean())
		return;
	if (!current_dive || strcmp(current_dive->dc.model, "planned dive")) {
		qDebug() << current_dive->dc.model;
		return;
	}
	ui.ListWidget->endSearch();
	// put us in PLAN mode
	DivePlannerPointsModel::instance()->clear();
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::PLAN);

	ui.newProfile->setPlanState();
	ui.newProfile->clearHandlers();
	ui.infoPane->setCurrentIndex(PLANNERWIDGET);
	DivePlannerPointsModel::instance()->loadFromDive(current_dive);
	reset_cylinders(&displayed_dive, true);
	ui.diveListPane->setCurrentIndex(1); // switch to the plan output
	ui.globePane->setCurrentIndex(1);
#ifdef NO_MARBLE
	ui.globePane->show();
#endif
}

void MainWindow::on_actionDivePlanner_triggered()
{
	if (!plannerStateClean())
		return;

	ui.ListWidget->endSearch();
	// put us in PLAN mode
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::PLAN);

	ui.newProfile->setPlanState();
	ui.infoPane->setCurrentIndex(PLANNERWIDGET);

	// create a simple starting dive, using the first gas from the just copied cylidners
	setupForAddAndPlan("planned dive"); // don't translate, stored in XML file
	DivePlannerPointsModel::instance()->setupStartTime();
	DivePlannerPointsModel::instance()->createSimpleDive();
	DivePictureModel::instance()->updateDivePictures();

	ui.diveListPane->setCurrentIndex(1); // switch to the plan output
	ui.globePane->setCurrentIndex(1);
#ifdef NO_MARBLE
	ui.globePane->show();
#endif
}

void MainWindow::on_actionAddDive_triggered()
{
	if (!plannerStateClean())
		return;

	if (dive_list()->selectedTrips().count() >= 1) {
		dive_list()->rememberSelection();
		dive_list()->clearSelection();
	}

	ui.ListWidget->endSearch();
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::ADD);

	// setup things so we can later create our starting dive
	setupForAddAndPlan("manually added dive"); // don't translate, stored in the XML file

	// now show the mostly empty main tab
	ui.InfoWidget->updateDiveInfo();

	// show main tab
	ui.InfoWidget->setCurrentIndex(0);

	ui.InfoWidget->addDiveStarted();
	ui.infoPane->setCurrentIndex(MAINTAB);

	ui.newProfile->setAddState();
	DivePlannerPointsModel::instance()->createSimpleDive();
	ui.newProfile->plotDive();
}

void MainWindow::on_actionRenumber_triggered()
{
	RenumberDialog::instance()->renumberOnlySelected(false);
	RenumberDialog::instance()->show();
}

void MainWindow::on_actionAutoGroup_triggered()
{
	autogroup = ui.actionAutoGroup->isChecked();
	if (autogroup)
		autogroup_dives();
	else
		remove_autogen_trips();
	refreshDisplay();
	mark_divelist_changed(true);
}

void MainWindow::on_actionYearlyStatistics_triggered()
{
	QDialog d;
	YearlyStatisticsWidget *newView = new YearlyStatisticsWidget();
	QVBoxLayout *l = new QVBoxLayout(&d);
	l->addWidget(newView);
	YearlyStatisticsModel *m = new YearlyStatisticsModel();
	QTreeView *view = new QTreeView();
	view->setModel(m);
	newView->setModel(m);
	l->addWidget(view);
	d.exec();
}

#define BEHAVIOR QList<int>()
void MainWindow::on_actionViewList_triggered()
{
	beginChangeState(LIST_MAXIMIZED);
	ui.listGlobeSplitter->setSizes(BEHAVIOR << EXPANDED << COLLAPSED);
	ui.mainSplitter->setSizes(BEHAVIOR << COLLAPSED << EXPANDED);
}

void MainWindow::on_actionViewProfile_triggered()
{
	beginChangeState(PROFILE_MAXIMIZED);
	ui.infoProfileSplitter->setSizes(BEHAVIOR << COLLAPSED << EXPANDED);
	ui.mainSplitter->setSizes(BEHAVIOR << EXPANDED << COLLAPSED);
}

void MainWindow::on_actionViewInfo_triggered()
{
	beginChangeState(INFO_MAXIMIZED);
	ui.infoProfileSplitter->setSizes(BEHAVIOR << EXPANDED << COLLAPSED);
	ui.mainSplitter->setSizes(BEHAVIOR << EXPANDED << COLLAPSED);
}

void MainWindow::on_actionViewGlobe_triggered()
{
	beginChangeState(GLOBE_MAXIMIZED);
	ui.mainSplitter->setSizes(BEHAVIOR << COLLAPSED << EXPANDED);
	ui.listGlobeSplitter->setSizes(BEHAVIOR << COLLAPSED << EXPANDED);
}
#undef BEHAVIOR

void MainWindow::on_actionViewAll_triggered()
{
	beginChangeState(VIEWALL);
	static QList<int> mainSizes;
	const int appH = qApp->desktop()->size().height();
	const int appW = qApp->desktop()->size().width();
	if (mainSizes.empty()) {
		mainSizes.append(appH * 0.7);
		mainSizes.append(appH * 0.3);
	}
	static QList<int> infoProfileSizes;
	if (infoProfileSizes.empty()) {
		infoProfileSizes.append(appW * 0.3);
		infoProfileSizes.append(appW * 0.7);
	}

	static QList<int> listGlobeSizes;
	if (listGlobeSizes.empty()) {
		listGlobeSizes.append(appW * 0.7);
		listGlobeSizes.append(appW * 0.3);
	}

	QSettings settings;
	settings.beginGroup("MainWindow");
	if (settings.value("mainSplitter").isValid()) {
		ui.mainSplitter->restoreState(settings.value("mainSplitter").toByteArray());
		ui.infoProfileSplitter->restoreState(settings.value("infoProfileSplitter").toByteArray());
		ui.listGlobeSplitter->restoreState(settings.value("listGlobeSplitter").toByteArray());
		if (ui.mainSplitter->sizes().first() == 0 || ui.mainSplitter->sizes().last() == 0)
			ui.mainSplitter->setSizes(mainSizes);
		if (ui.infoProfileSplitter->sizes().first() == 0 || ui.infoProfileSplitter->sizes().last() == 0)
			ui.infoProfileSplitter->setSizes(infoProfileSizes);
		if (ui.listGlobeSplitter->sizes().first() == 0 || ui.listGlobeSplitter->sizes().last() == 0)
			ui.listGlobeSplitter->setSizes(listGlobeSizes);

	} else {
		ui.mainSplitter->setSizes(mainSizes);
		ui.infoProfileSplitter->setSizes(infoProfileSizes);
		ui.listGlobeSplitter->setSizes(listGlobeSizes);
	}
}

void MainWindow::beginChangeState(CurrentState s)
{
	if (state == VIEWALL && state != s) {
		saveSplitterSizes();
	}
	state = s;
}

void MainWindow::saveSplitterSizes()
{
	QSettings settings;
	settings.beginGroup("MainWindow");
	settings.setValue("mainSplitter", ui.mainSplitter->saveState());
	settings.setValue("infoProfileSplitter", ui.infoProfileSplitter->saveState());
	settings.setValue("listGlobeSplitter", ui.listGlobeSplitter->saveState());
}

void MainWindow::on_actionPreviousDC_triggered()
{
	unsigned nrdc = number_of_computers(current_dive);
	dc_number = (dc_number + nrdc - 1) % nrdc;
	ui.newProfile->plotDive();
	ui.InfoWidget->updateDiveInfo();
}

void MainWindow::on_actionNextDC_triggered()
{
	unsigned nrdc = number_of_computers(current_dive);
	dc_number = (dc_number + 1) % nrdc;
	ui.newProfile->plotDive();
	ui.InfoWidget->updateDiveInfo();
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
	if (!helpView) {
		helpView = new UserManual();
	}
	helpView->show();
#endif
}

QString MainWindow::filter()
{
	QString f;
	f += "ALL ( *.ssrf *.xml *.XML *.uddf *.udcf *.UDFC *.jlb *.JLB ";
	f += "*.sde *.SDE *.dld *.DLD ";
	f += "*.db";
	f += ");;";

	f += "Subsurface (*.ssrf);;";
	f += "XML (*.xml *.XML);;";
	f += "UDDF (*.uddf);;";
	f += "UDCF (*.udcf *.UDCF);;";
	f += "JLB  (*.jlb *.JLB);;";

	f += "SDE (*.sde *.SDE);;";
	f += "DLD (*.dld *.DLD);;";
	f += "DB (*.db)";

	return f;
}

bool MainWindow::askSaveChanges()
{
	QString message;
	QMessageBox response(this);

	if (existing_filename)
		message = tr("Do you want to save the changes that you made in the file %1?").arg(existing_filename);
	else
		message = tr("Do you want to save the changes that you made in the data file?");

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
	QSize sz = settings.value("size", qApp->desktop()->size()).value<QSize>();
	if (settings.value("maximized", isMaximized()).value<bool>())
		showMaximized();
	else
		resize(sz);

	state = (CurrentState)settings.value("lastState", 0).toInt();
	switch (state) {
	case VIEWALL:
		on_actionViewAll_triggered();
		break;
	case GLOBE_MAXIMIZED:
		on_actionViewGlobe_triggered();
		break;
	case INFO_MAXIMIZED:
		on_actionViewInfo_triggered();
		break;
	case LIST_MAXIMIZED:
		on_actionViewList_triggered();
		break;
	case PROFILE_MAXIMIZED:
		on_actionViewProfile_triggered();
		break;
	}
	settings.endGroup();
}

const char *getSetting(QSettings &s, QString name)
{
	QVariant v;
	v = s.value(name);
	if (v.isValid()) {
		return strdup(v.toString().toUtf8().data());
	}
	return NULL;
}

#define TOOLBOX_PREF_BUTTON(pref, setting, button) \
	prefs.pref = s.value(#setting).toBool();   \
	ui.button->setChecked(prefs.pref);

void MainWindow::readSettings()
{
	static bool firstRun = true;
	QSettings s;
	// the static object for preferences already reads in the settings
	// and sets up the font, so just get what we need for the toolbox and other widgets here

	s.beginGroup("TecDetails");
	TOOLBOX_PREF_BUTTON(calcalltissues, calcalltissues, profCalcAllTissues);
	TOOLBOX_PREF_BUTTON(calcceiling, calcceiling, profCalcCeiling);
	TOOLBOX_PREF_BUTTON(dcceiling, dcceiling, profDcCeiling);
	TOOLBOX_PREF_BUTTON(ead, ead, profEad);
	TOOLBOX_PREF_BUTTON(calcceiling3m, calcceiling3m, profIncrement3m);
	TOOLBOX_PREF_BUTTON(mod, mod, profMod);
	TOOLBOX_PREF_BUTTON(calcndltts, calcndltts, profNdl_tts);
	TOOLBOX_PREF_BUTTON(pp_graphs.phe, phegraph, profPhe);
	TOOLBOX_PREF_BUTTON(pp_graphs.pn2, pn2graph, profPn2);
	TOOLBOX_PREF_BUTTON(pp_graphs.po2, po2graph, profPO2);
	TOOLBOX_PREF_BUTTON(hrgraph, hrgraph, profHR);
	TOOLBOX_PREF_BUTTON(rulergraph, rulergraph, profRuler);
	TOOLBOX_PREF_BUTTON(show_sac, show_sac, profSAC);
	TOOLBOX_PREF_BUTTON(show_pictures_in_profile, show_pictures_in_profile, profTogglePicture);
	TOOLBOX_PREF_BUTTON(tankbar, tankbar, profTankbar);
	TOOLBOX_PREF_BUTTON(percentagegraph, precentagegraph, profTissues);
	s.endGroup();
	s.beginGroup("DiveComputer");
	default_dive_computer_vendor = getSetting(s, "dive_computer_vendor");
	default_dive_computer_product = getSetting(s, "dive_computer_product");
	default_dive_computer_device = getSetting(s, "dive_computer_device");
	s.endGroup();
	QNetworkProxy proxy;
	proxy.setType(QNetworkProxy::ProxyType(prefs.proxy_type));
	proxy.setHostName(prefs.proxy_host);
	proxy.setPort(prefs.proxy_port);
	if (prefs.proxy_auth) {
		proxy.setUser(prefs.proxy_user);
		proxy.setPassword(prefs.proxy_pass);
	}
	QNetworkProxy::setApplicationProxy(proxy);

	loadRecentFiles(&s);
	if (firstRun) {
		checkSurvey(&s);
		firstRun = false;
	}
}

#undef TOOLBOX_PREF_BUTTON

void MainWindow::checkSurvey(QSettings *s)
{
	s->beginGroup("UserSurvey");
	if (!s->contains("FirstUse42")) {
		QVariant value = QDate().currentDate();
		s->setValue("FirstUse42", value);
	}
	// wait a week for production versions, but not at all for non-tagged builds
	QString ver(VERSION_STRING);
	int waitTime = ver.contains('-') ? -1 : 7;
	QDate firstUse42 = s->value("FirstUse42").toDate();
	if (run_survey || (firstUse42.daysTo(QDate().currentDate()) > waitTime && !s->contains("SurveyDone"))) {
		if (!survey)
			survey = new UserSurvey(this);
		survey->show();
	}
	s->endGroup();
}

void MainWindow::writeSettings()
{
	QSettings settings;

	settings.beginGroup("MainWindow");
	settings.setValue("lastState", (int)state);
	settings.setValue("maximized", isMaximized());
	if (!isMaximized())
		settings.setValue("size", size());
	if (state == VIEWALL)
		saveSplitterSizes();
	settings.endGroup();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING ||
	    ui.InfoWidget->isEditing()) {
		on_actionQuit_triggered();
		event->ignore();
		return;
	}

#ifndef NO_USERMANUAL
	if (helpView && helpView->isVisible()) {
		helpView->close();
		helpView->deleteLater();
	}
#endif

	if (unsaved_changes() && (askSaveChanges() == false)) {
		event->ignore();
		return;
	}
	event->accept();
	writeSettings();
	QApplication::closeAllWindows();
}

DiveListView *MainWindow::dive_list()
{
	return ui.ListWidget;
}

GlobeGPS *MainWindow::globe()
{
	return ui.globe;
}

MainTab *MainWindow::information()
{
	return ui.InfoWidget;
}

void MainWindow::loadRecentFiles(QSettings *s)
{
	QStringList files;
	bool modified = false;

	s->beginGroup("Recent_Files");
	for (int c = 1; c <= 4; c++) {
		QString key = QString("File_%1").arg(c);
		if (s->contains(key)) {
			QString file = s->value(key).toString();

			if (QFile::exists(file)) {
				files.append(file);
			} else {
				modified = true;
			}
		} else {
			break;
		}
	}

	if (modified) {
		for (int c = 0; c < 4; c++) {
			QString key = QString("File_%1").arg(c + 1);

			if (files.count() > c) {
				s->setValue(key, files.at(c));
			} else {
				if (s->contains(key)) {
					s->remove(key);
				}
			}
		}

		s->sync();
	}
	s->endGroup();

	for (int c = 0; c < 4; c++) {
		QAction *action = this->findChild<QAction *>(QString("actionRecent%1").arg(c + 1));

		if (files.count() > c) {
			QFileInfo fi(files.at(c));
			action->setText(fi.fileName());
			action->setToolTip(fi.absoluteFilePath());
			action->setVisible(true);
		} else {
			action->setVisible(false);
		}
	}
}

void MainWindow::addRecentFile(const QStringList &newFiles)
{
	QStringList files;
	QSettings s;

	if (newFiles.isEmpty())
		return;

	s.beginGroup("Recent_Files");

	for (int c = 1; c <= 4; c++) {
		QString key = QString("File_%1").arg(c);
		if (s.contains(key)) {
			QString file = s.value(key).toString();

			files.append(file);
		} else {
			break;
		}
	}

	foreach (const QString &file, newFiles) {
		int index = files.indexOf(QDir::toNativeSeparators(file));

		if (index >= 0) {
			files.removeAt(index);
		}
	}

	foreach (const QString &file, newFiles) {
		if (QFile::exists(file)) {
			files.prepend(QDir::toNativeSeparators(file));
		}
	}

	while (files.count() > 4) {
		files.removeLast();
	}

	for (int c = 1; c <= 4; c++) {
		QString key = QString("File_%1").arg(c);

		if (files.count() >= c) {
			s.setValue(key, files.at(c - 1));
		} else {
			if (s.contains(key)) {
				s.remove(key);
			}
		}
	}
	s.endGroup();
	s.sync();

	loadRecentFiles(&s);
}

void MainWindow::removeRecentFile(QStringList failedFiles)
{
	QStringList files;
	QSettings s;

	if (failedFiles.isEmpty())
		return;

	s.beginGroup("Recent_Files");

	for (int c = 1; c <= 4; c++) {
		QString key = QString("File_%1").arg(c);

		if (s.contains(key)) {
			QString file = s.value(key).toString();
			files.append(file);
		} else {
			break;
		}
	}

	foreach (const QString &file, failedFiles)
		files.removeAll(file);

	for (int c = 1; c <= 4; c++) {
		QString key = QString("File_%1").arg(c);

		if (files.count() >= c)
			s.setValue(key, files.at(c - 1));
		else if (s.contains(key))
			s.remove(key);
	}

	s.endGroup();
	s.sync();

	loadRecentFiles(&s);
}

void MainWindow::recentFileTriggered(bool checked)
{
	Q_UNUSED(checked);

	if (!okToClose(tr("Please save or cancel the current dive edit before opening a new file.")))
		return;

	QAction *actionRecent = (QAction *)sender();

	const QString &filename = actionRecent->toolTip();

	updateLastUsedDir(QFileInfo(filename).dir().path());
	closeCurrentFile();
	loadFiles(QStringList() << filename);
}

int MainWindow::file_save_as(void)
{
	QString filename;
	const char *default_filename = existing_filename;
	filename = QFileDialog::getSaveFileName(this, tr("Save file as"), default_filename,
						tr("Subsurface XML files (*.ssrf *.xml *.XML)"));
	if (filename.isNull() || filename.isEmpty())
		return report_error("No filename to save into");

	if (ui.InfoWidget->isEditing())
		ui.InfoWidget->acceptChanges();

	if (save_dives(filename.toUtf8().data())) {
		showError(get_error_string());
		return -1;
	}

	showError(get_error_string());
	set_filename(filename.toUtf8().data(), true);
	setTitle(MWTF_FILENAME);
	mark_divelist_changed(false);
	addRecentFile(QStringList() << filename);
	return 0;
}

int MainWindow::file_save(void)
{
	const char *current_default;

	if (!existing_filename)
		return file_save_as();

	if (ui.InfoWidget->isEditing())
		ui.InfoWidget->acceptChanges();

	current_default = prefs.default_filename;
	if (strcmp(existing_filename, current_default) == 0) {
		/* if we are using the default filename the directory
		 * that we are creating the file in may not exist */
		QDir current_def_dir = QFileInfo(current_default).absoluteDir();
		if (!current_def_dir.exists())
			current_def_dir.mkpath(current_def_dir.absolutePath());
	}
	if (save_dives(existing_filename)) {
		showError(get_error_string());
		return -1;
	}
	showError(get_error_string());
	mark_divelist_changed(false);
	addRecentFile(QStringList() << QString(existing_filename));
	return 0;
}

void MainWindow::showError(QString message)
{
	if (message.isEmpty())
		return;
	ui.mainErrorMessage->setText(message);
	ui.mainErrorMessage->setCloseButtonVisible(true);
	ui.mainErrorMessage->setMessageType(KMessageWidget::Error);
	ui.mainErrorMessage->animatedShow();
}

void MainWindow::setTitle(enum MainWindowTitleFormat format)
{
	switch (format) {
	case MWTF_DEFAULT:
		setWindowTitle("Subsurface");
		break;
	case MWTF_FILENAME:
		if (!existing_filename) {
			setTitle(MWTF_DEFAULT);
			return;
		}
		QFile f(existing_filename);
		QFileInfo fileInfo(f);
		QString fileName(fileInfo.fileName());
		setWindowTitle("Subsurface: " + fileName);
		break;
	}
}

void MainWindow::importFiles(const QStringList fileNames)
{
	if (fileNames.isEmpty())
		return;

	QByteArray fileNamePtr;

	for (int i = 0; i < fileNames.size(); ++i) {
		fileNamePtr = QFile::encodeName(fileNames.at(i));
		parse_file(fileNamePtr.data());
	}
	process_dives(true, false);
	refreshDisplay();
}

void MainWindow::loadFiles(const QStringList fileNames)
{
	if (fileNames.isEmpty())
		return;

	QByteArray fileNamePtr;
	QStringList failedParses;

	for (int i = 0; i < fileNames.size(); ++i) {
		int error;

		fileNamePtr = QFile::encodeName(fileNames.at(i));
		error = parse_file(fileNamePtr.data());
		if (!error) {
			set_filename(fileNamePtr.data(), true);
			setTitle(MWTF_FILENAME);
		} else {
			failedParses.append(fileNames.at(i));
		}
	}

	process_dives(false, false);
	addRecentFile(fileNames);
	removeRecentFile(failedParses);

	refreshDisplay();
	ui.actionAutoGroup->setChecked(autogroup);
}

void MainWindow::on_actionImportDiveLog_triggered()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open dive log file"), lastUsedDir(),
		tr("Dive log files (*.xml *.uddf *.udcf *.csv *.jlb *.dld *.sde *.db);;"
			"XML files (*.xml);;UDDF/UDCF files(*.uddf *.udcf);;JDiveLog files(*.jlb);;"
			"Suunto Files(*.sde *.db);;CSV Files(*.csv);;All Files(*)"));

	if (fileNames.isEmpty())
		return;
	updateLastUsedDir(QFileInfo(fileNames[0]).dir().path());

	QStringList logFiles = fileNames.filter(QRegExp("^.*\\.(?!csv)", Qt::CaseInsensitive));
	QStringList csvFiles = fileNames.filter(".csv", Qt::CaseInsensitive);
	if (logFiles.size()) {
		importFiles(logFiles);
	}

	if (csvFiles.size()) {
		DiveLogImportDialog *diveLogImport = new DiveLogImportDialog(&csvFiles, this);
		diveLogImport->show();
		process_dives(true, false);
		refreshDisplay();
	}
}

void MainWindow::editCurrentDive()
{
	if (information()->isEditing() || DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING) {
		QMessageBox::warning(this, tr("Warning"), tr("Please, first finish the current edition before trying to do another."));
		return;
	}

	struct dive *d = current_dive;
	QString defaultDC(d->dc.model);
	DivePlannerPointsModel::instance()->clear();
	if (defaultDC == "manually added dive") {
		disableDcShortcuts();
		DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::ADD);
		ui.newProfile->setAddState();
		ui.infoPane->setCurrentIndex(MAINTAB);
		DivePlannerPointsModel::instance()->loadFromDive(d);
		ui.InfoWidget->enableEdition(MainTab::MANUALLY_ADDED_DIVE);
	} else if (defaultDC == "planned dive") {
		disableDcShortcuts();
		DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::PLAN);
		//TODO: I BROKE THIS BY COMMENTING THE LINE BELOW
		// and I'm sleepy now, so I think I should not try to fix right away.
		// we don't setCurrentIndex anymore, we ->setPlanState() or ->setAddState() on the ProfileView.
		//ui.stackedWidget->setCurrentIndex(PLANNERPROFILE); // Planner.
		ui.infoPane->setCurrentIndex(PLANNERWIDGET);
		DivePlannerPointsModel::instance()->loadFromDive(d);
		ui.InfoWidget->enableEdition(MainTab::MANUALLY_ADDED_DIVE);
	}
}

#define PREF_PROFILE(QT_PREFS)            \
	QSettings s;                      \
	s.beginGroup("TecDetails");       \
	s.setValue(#QT_PREFS, triggered); \
	PreferencesDialog::instance()->emitSettingsChanged();

#define TOOLBOX_PREF_PROFILE(METHOD, INTERNAL_PREFS, QT_PREFS)   \
	void MainWindow::on_##METHOD##_triggered(bool triggered) \
	{                                                        \
		prefs.INTERNAL_PREFS = triggered;                \
		PREF_PROFILE(QT_PREFS);                          \
	}

TOOLBOX_PREF_PROFILE(profCalcAllTissues, calcalltissues, calcalltissues);
TOOLBOX_PREF_PROFILE(profCalcCeiling, calcceiling, calcceiling);
TOOLBOX_PREF_PROFILE(profDcCeiling, dcceiling, dcceiling);
TOOLBOX_PREF_PROFILE(profEad, ead, ead);
TOOLBOX_PREF_PROFILE(profIncrement3m, calcceiling3m, calcceiling3m);
TOOLBOX_PREF_PROFILE(profMod, mod, mod);
TOOLBOX_PREF_PROFILE(profNdl_tts, calcndltts, calcndltts);
TOOLBOX_PREF_PROFILE(profPhe, pp_graphs.phe, phegraph);
TOOLBOX_PREF_PROFILE(profPn2, pp_graphs.pn2, pn2graph);
TOOLBOX_PREF_PROFILE(profPO2, pp_graphs.po2, po2graph);
TOOLBOX_PREF_PROFILE(profHR, hrgraph, hrgraph);
TOOLBOX_PREF_PROFILE(profRuler, rulergraph, rulergraph);
TOOLBOX_PREF_PROFILE(profSAC, show_sac, show_sac);
TOOLBOX_PREF_PROFILE(profScaled, zoomed_plot, zoomed_plot);
TOOLBOX_PREF_PROFILE(profTogglePicture, show_pictures_in_profile, show_pictures_in_profile);
TOOLBOX_PREF_PROFILE(profTankbar, tankbar, tankbar);
TOOLBOX_PREF_PROFILE(profTissues, percentagegraph, percentagegraph);

void MainWindow::turnOffNdlTts()
{
	const bool triggered = false;
	prefs.calcndltts = triggered;
	PREF_PROFILE(calcndltts);
}

#undef TOOLBOX_PREF_PROFILE
#undef PERF_PROFILE

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
	Q_FOREACH (QAction *b, profileToolbarActions)
		b->setEnabled(arg1);
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
	// take the data in our copyPasteDive and apply it to selected dives
	selective_copy_dive(&copyPasteDive, &displayed_dive, what, false);
	ui.InfoWidget->showAndTriggerEditSelective(what);
}

void MainWindow::on_actionFilterTags_triggered()
{
	ui.tagFilter->show();
}
