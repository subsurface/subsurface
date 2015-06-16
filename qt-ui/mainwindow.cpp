/*
 * mainwindow.cpp
 *
 * classes for the main UI window in Subsurface
 */
#include "mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QSettings>
#include <QShortcut>
#include <QToolBar>
#include "version.h"
#include "divelistview.h"
#include "downloadfromdivecomputer.h"
#include "preferences.h"
#include "subsurfacewebservices.h"
#include "divecomputermanagementdialog.h"
#include "about.h"
#include "updatemanager.h"
#include "planner.h"
#include "filtermodels.h"
#include "profile/profilewidget2.h"
#include "globe.h"
#include "divecomputer.h"
#include "maintab.h"
#include "diveplanner.h"
#ifndef NO_PRINTING
#include <QPrintDialog>
#include "printdialog.h"
#endif
#include "tankinfomodel.h"
#include "weigthsysteminfomodel.h"
#include "yearlystatisticsmodel.h"
#include "diveplannermodel.h"
#include "divelogimportdialog.h"
#include "divelogexportdialog.h"
#include "usersurvey.h"
#include "divesitehelpers.h"
#include "locationinformation.h"
#include "windowtitleupdate.h"
#ifndef NO_USERMANUAL
#include "usermanual.h"
#endif
#include "divepicturemodel.h"
#include "git-access.h"
#include <QNetworkProxy>
#include <QUndoStack>
#include <qthelper.h>
#include <QtConcurrentRun>

MainWindow *MainWindow::m_Instance = NULL;

MainWindow::MainWindow() : QMainWindow(),
	actionNextDive(0),
	actionPreviousDive(0),
	helpView(0),
	state(VIEWALL),
	survey(0)
{
	Q_ASSERT_X(m_Instance == NULL, "MainWindow", "MainWindow recreated!");
	m_Instance = this;
	ui.setupUi(this);
	read_hashes();
	// Define the States of the Application Here, Currently the states are situations where the different
	// widgets will change on the mainwindow.

	// for the "default" mode
	MainTab *mainTab = new MainTab();
	DiveListView *diveListView = new DiveListView();
	ProfileWidget2 *profileWidget = new ProfileWidget2();

#ifndef NO_MARBLE
	GlobeGPS *globeGps = new GlobeGPS();
#else
	QWidget *globeGps = NULL;
#endif

	PlannerSettingsWidget *plannerSettings = new PlannerSettingsWidget();
	DivePlannerWidget *plannerWidget = new DivePlannerWidget();
	PlannerDetails *plannerDetails = new PlannerDetails();
	LocationInformationWidget *locationInformation = new LocationInformationWidget();

	// what is a sane order for those icons? we should have the ones the user is
	// most likely to want towards the top so they are always visible
	// and the ones that someone likely sets and then never touches again towards the bottom
	profileToolbarActions << ui.profCalcCeiling << ui.profCalcAllTissues << // start with various ceilings
				 ui.profIncrement3m << ui.profDcCeiling <<
				 ui.profPhe << ui.profPn2 << ui.profPO2 << // partial pressure graphs
				 ui.profRuler << ui.profScaled << // measuring and scaling
				 ui.profTogglePicture << ui.profTankbar <<
				 ui.profMod << ui.profNdl_tts << // various values that a user is either interested in or not
				 ui.profEad << ui.profSAC <<
				 ui.profHR << // very few dive computers support this
				 ui.profTissues; // maybe less frequently used

	QToolBar *toolBar = new QToolBar();
	Q_FOREACH (QAction *a, profileToolbarActions)
		toolBar->addAction(a);
	toolBar->setOrientation(Qt::Vertical);
	toolBar->setIconSize(QSize(24,24));

	QWidget *profileContainer = new QWidget();
	QHBoxLayout *profLayout = new QHBoxLayout();
	profLayout->setSpacing(0);
	profLayout->setMargin(0);
	profLayout->setContentsMargins(0,0,0,0);
	profLayout->addWidget(toolBar);
	profLayout->addWidget(profileWidget);
	profileContainer->setLayout(profLayout);
	DivePictureWidget *divePictures = new DivePictureWidget(this);
	divePictures->setModel(DivePictureModel::instance());
	registerApplicationState("Default", mainTab, profileContainer, diveListView, globeGps );
	registerApplicationState("AddDive", mainTab, profileContainer, diveListView, globeGps );
	registerApplicationState("EditDive", mainTab, profileContainer, diveListView, globeGps );
	registerApplicationState("PlanDive", plannerWidget, profileContainer, plannerSettings, plannerDetails );
	registerApplicationState("EditPlannedDive", plannerWidget, profileContainer, diveListView, globeGps );
	registerApplicationState("EditDiveSite",locationInformation, divePictures, diveListView, globeGps );

	setApplicationState("Default");

	ui.multiFilter->hide();

	setWindowIcon(QIcon(":subsurface-icon"));
	if (!QIcon::hasThemeIcon("window-close")) {
		QIcon::setThemeName("subsurface");
	}
	connect(dive_list(), SIGNAL(currentDiveChanged(int)), this, SLOT(current_dive_changed(int)));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), this, SLOT(readSettings()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), diveListView, SLOT(update()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), diveListView, SLOT(reloadHeaderActions()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), information(), SLOT(updateDiveInfo()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), divePlannerWidget(), SLOT(settingsChanged()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), divePlannerSettingsWidget(), SLOT(settingsChanged()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), TankInfoModel::instance(), SLOT(update()));
	connect(ui.actionRecent1, SIGNAL(triggered(bool)), this, SLOT(recentFileTriggered(bool)));
	connect(ui.actionRecent2, SIGNAL(triggered(bool)), this, SLOT(recentFileTriggered(bool)));
	connect(ui.actionRecent3, SIGNAL(triggered(bool)), this, SLOT(recentFileTriggered(bool)));
	connect(ui.actionRecent4, SIGNAL(triggered(bool)), this, SLOT(recentFileTriggered(bool)));
	connect(information(), SIGNAL(addDiveFinished()), graphics(), SLOT(setProfileState()));
	connect(DivePlannerPointsModel::instance(), SIGNAL(planCreated()), this, SLOT(planCreated()));
	connect(DivePlannerPointsModel::instance(), SIGNAL(planCanceled()), this, SLOT(planCanceled()));
	connect(plannerDetails->printPlan(), SIGNAL(pressed()), divePlannerWidget(), SLOT(printDecoPlan()));
	connect(mainTab, SIGNAL(requestDiveSiteAdd()), this, SLOT(enableDiveSiteCreation()));
	connect(locationInformation, SIGNAL(informationManagementEnded()), this, SLOT(setDefaultState()));
	connect(locationInformation, SIGNAL(informationManagementEnded()), information(), SLOT(showLocation()));
	connect(locationInformation, SIGNAL(coordinatesChanged()), globe(), SLOT(repopulateLabels()));
	connect(locationInformation, SIGNAL(startEditDiveSite(uint32_t)), globeGps, SLOT(prepareForGetDiveCoordinates()));
	connect(locationInformation, SIGNAL(endEditDiveSite()), globeGps, SLOT(prepareForGetDiveCoordinates()));
	connect(information(), SIGNAL(diveSiteChanged(uint32_t)), globeGps, SLOT(centerOnDiveSite(uint32_t)));
	wtu = new WindowTitleUpdate();
	connect(WindowTitleUpdate::instance(), SIGNAL(updateTitle()), this, SLOT(setAutomaticTitle()));
#ifdef NO_PRINTING
	plannerDetails->printPlan()->hide();
	ui.menuFile->removeAction(ui.actionPrint);
#endif
#ifndef USE_LIBGIT23_API
	ui.menuFile->removeAction(ui.actionCloudstorageopen);
	ui.menuFile->removeAction(ui.actionCloudstoragesave);
	qDebug() << "disabled / made invisible the cloud storage stuff";
#else
	enableDisableCloudActions();
#endif

	ui.mainErrorMessage->hide();
	graphics()->setEmptyState();
	initialUiSetup();
	readSettings();
	diveListView->reload(DiveTripModel::TREE);
	diveListView->reloadHeaderActions();
	diveListView->setFocus();
	globe()->reload();
	diveListView->expand(dive_list()->model()->index(0, 0));
	diveListView->scrollTo(dive_list()->model()->index(0, 0), QAbstractItemView::PositionAtCenter);
	divePlannerWidget()->settingsChanged();
	divePlannerSettingsWidget()->settingsChanged();
#ifdef NO_MARBLE
	ui.menuView->removeAction(ui.actionViewGlobe);
#else
	connect(globe(), SIGNAL(coordinatesChanged()), locationInformation, SLOT(updateGpsCoordinates()));
#endif
#ifdef NO_USERMANUAL
	ui.menuHelp->removeAction(ui.actionUserManual);
#endif
	memset(&copyPasteDive, 0, sizeof(copyPasteDive));
	memset(&what, 0, sizeof(what));

	updateManager = new UpdateManager(this);
	undoStack = new QUndoStack(this);
	QAction *undoAction = undoStack->createUndoAction(this, tr("&Undo"));
	QAction *redoAction = undoStack->createRedoAction(this, tr("&Redo"));
	undoAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Z));
	redoAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z));
	QList<QAction*>undoRedoActions;
	undoRedoActions.append(undoAction);
	undoRedoActions.append(redoAction);
	ui.menu_Edit->addActions(undoRedoActions);

	ReverseGeoLookupThread *geoLookup = ReverseGeoLookupThread::instance();
	connect(geoLookup, SIGNAL(started()),information(), SLOT(disableGeoLookupEdition()));
	connect(geoLookup, SIGNAL(finished()), information(), SLOT(enableGeoLookupEdition()));
}

MainWindow::~MainWindow()
{
	write_hashes();
	m_Instance = NULL;
}

void MainWindow::enableDisableCloudActions()
{
#ifdef USE_LIBGIT23_API
	ui.actionCloudstorageopen->setEnabled(prefs.cloud_verification_status == CS_VERIFIED);
	ui.actionCloudstoragesave->setEnabled(prefs.cloud_verification_status == CS_VERIFIED);
#endif
}

PlannerDetails *MainWindow::plannerDetails() const {
	return qobject_cast<PlannerDetails*>(applicationState["PlanDive"].bottomRight);
}

PlannerSettingsWidget *MainWindow::divePlannerSettingsWidget() {
	return qobject_cast<PlannerSettingsWidget*>(applicationState["PlanDive"].bottomLeft);
}

LocationInformationWidget *MainWindow::locationInformationWidget() {
	return qobject_cast<LocationInformationWidget*>(applicationState["EditDiveSite"].topLeft);
}

void MainWindow::on_actionManage_dive_sites_triggered() {
	enableDiveSiteEdit(displayed_dive.dive_site_uuid);
}

void MainWindow::enableDiveSiteCreation() {
	locationInformationWidget()->createDiveSite();
	setApplicationState("EditDiveSite");
}

void MainWindow::enableDiveSiteEdit(uint32_t id) {
	locationInformationWidget()->editDiveSite(id);
	setApplicationState("EditDiveSite");
}

void MainWindow::setDefaultState() {
	setApplicationState("Default");
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
	getNotificationWidget()->showNotification(get_error_string(), KMessageWidget::Error);
	information()->reload();
	TankInfoModel::instance()->update();
	globe()->reload();
	if (doRecreateDiveList)
		recreateDiveList();

	setApplicationState("Default");
	dive_list()->setEnabled(true);
	dive_list()->setFocus();
	WSInfoModel::instance()->updateInfo();
	if (amount_selected == 0)
		cleanUpEmpty();
}

void MainWindow::recreateDiveList()
{
	dive_list()->reload(DiveTripModel::CURRENT);
	TagFilterModel::instance()->repopulate();
	BuddyFilterModel::instance()->repopulate();
	LocationFilterModel::instance()->repopulate();
	SuitsFilterModel::instance()->repopulate();
}

void MainWindow::current_dive_changed(int divenr)
{
	if (divenr >= 0) {
		select_dive(divenr);
	}
	graphics()->plotDive();
	information()->updateDiveInfo();
}

void MainWindow::on_actionNew_triggered()
{
	on_actionClose_triggered();
}

void MainWindow::on_actionOpen_triggered()
{
	if (!okToClose(tr("Please save or cancel the current dive edit before opening a new file.")))
		return;

	// yes, this look wrong to use getSaveFileName() for the open dialog, but we need to be able
	// to enter file names that don't exist in order to use our git syntax /path/to/dir[branch]
	// with is a potentially valid input, but of course won't exist. So getOpenFileName() wouldn't work
	QFileDialog dialog(this, tr("Open file"), lastUsedDir(), filter());
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setViewMode(QFileDialog::Detail);
	dialog.setLabelText(QFileDialog::Accept, tr("Open"));
	dialog.setLabelText(QFileDialog::Reject, tr("Cancel"));
	QStringList filenames;
	if (dialog.exec())
		filenames = dialog.selectedFiles();
	if (filenames.isEmpty())
		return;
	updateLastUsedDir(QFileInfo(filenames.first()).dir().path());
	closeCurrentFile();
	loadFiles(filenames);
}

void MainWindow::on_actionSave_triggered()
{
	file_save();
}

void MainWindow::on_actionSaveAs_triggered()
{
	file_save_as();
}

void MainWindow::on_actionCloudstorageopen_triggered()
{
	if (!okToClose(tr("Please save or cancel the current dive edit before opening a new file.")))
		return;

	QString filename;
	if (getCloudURL(filename)) {
		getNotificationWidget()->showNotification(get_error_string(), KMessageWidget::Error);
		return;
	}
	qDebug() << filename;

	closeCurrentFile();

	int error;

	QByteArray fileNamePtr = QFile::encodeName(filename);
	error = parse_file(fileNamePtr.data());
	if (!error) {
		set_filename(fileNamePtr.data(), true);
		setTitle(MWTF_FILENAME);
	}
	getNotificationWidget()->hideNotification();
	process_dives(false, false);
	refreshDisplay();
	ui.actionAutoGroup->setChecked(autogroup);
}

void MainWindow::on_actionCloudstoragesave_triggered()
{
	QString filename;
	if (getCloudURL(filename)) {
		getNotificationWidget()->showNotification(get_error_string(), KMessageWidget::Error);
		return;
	}
	qDebug() << filename;
	if (information()->isEditing())
		information()->acceptChanges();

	if (save_dives(filename.toUtf8().data())) {
		getNotificationWidget()->showNotification(get_error_string(), KMessageWidget::Error);
		return;
	}
	getNotificationWidget()->showNotification(get_error_string(), KMessageWidget::Error);
	set_filename(filename.toUtf8().data(), true);
	setTitle(MWTF_FILENAME);
	mark_divelist_changed(false);
}

void learnImageDirs(QStringList dirnames)
{
	QList<QFuture<void> > futures;
	foreach (QString dir, dirnames) {
		futures << QtConcurrent::run(learnImages, QDir(dir), 10, false);
	}
	DivePictureModel::instance()->updateDivePicturesWhenDone(futures);
}

void MainWindow::on_actionHash_images_triggered()
{
	QFuture<void> future;
	QFileDialog dialog(this, tr("Traverse image directories"), lastUsedDir(), filter());
	dialog.setFileMode(QFileDialog::Directory);
	dialog.setViewMode(QFileDialog::Detail);
	dialog.setLabelText(QFileDialog::Accept, tr("Scan"));
	dialog.setLabelText(QFileDialog::Reject, tr("Cancel"));
	QStringList dirnames;
	if (dialog.exec())
		dirnames = dialog.selectedFiles();
	if (dirnames.isEmpty())
		return;
	future = QtConcurrent::run(learnImageDirs,dirnames);
	MainWindow::instance()->getNotificationWidget()->showNotification(tr("Scanning images...(this can take a while)"), KMessageWidget::Information);
	MainWindow::instance()->getNotificationWidget()->setFuture(future);

}

ProfileWidget2 *MainWindow::graphics() const
{
	return qobject_cast<ProfileWidget2*>(applicationState["Default"].topRight->layout()->itemAt(1)->widget());
}

void MainWindow::cleanUpEmpty()
{
	information()->clearStats();
	information()->clearInfo();
	information()->clearEquipment();
	information()->updateDiveInfo(true);
	graphics()->setEmptyState();
	dive_list()->reload(DiveTripModel::TREE);
	globe()->reload();
	if (!existing_filename)
		setTitle(MWTF_DEFAULT);
	disableShortcuts();
}

bool MainWindow::okToClose(QString message)
{
	if (DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING ||
		information()->isEditing() ||
		currentApplicationState == "EditDiveSite") {
		QMessageBox::warning(this, tr("Warning"), message);
		return false;
	}
	if (unsaved_changes() && askSaveChanges() == false)
		return false;

	return true;
}

void MainWindow::closeCurrentFile()
{
	graphics()->setEmptyState();
	/* free the dives and trips */
	clear_git_id();
	while (dive_table.nr)
		delete_single_dive(0);
	while (dive_site_table.nr)
		delete_dive_site(get_dive_site(0)->uuid);

	free((void *)existing_filename);
	existing_filename = NULL;

	cleanUpEmpty();
	mark_divelist_changed(false);

	clear_events();

	dcList.dcMap.clear();
}

void MainWindow::on_actionClose_triggered()
{
	if (okToClose(tr("Please save or cancel the current dive edit before closing the file."))) {
		closeCurrentFile();
		// hide any pictures and the filter
		DivePictureModel::instance()->updateDivePictures();
		ui.multiFilter->closeFilter();
		recreateDiveList();
	}
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

void MainWindow::disableShortcuts(bool disablePaste)
{
	ui.actionPreviousDC->setShortcut(QKeySequence());
	ui.actionNextDC->setShortcut(QKeySequence());
	ui.copy->setShortcut(QKeySequence());
	if (disablePaste)
		ui.paste->setShortcut(QKeySequence());
}

void MainWindow::enableShortcuts()
{
	ui.actionPreviousDC->setShortcut(Qt::Key_Left);
	ui.actionNextDC->setShortcut(Qt::Key_Right);
	ui.copy->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
	ui.paste->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_V));
}

void MainWindow::showProfile()
{
	enableShortcuts();
	graphics()->setProfileState();
	setApplicationState("Default");
}

void MainWindow::on_actionPreferences_triggered()
{
	PreferencesDialog::instance()->show();
}

void MainWindow::on_actionQuit_triggered()
{
	if (information()->isEditing()) {
		information()->rejectChanges();
		if (information()->isEditing())
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
		information()->isEditing()) {
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
	graphics()->replot();
	refreshDisplay(false);
	graphics()->plotDive(get_dive(selected_dive));
	DivePictureModel::instance()->updateDivePictures();
}

void MainWindow::planCreated()
{
	// get the new dive selected and assign a number if reasonable
	graphics()->setProfileState();
	if (displayed_dive.id == 0) {
		// we might have added a new dive (so displayed_dive was cleared out by clone_dive()
		dive_list()->unselectDives();
		select_dive(dive_table.nr - 1);
		dive_list()->selectDive(selected_dive);
		set_dive_nr_for_current_dive();
	}
	// make sure our UI is in a consistent state
	information()->updateDiveInfo();
	showProfile();
	refreshDisplay();
}

void MainWindow::setPlanNotes()
{
	plannerDetails()->divePlanOutput()->setHtml(displayed_dive.notes);
}

void MainWindow::printPlan()
{
#ifndef NO_PRINTING
	QString diveplan = plannerDetails()->divePlanOutput()->toHtml();
	QString withDisclaimer = QString("<img height=50 src=\":subsurface-icon\"> ") + diveplan + QString(disclaimer);

	QPrinter printer;
	QPrintDialog *dialog = new QPrintDialog(&printer, this);
	dialog->setWindowTitle(tr("Print runtime table"));
	if (dialog->exec() != QDialog::Accepted)
		return;

	plannerDetails()->divePlanOutput()->setHtml(withDisclaimer);
	plannerDetails()->divePlanOutput()->print(&printer);
	plannerDetails()->divePlanOutput()->setHtml(diveplan);
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
	if (!plannerStateClean() || !current_dive || !current_dive->dc.model)
		return;
	else if (strcmp(current_dive->dc.model, "planned dive")) {
		if (QMessageBox::warning(this, tr("Warning"), tr("trying to replan a dive that's not a planned dive."),
					 QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)
					return;
	}
	// put us in PLAN mode
	DivePlannerPointsModel::instance()->clear();
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::PLAN);

	graphics()->setPlanState();
	graphics()->clearHandlers();
	setApplicationState("PlanDive");
	divePlannerWidget()->setReplanButton(true);
	DivePlannerPointsModel::instance()->loadFromDive(current_dive);
	reset_cylinders(&displayed_dive, true);
}

void MainWindow::on_actionDivePlanner_triggered()
{
	if (!plannerStateClean())
		return;

	// put us in PLAN mode
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::PLAN);
	setApplicationState("PlanDive");

	graphics()->setPlanState();

	// create a simple starting dive, using the first gas from the just copied cylinders
	setupForAddAndPlan("planned dive"); // don't translate, stored in XML file
	DivePlannerPointsModel::instance()->setupStartTime();
	DivePlannerPointsModel::instance()->createSimpleDive();
	DivePictureModel::instance()->updateDivePictures();
	divePlannerWidget()->setReplanButton(false);
}

DivePlannerWidget* MainWindow::divePlannerWidget() {
	return qobject_cast<DivePlannerWidget*>(applicationState["PlanDive"].topLeft);
}

void MainWindow::on_actionAddDive_triggered()
{
	if (!plannerStateClean())
		return;

	if (dive_list()->selectedTrips().count() >= 1) {
		dive_list()->rememberSelection();
		dive_list()->clearSelection();
	}

	setApplicationState("AddDive");
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::ADD);

	// setup things so we can later create our starting dive
	setupForAddAndPlan("manually added dive"); // don't translate, stored in the XML file

	// now show the mostly empty main tab
	information()->updateDiveInfo();

	// show main tab
	information()->setCurrentIndex(0);

	information()->addDiveStarted();

	graphics()->setAddState();
	DivePlannerPointsModel::instance()->createSimpleDive();
	graphics()->plotDive();
}

void MainWindow::on_actionEditDive_triggered()
{
	if (information()->isEditing() || DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING) {
		QMessageBox::warning(this, tr("Warning"), tr("Please, first finish the current edition before trying to do another."));
		return;
	}

	const bool isTripEdit = dive_list()->selectedTrips().count() >= 1;
	if (!current_dive || isTripEdit || strcmp(current_dive->dc.model, "manually added dive")) {
		QMessageBox::warning(this, tr("Warning"), tr("Trying to edit a dive that's not a manually added dive."));
		return;
	}

	DivePlannerPointsModel::instance()->clear();
	disableShortcuts();
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::ADD);
	graphics()->setAddState();
	globe()->endGetDiveCoordinates();
	setApplicationState("EditDive");
	DivePlannerPointsModel::instance()->loadFromDive(current_dive);
	information()->enableEdition(MainTab::MANUALLY_ADDED_DIVE);
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
	QVBoxLayout *l = new QVBoxLayout(&d);
	YearlyStatisticsModel *m = new YearlyStatisticsModel();
	QTreeView *view = new QTreeView();
	view->setModel(m);
	l->addWidget(view);
	d.resize(width() * .8, height() / 2);
	d.move(width() * .1, height() / 4);
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), &d);
	connect(close, SIGNAL(activated()), &d, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), &d);
	connect(quit, SIGNAL(activated()), this, SLOT(close()));
	d.setWindowFlags(Qt::Window | Qt::CustomizeWindowHint
		| Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
	d.setWindowTitle(tr("Yearly statistics"));
	d.exec();
}

#define BEHAVIOR QList<int>()

#define TOGGLE_COLLAPSABLE( X ) \
	ui.mainSplitter->setCollapsible(0, X); \
	ui.mainSplitter->setCollapsible(1, X); \
	ui.topSplitter->setCollapsible(0, X); \
	ui.topSplitter->setCollapsible(1, X); \
	ui.bottomSplitter->setCollapsible(0, X); \
	ui.bottomSplitter->setCollapsible(1, X);

void MainWindow::on_actionViewList_triggered()
{
	TOGGLE_COLLAPSABLE( true );
	beginChangeState(LIST_MAXIMIZED);
	ui.topSplitter->setSizes(BEHAVIOR << EXPANDED << COLLAPSED);
	ui.mainSplitter->setSizes(BEHAVIOR << COLLAPSED << EXPANDED);
}

void MainWindow::on_actionViewProfile_triggered()
{
	TOGGLE_COLLAPSABLE( true );
	beginChangeState(PROFILE_MAXIMIZED);
	ui.topSplitter->setSizes(BEHAVIOR << COLLAPSED << EXPANDED);
	ui.mainSplitter->setSizes(BEHAVIOR << EXPANDED << COLLAPSED);
}

void MainWindow::on_actionViewInfo_triggered()
{
	TOGGLE_COLLAPSABLE( true );
	beginChangeState(INFO_MAXIMIZED);
	ui.topSplitter->setSizes(BEHAVIOR << EXPANDED << COLLAPSED);
	ui.mainSplitter->setSizes(BEHAVIOR << EXPANDED << COLLAPSED);
}

void MainWindow::on_actionViewGlobe_triggered()
{
	TOGGLE_COLLAPSABLE( true );
	beginChangeState(GLOBE_MAXIMIZED);
	ui.mainSplitter->setSizes(BEHAVIOR << COLLAPSED << EXPANDED);
	ui.bottomSplitter->setSizes(BEHAVIOR << COLLAPSED << EXPANDED);
}
#undef BEHAVIOR

void MainWindow::on_actionViewAll_triggered()
{
	TOGGLE_COLLAPSABLE( false );
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
		ui.topSplitter->restoreState(settings.value("topSplitter").toByteArray());
		ui.bottomSplitter->restoreState(settings.value("bottomSplitter").toByteArray());
		if (ui.mainSplitter->sizes().first() == 0 || ui.mainSplitter->sizes().last() == 0)
			ui.mainSplitter->setSizes(mainSizes);
		if (ui.topSplitter->sizes().first() == 0 || ui.topSplitter->sizes().last() == 0)
			ui.topSplitter->setSizes(infoProfileSizes);
		if (ui.bottomSplitter->sizes().first() == 0 || ui.bottomSplitter->sizes().last() == 0)
			ui.bottomSplitter->setSizes(listGlobeSizes);

	} else {
		ui.mainSplitter->setSizes(mainSizes);
		ui.topSplitter->setSizes(infoProfileSizes);
		ui.bottomSplitter->setSizes(listGlobeSizes);
	}
	ui.mainSplitter->setCollapsible(0, false);
	ui.mainSplitter->setCollapsible(1, false);
	ui.topSplitter->setCollapsible(0, false);
	ui.topSplitter->setCollapsible(1, false);
	ui.bottomSplitter->setCollapsible(0,false);
	ui.bottomSplitter->setCollapsible(1,false);
}

#undef TOGGLE_COLLAPSABLE

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
	settings.setValue("topSplitter", ui.topSplitter->saveState());
	settings.setValue("bottomSplitter", ui.bottomSplitter->saveState());
}

void MainWindow::on_actionPreviousDC_triggered()
{
	unsigned nrdc = number_of_computers(current_dive);
	dc_number = (dc_number + nrdc - 1) % nrdc;
	graphics()->plotDive();
	information()->updateDiveInfo();
}

void MainWindow::on_actionNextDC_triggered()
{
	unsigned nrdc = number_of_computers(current_dive);
	dc_number = (dc_number + 1) % nrdc;
	graphics()->plotDive();
	information()->updateDiveInfo();
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

void MainWindow::on_actionUserSurvey_triggered()
{
	if(!survey) {
		survey = new UserSurvey(this);
	}
	survey->show();
}

QString MainWindow::filter()
{
	QString f;
	f += "Dive log files ( *.ssrf ";
	f += "*.can *.CAN ";
	f += "*.db *.DB " ;
	f += "*.dld *.DLD ";
	f += "*.jlb *.JLB ";
	f += "*.lvd *.LVD ";
	f += "*.sde *.SDE ";
	f += "*.udcf *.UDCF ";
	f += "*.uddf *.UDDF ";
	f += "*.xml *.XML ";
	f += "*.dlf *.DLF ";
	f += ");;";

	f += "Subsurface (*.ssrf);;";
	f += "Cochran (*.can *.CAN);;";
	f += "DiveLogs.de (*.dld *.DLD);;";
	f += "JDiveLog  (*.jlb *.JLB);;";
	f += "Liquivision (*.lvd *.LVD);;";
	f += "Suunto (*.sde *.SDE *.db *.DB);;";
	f += "UDCF (*.udcf *.UDCF);;";
	f += "UDDF (*.uddf *.UDDF);;";
	f += "XML (*.xml *.XML)";
	f += "Divesoft (*.dlf *.DLF)";
	f += "Datatrak/WLog Files (*.log *.LOG)";

	return f;
}

bool MainWindow::askSaveChanges()
{
	QString message;
	QMessageBox response(this);

	if (existing_filename)
		message = tr("Do you want to save the changes that you made in the file %1?")
				.arg(displayedFilename(existing_filename));
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
	TOOLBOX_PREF_BUTTON(percentagegraph, percentagegraph, profTissues);
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
	QString ver(subsurface_version());
	int waitTime = 7;
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
		information()->isEditing()) {
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

	if (survey && survey->isVisible()) {
		survey->close();
		survey->deleteLater();
	}

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
	return qobject_cast<DiveListView*>(applicationState["Default"].bottomLeft);
}

GlobeGPS *MainWindow::globe()
{
	return qobject_cast<GlobeGPS*>(applicationState["Default"].bottomRight);
}

MainTab *MainWindow::information()
{
	return qobject_cast<MainTab*>(applicationState["Default"].topLeft);
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

	// create a file dialog that allows us to save to a new file
	QFileDialog selection_dialog(this, tr("Save file as"), default_filename,
					 tr("Subsurface XML files (*.ssrf *.xml *.XML)"));
	selection_dialog.setAcceptMode(QFileDialog::AcceptSave);
	selection_dialog.setFileMode(QFileDialog::AnyFile);
	selection_dialog.setDefaultSuffix("");

	/* if the exit/cancel button is pressed return */
	if (!selection_dialog.exec())
		return 0;

	/* get the first selected file */
	filename = selection_dialog.selectedFiles().at(0);

	/* now for reasons I don't understand we appear to add a .ssrf to
	 * git style filenames <path>/directory[branch]
	 * so let's remove that */
	QRegExp r("\\[.*\\]\\.ssrf$", Qt::CaseInsensitive);
	if (filename.contains(r))
		filename.remove(QRegExp("\\.ssrf$", Qt::CaseInsensitive));
	if (filename.isNull() || filename.isEmpty())
		return report_error("No filename to save into");

	if (information()->isEditing())
		information()->acceptChanges();

	if (save_dives(filename.toUtf8().data())) {
		getNotificationWidget()->showNotification(get_error_string(), KMessageWidget::Error);
		return -1;
	}

	getNotificationWidget()->showNotification(get_error_string(), KMessageWidget::Error);
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

	if (information()->isEditing())
		information()->acceptChanges();

	current_default = prefs.default_filename;
	if (strcmp(existing_filename, current_default) == 0) {
		/* if we are using the default filename the directory
		 * that we are creating the file in may not exist */
		QDir current_def_dir = QFileInfo(current_default).absoluteDir();
		if (!current_def_dir.exists())
			current_def_dir.mkpath(current_def_dir.absolutePath());
	}
	if (save_dives(existing_filename)) {
		getNotificationWidget()->showNotification(get_error_string(), KMessageWidget::Error);
		return -1;
	}
	getNotificationWidget()->showNotification(get_error_string(), KMessageWidget::Error);
	mark_divelist_changed(false);
	addRecentFile(QStringList() << QString(existing_filename));
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

	if (fullFilename.contains(prefs.cloud_git_url))
		return tr("[cloud storage for] %1").arg(fileName.left(fileName.indexOf('[')));
	else
		return fileName;
}

void MainWindow::setAutomaticTitle()
{
	setTitle();
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
		QString unsaved = (unsaved_changes() ? " *" : "");
		setWindowTitle("Subsurface: " + displayedFilename(existing_filename) + unsaved);
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

void MainWindow::importTxtFiles(const QStringList fileNames)
{
	if (fileNames.isEmpty())
		return;

	QByteArray fileNamePtr, csv;

	for (int i = 0; i < fileNames.size(); ++i) {
		fileNamePtr = QFile::encodeName(fileNames.at(i));
		csv = fileNamePtr.data();
		csv.replace(strlen(csv.data()) - 3, 3, "csv");
		parse_txt_file(fileNamePtr.data(), csv);
	}
	process_dives(true, false);
	refreshDisplay();
}

void MainWindow::loadFiles(const QStringList fileNames)
{
	bool showWarning = false;
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
			// if there were any messages, show them
			QString warning = get_error_string();
			if (!warning.isEmpty()) {
				showWarning = true;
				getNotificationWidget()->showNotification(warning , KMessageWidget::Information);
			}
		} else {
			failedParses.append(fileNames.at(i));
		}
	}
	if (!showWarning)
		getNotificationWidget()->hideNotification();
	process_dives(false, false);
	addRecentFile(fileNames);
	removeRecentFile(failedParses);

	// searches for geo lookup information in a thread so it doesn`t
	// freezes the ui.
	ReverseGeoLookupThread::instance()->start();

	refreshDisplay();
	ui.actionAutoGroup->setChecked(autogroup);
}

void MainWindow::on_actionImportDiveLog_triggered()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open dive log file"), lastUsedDir(),
		tr("Dive log files (*.ssrf *.can *.csv *.db *.dld *.jlb *.lvd *.sde *.udcf *.uddf *.xml *.txt *.dlf *.apd"
			"*.SSRF *.CAN *.CSV *.DB *.DLD *.JLB *.LVD *.SDE *.UDCF *.UDDF *.xml *.TXT *.DLF *.APD);;"
			"Cochran files (*.can *.CAN);;"
			"CSV files (*.csv *.CSV);;"
			"DiveLog.de files (*.dld *.DLD);;"
			"JDiveLog files (*.jlb *.JLB);;"
			"Liquivision files (*.lvd *.LVD);;"
			"MkVI files (*.txt *.TXT);;"
			"Suunto files (*.sde *.db *.SDE *.DB);;"
			"Divesoft files (*.dlf *.DLF);;"
			"UDDF/UDCF files (*.uddf *.udcf *.UDDF *.UDCF);;"
			"XML files (*.xml *.XML);;"
			"APD log viewer (*.apd *.APD);;"
			"Datatrak/WLog Files (*.log *.LOG);;"
			"OSTCtools Files (*.dive *.DIVE);;"
			"All files (*)"));

	if (fileNames.isEmpty())
		return;
	updateLastUsedDir(QFileInfo(fileNames[0]).dir().path());

	QStringList logFiles = fileNames.filter(QRegExp("^.*\\.(?!(csv|txt|apd))", Qt::CaseInsensitive));
	QStringList csvFiles = fileNames.filter(".csv", Qt::CaseInsensitive);
	csvFiles += fileNames.filter(".apd", Qt::CaseInsensitive);
	QStringList txtFiles = fileNames.filter(".txt", Qt::CaseInsensitive);

	if (logFiles.size()) {
		importFiles(logFiles);
	}

	if (csvFiles.size()) {
		DiveLogImportDialog *diveLogImport = new DiveLogImportDialog(csvFiles, this);
		diveLogImport->show();
		process_dives(true, false);
		refreshDisplay();
	}

	if (txtFiles.size()) {
		importTxtFiles(txtFiles);
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
		disableShortcuts();
		DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::ADD);
		graphics()->setAddState();
		setApplicationState("EditDive");
		DivePlannerPointsModel::instance()->loadFromDive(d);
		information()->enableEdition(MainTab::MANUALLY_ADDED_DIVE);
	} else if (defaultDC == "planned dive") {
		disableShortcuts();
		DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::PLAN);
		setApplicationState("EditPlannedDive");
		DivePlannerPointsModel::instance()->loadFromDive(d);
		information()->enableEdition(MainTab::MANUALLY_ADDED_DIVE);
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
	information()->showAndTriggerEditSelective(what);
}

void MainWindow::on_actionFilterTags_triggered()
{
	if (ui.multiFilter->isVisible())
		ui.multiFilter->closeFilter();
	else
		ui.multiFilter->setVisible(true);
}

void MainWindow::registerApplicationState(const QByteArray& state, QWidget *topLeft, QWidget *topRight, QWidget *bottomLeft, QWidget *bottomRight)
{
	applicationState[state] = WidgetForQuadrant(topLeft, topRight, bottomLeft, bottomRight);
	if (ui.topLeft->indexOf(topLeft) == -1 && topLeft) {
		ui.topLeft->addWidget(topLeft);
	}
	if (ui.topRight->indexOf(topRight) == -1 && topRight) {
		ui.topRight->addWidget(topRight);
	}
	if (ui.bottomLeft->indexOf(bottomLeft) == -1 && bottomLeft) {
		ui.bottomLeft->addWidget(bottomLeft);
	}
	if(ui.bottomRight->indexOf(bottomRight) == -1 && bottomRight) {
		ui.bottomRight->addWidget(bottomRight);
	}
}

void MainWindow::setApplicationState(const QByteArray& state) {
	if (!applicationState.keys().contains(state))
		return;

	if (currentApplicationState == state)
		return;

	currentApplicationState = state;
#define SET_CURRENT_INDEX( X ) \
	if (applicationState[state].X) { \
		ui.X->setCurrentWidget( applicationState[state].X); \
		ui.X->show(); \
	} else { \
		ui.X->hide(); \
	}

	SET_CURRENT_INDEX( topLeft )
	SET_CURRENT_INDEX( topRight )
	SET_CURRENT_INDEX( bottomLeft )
	SET_CURRENT_INDEX( bottomRight )
#undef SET_CURRENT_INDEX
}
