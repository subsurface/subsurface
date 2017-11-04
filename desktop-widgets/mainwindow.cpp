// SPDX-License-Identifier: GPL-2.0
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
#include <QStatusBar>

#include "core/version.h"
#include "desktop-widgets/divelistview.h"
#include "desktop-widgets/downloadfromdivecomputer.h"
#include "desktop-widgets/subsurfacewebservices.h"
#include "desktop-widgets/divecomputermanagementdialog.h"
#include "desktop-widgets/about.h"
#include "desktop-widgets/updatemanager.h"
#include "core/planner.h"
#include "qt-models/filtermodels.h"
#include "profile-widget/profilewidget2.h"
#include "core/divecomputer.h"
#include "desktop-widgets/tab-widgets/maintab.h"
#include "desktop-widgets/diveplanner.h"
#ifndef NO_PRINTING
#include <QPrintDialog>
#include <QBuffer>
#include "desktop-widgets/printdialog.h"
#endif
#include "qt-models/tankinfomodel.h"
#include "qt-models/weigthsysteminfomodel.h"
#include "qt-models/yearlystatisticsmodel.h"
#include "qt-models/diveplannermodel.h"
#include "desktop-widgets/divelogimportdialog.h"
#include "desktop-widgets/divelogexportdialog.h"
#include "desktop-widgets/usersurvey.h"
#include "core/divesitehelpers.h"
#include "core/windowtitleupdate.h"
#include "desktop-widgets/locationinformation.h"
#include "preferences/preferencesdialog.h"

#ifndef NO_USERMANUAL
#include "usermanual.h"
#endif
#include "qt-models/divepicturemodel.h"
#include "core/git-access.h"
#include <QNetworkProxy>
#include <QUndoStack>
#include "core/qthelper.h"
#include <QtConcurrentRun>
#include "core/color.h"
#include "core/isocialnetworkintegration.h"
#include "core/pluginmanager.h"
#include "core/subsurface-qt/SettingsObjectWrapper.h"

#if defined(FBSUPPORT)
#include "plugins/facebook/facebook_integration.h"
#include "plugins/facebook/facebookconnectwidget.h"
#endif

#include "desktop-widgets/mapwidget.h"

QProgressDialog *progressDialog = NULL;
bool progressDialogCanceled = false;

static int progressCounter = 0;

extern "C" int updateProgress(const char *text)
{
	if (verbose)
		qDebug() << "git storage:" << text;
	if (progressDialog) {
		progressDialog->setLabelText(text);
		progressDialog->setValue(++progressCounter);
		if (progressCounter == 100)
			progressCounter = 0; // yes this is silly, but we really don't know how long it will take
	}
	qApp->processEvents();
	return progressDialogCanceled;
}

MainWindow *MainWindow::m_Instance = NULL;

extern "C" void showErrorFromC()
{
	// Show errors only if we are running in the GUI thread.
	// If we're not in the GUI thread, let errors accumulate.
	if (QThread::currentThread() != QCoreApplication::instance()->thread())
		return;

	MainWindow *mainwindow = MainWindow::instance();
	if (mainwindow)
		mainwindow->showErrors();
}

void MainWindow::showErrors()
{
	const char *error = get_error_string();
	if (error && error[0])
		getNotificationWidget()->showNotification(error, KMessageWidget::Error);
}

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
	MapWidget *mapWidget = MapWidget::instance();

	PlannerSettingsWidget *plannerSettings = new PlannerSettingsWidget();
	DivePlannerWidget *plannerWidget = new DivePlannerWidget();
	PlannerDetails *plannerDetails = new PlannerDetails();

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

	LocationInformationWidget * diveSiteEdit = new LocationInformationWidget();
	connect(diveSiteEdit, &LocationInformationWidget::endEditDiveSite,
			this, &MainWindow::setDefaultState);

	connect(diveSiteEdit, &LocationInformationWidget::endEditDiveSite,
			mainTab, &MainTab::refreshDiveInfo);

	connect(diveSiteEdit, &LocationInformationWidget::endEditDiveSite,
			mainTab, &MainTab::refreshDisplayedDiveSite);

	std::pair<QByteArray, QVariant> enabled = std::make_pair("enabled", QVariant(true));
	std::pair<QByteArray, QVariant> disabled = std::make_pair("enabled", QVariant(false));
	PropertyList enabledList;
	PropertyList disabledList;
	enabledList.push_back(enabled);
	disabledList.push_back(disabled);

	registerApplicationState("Default", mainTab, profileContainer, diveListView, mapWidget );
	registerApplicationState("AddDive", mainTab, profileContainer, diveListView, mapWidget );
	registerApplicationState("EditDive", mainTab, profileContainer, diveListView, mapWidget );
	registerApplicationState("PlanDive", plannerWidget, profileContainer, plannerSettings, plannerDetails );
	registerApplicationState("EditPlannedDive", plannerWidget, profileContainer, diveListView, mapWidget );
	registerApplicationState("EditDiveSite", diveSiteEdit, profileContainer, diveListView, mapWidget);

	setStateProperties("Default", enabledList, enabledList, enabledList,enabledList);
	setStateProperties("AddDive", enabledList, enabledList, enabledList,enabledList);
	setStateProperties("EditDive", enabledList, enabledList, enabledList,enabledList);
	setStateProperties("PlanDive", enabledList, enabledList, enabledList,enabledList);
	setStateProperties("EditPlannedDive", enabledList, enabledList, enabledList,enabledList);
	setStateProperties("EditDiveSite", enabledList, disabledList, disabledList, enabledList);

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
	connect(information(), SIGNAL(dateTimeChanged()), graphics(), SLOT(dateTimeChanged()));
	connect(DivePlannerPointsModel::instance(), SIGNAL(planCreated()), this, SLOT(planCreated()));
	connect(DivePlannerPointsModel::instance(), SIGNAL(planCanceled()), this, SLOT(planCanceled()));
	connect(plannerDetails->printPlan(), SIGNAL(pressed()), divePlannerWidget(), SLOT(printDecoPlan()));
	connect(this, SIGNAL(startDiveSiteEdit()), this, SLOT(on_actionDiveSiteEdit_triggered()));
	connect(information(), SIGNAL(diveSiteChanged(struct dive_site *)), mapWidget, SLOT(centerOnDiveSite(struct dive_site *)));

	wtu = new WindowTitleUpdate();
	connect(WindowTitleUpdate::instance(), SIGNAL(updateTitle()), this, SLOT(setAutomaticTitle()));
#ifdef NO_PRINTING
	plannerDetails->printPlan()->hide();
	ui.menuFile->removeAction(ui.actionPrint);
#endif
	enableDisableCloudActions();

	ui.mainErrorMessage->hide();
	graphics()->setEmptyState();
	initialUiSetup();
	readSettings();
	diveListView->reload(DiveTripModel::TREE);
	diveListView->reloadHeaderActions();
	diveListView->setFocus();
	MapWidget::instance()->reload();
	diveListView->expand(dive_list()->model()->index(0, 0));
	diveListView->scrollTo(dive_list()->model()->index(0, 0), QAbstractItemView::PositionAtCenter);
	divePlannerWidget()->settingsChanged();
	divePlannerSettingsWidget()->settingsChanged();
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
#ifndef NO_PRINTING
	// copy the bundled print templates to the user path; no overwriting occurs!
	copyPath(getPrintingTemplatePathBundle(), getPrintingTemplatePathUser());
	find_all_templates();
#endif

	setupSocialNetworkMenu();
	set_git_update_cb(&updateProgress);
	set_error_cb(&showErrorFromC);

	// Toolbar Connections related to the Profile Update
	SettingsObjectWrapper *sWrapper = SettingsObjectWrapper::instance();
	connect(ui.profCalcAllTissues, &QAction::triggered, sWrapper->techDetails, &TechnicalDetailsSettings::setCalcalltissues);
	connect(ui.profCalcCeiling,    &QAction::triggered, sWrapper->techDetails, &TechnicalDetailsSettings::setCalcceiling);
	connect(ui.profDcCeiling,      &QAction::triggered, sWrapper->techDetails, &TechnicalDetailsSettings::setDCceiling);
	connect(ui.profEad,            &QAction::triggered, sWrapper->techDetails, &TechnicalDetailsSettings::setEad);
	connect(ui.profIncrement3m,    &QAction::triggered, sWrapper->techDetails, &TechnicalDetailsSettings::setCalcceiling3m);
	connect(ui.profMod,            &QAction::triggered, sWrapper->techDetails, &TechnicalDetailsSettings::setMod);
	connect(ui.profNdl_tts,        &QAction::triggered, sWrapper->techDetails, &TechnicalDetailsSettings::setCalcndltts);
	connect(ui.profHR,             &QAction::triggered, sWrapper->techDetails, &TechnicalDetailsSettings::setHRgraph);
	connect(ui.profRuler,          &QAction::triggered, sWrapper->techDetails, &TechnicalDetailsSettings::setRulerGraph);
	connect(ui.profSAC,            &QAction::triggered, sWrapper->techDetails, &TechnicalDetailsSettings::setShowSac);
	connect(ui.profScaled,         &QAction::triggered, sWrapper->techDetails, &TechnicalDetailsSettings::setZoomedPlot);
	connect(ui.profTogglePicture,  &QAction::triggered, sWrapper->techDetails, &TechnicalDetailsSettings::setShowPicturesInProfile);
	connect(ui.profTankbar,        &QAction::triggered, sWrapper->techDetails, &TechnicalDetailsSettings::setTankBar);
	connect(ui.profTissues,        &QAction::triggered, sWrapper->techDetails, &TechnicalDetailsSettings::setPercentageGraph);

	connect(ui.profPhe, &QAction::triggered, sWrapper->pp_gas, &PartialPressureGasSettings::setShowPhe);
	connect(ui.profPn2, &QAction::triggered, sWrapper->pp_gas, &PartialPressureGasSettings::setShowPn2);
	connect(ui.profPO2, &QAction::triggered, sWrapper->pp_gas, &PartialPressureGasSettings::setShowPo2);

	connect(sWrapper->techDetails, &TechnicalDetailsSettings::calcalltissuesChanged        , graphics(), &ProfileWidget2::actionRequestedReplot);
	connect(sWrapper->techDetails, &TechnicalDetailsSettings::calcceilingChanged           , graphics(), &ProfileWidget2::actionRequestedReplot);
	connect(sWrapper->techDetails, &TechnicalDetailsSettings::dcceilingChanged             , graphics(), &ProfileWidget2::actionRequestedReplot);
	connect(sWrapper->techDetails, &TechnicalDetailsSettings::eadChanged                   , graphics(), &ProfileWidget2::actionRequestedReplot);
	connect(sWrapper->techDetails, &TechnicalDetailsSettings::calcceiling3mChanged         , graphics(), &ProfileWidget2::actionRequestedReplot);
	connect(sWrapper->techDetails, &TechnicalDetailsSettings::modChanged                   , graphics(), &ProfileWidget2::actionRequestedReplot);
	connect(sWrapper->techDetails, &TechnicalDetailsSettings::calcndlttsChanged            , graphics(), &ProfileWidget2::actionRequestedReplot);
	connect(sWrapper->techDetails, &TechnicalDetailsSettings::hrgraphChanged               , graphics(), &ProfileWidget2::actionRequestedReplot);
	connect(sWrapper->techDetails, &TechnicalDetailsSettings::rulerGraphChanged            , graphics(), &ProfileWidget2::actionRequestedReplot);
	connect(sWrapper->techDetails, &TechnicalDetailsSettings::showSacChanged               , graphics(), &ProfileWidget2::actionRequestedReplot);
	connect(sWrapper->techDetails, &TechnicalDetailsSettings::zoomedPlotChanged            , graphics(), &ProfileWidget2::actionRequestedReplot);
	connect(sWrapper->techDetails, &TechnicalDetailsSettings::showPicturesInProfileChanged , graphics(), &ProfileWidget2::actionRequestedReplot);
	connect(sWrapper->techDetails, &TechnicalDetailsSettings::tankBarChanged               , graphics(), &ProfileWidget2::actionRequestedReplot);
	connect(sWrapper->techDetails, &TechnicalDetailsSettings::percentageGraphChanged       , graphics(), &ProfileWidget2::actionRequestedReplot);

	connect(sWrapper->pp_gas, &PartialPressureGasSettings::showPheChanged, graphics(), &ProfileWidget2::actionRequestedReplot);
	connect(sWrapper->pp_gas, &PartialPressureGasSettings::showPn2Changed, graphics(), &ProfileWidget2::actionRequestedReplot);
	connect(sWrapper->pp_gas, &PartialPressureGasSettings::showPo2Changed, graphics(), &ProfileWidget2::actionRequestedReplot);

	// now let's set up some connections
	connect(graphics(), &ProfileWidget2::enableToolbar ,this, &MainWindow::setEnabledToolbar);
	connect(graphics(), &ProfileWidget2::disableShortcuts, this, &MainWindow::disableShortcuts);
	connect(graphics(), &ProfileWidget2::enableShortcuts, this, &MainWindow::enableShortcuts);
	connect(graphics(), &ProfileWidget2::refreshDisplay, this, &MainWindow::refreshDisplay);
	connect(graphics(), &ProfileWidget2::editCurrentDive, this, &MainWindow::editCurrentDive);
	connect(graphics(), &ProfileWidget2::updateDiveInfo, information(), &MainTab::updateDiveInfo);

	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), graphics(), SLOT(settingsChanged()));

	ui.profCalcAllTissues->setChecked(sWrapper->techDetails->calcalltissues());
	ui.profCalcCeiling->setChecked(sWrapper->techDetails->calcceiling());
	ui.profDcCeiling->setChecked(sWrapper->techDetails->dcceiling());
	ui.profEad->setChecked(sWrapper->techDetails->ead());
	ui.profIncrement3m->setChecked(sWrapper->techDetails->calcceiling3m());
	ui.profMod->setChecked(sWrapper->techDetails->mod());
	ui.profNdl_tts->setChecked(sWrapper->techDetails->calcndltts());
	ui.profPhe->setChecked(sWrapper->pp_gas->showPhe());
	ui.profPn2->setChecked(sWrapper->pp_gas->showPn2());
	ui.profPO2->setChecked(sWrapper->pp_gas->showPo2());
	ui.profHR->setChecked(sWrapper->techDetails->hrgraph());
	ui.profRuler->setChecked(sWrapper->techDetails->rulerGraph());
	ui.profSAC->setChecked(sWrapper->techDetails->showSac());
	ui.profTogglePicture->setChecked(sWrapper->techDetails->showPicturesInProfile());
	ui.profTankbar->setChecked(sWrapper->techDetails->tankBar());
	ui.profTissues->setChecked(sWrapper->techDetails->percentageGraph());
	ui.profScaled->setChecked(sWrapper->techDetails->zoomedPlot());

// full screen support is buggy on Windows and Ubuntu.
// require the FULLSCREEN_SUPPORT macro to enable it!
#ifndef FULLSCREEN_SUPPORT
	ui.actionFullScreen->setEnabled(false);
	ui.actionFullScreen->setVisible(false);
	setWindowState(windowState() & ~Qt::WindowFullScreen);
#endif
}

MainWindow::~MainWindow()
{
	write_hashes();
	m_Instance = NULL;
}

void MainWindow::setupSocialNetworkMenu()
{
#ifdef FBSUPPORT
	connections = new QMenu(tr("Connect to"));
	FacebookPlugin *facebookPlugin = new FacebookPlugin();
	QAction *toggle_connection = new QAction(this);
	QObject *obj = qobject_cast<QObject*>(facebookPlugin);
	toggle_connection->setText(facebookPlugin->socialNetworkName());
	toggle_connection->setIcon(QIcon(facebookPlugin->socialNetworkIcon()));
	toggle_connection->setData(QVariant::fromValue(obj));
	connect(toggle_connection, SIGNAL(triggered()), this, SLOT(socialNetworkRequestConnect()));
	FacebookManager *fb = FacebookManager::instance();
	connect(fb, &FacebookManager::justLoggedIn, this, &MainWindow::facebookLoggedIn);
	connect(fb, &FacebookManager::justLoggedOut, this, &MainWindow::facebookLoggedOut);
	connect(fb, &FacebookManager::sendMessage, [this](const QString& msg) {
		statusBar()->showMessage(msg, 10000); // show message for 10 secs on the statusbar.
	});
	share_on_fb = new QAction(this);
	share_on_fb->setText(facebookPlugin->socialNetworkName());
	share_on_fb->setIcon(QIcon(facebookPlugin->socialNetworkIcon()));
	share_on_fb->setData(QVariant::fromValue(obj));
	share_on_fb->setEnabled(false);
	ui.menuShare_on->addAction(share_on_fb);
	connections->addAction(toggle_connection);
	connect(share_on_fb, SIGNAL(triggered()), this, SLOT(socialNetworkRequestUpload()));
	ui.menuShare_on->addSeparator();
	ui.menuShare_on->addMenu(connections);
	ui.menubar->show();
#endif
}

void MainWindow::facebookLoggedIn()
{
	connections->setTitle(tr("Disconnect from"));
	share_on_fb->setEnabled(true);
}

void MainWindow::facebookLoggedOut()
{
	connections->setTitle(tr("Connect to"));
	share_on_fb->setEnabled(false);
}

void MainWindow::socialNetworkRequestConnect()
{
	qDebug() << "Requesting connect on the social network";
	QAction *action = qobject_cast<QAction*>(sender());
	ISocialNetworkIntegration *plugin = qobject_cast<ISocialNetworkIntegration*>(action->data().value<QObject*>());
	if (plugin->isConnected())
		plugin->requestLogoff();
	else
		plugin->requestLogin();
}

void MainWindow::socialNetworkRequestUpload()
{
	QAction *action = qobject_cast<QAction*>(sender());
	ISocialNetworkIntegration *plugin = action->data().value<ISocialNetworkIntegration*>();
	plugin->requestUpload();
}

void MainWindow::setStateProperties(const QByteArray& state, const PropertyList& tl, const PropertyList& tr, const PropertyList& bl, const PropertyList& br)
{
	stateProperties[state] = PropertiesForQuadrant(tl, tr, bl, br);
}

void MainWindow::on_actionDiveSiteEdit_triggered() {
	setApplicationState("EditDiveSite");
}

void MainWindow::enableDisableCloudActions()
{
	ui.actionCloudstorageopen->setEnabled(prefs.cloud_verification_status == CS_VERIFIED);
	ui.actionCloudstoragesave->setEnabled(prefs.cloud_verification_status == CS_VERIFIED);
	ui.actionTake_cloud_storage_online->setEnabled(prefs.cloud_verification_status == CS_VERIFIED && prefs.git_local_only);
}

PlannerDetails *MainWindow::plannerDetails() const {
	return qobject_cast<PlannerDetails*>(applicationState["PlanDive"].bottomRight);
}

PlannerSettingsWidget *MainWindow::divePlannerSettingsWidget() {
	return qobject_cast<PlannerSettingsWidget*>(applicationState["PlanDive"].bottomLeft);
}

void MainWindow::setDefaultState() {
	setApplicationState("Default");
	if (information()->getEditMode() != MainTab::NONE) {
		ui.bottomLeft->currentWidget()->setEnabled(false);
	}
}

MainWindow *MainWindow::instance()
{
	return m_Instance;
}

// this gets called after we download dives from a divecomputer
void MainWindow::refreshDisplay(bool doRecreateDiveList)
{
	information()->reload();
	TankInfoModel::instance()->update();
	MapWidget::instance()->reload();
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

void MainWindow::configureToolbar() {
	if (selected_dive>0) {
		if (current_dive->dc.divemode == FREEDIVE) {
			ui.profCalcCeiling->setDisabled(true);
			ui.profCalcAllTissues ->setDisabled(true);
			ui.profIncrement3m->setDisabled(true);
			ui.profDcCeiling->setDisabled(true);
			ui.profPhe->setDisabled(true);
			ui.profPn2->setDisabled(true); //TODO is the same as scuba?
			ui.profPO2->setDisabled(true); //TODO is the same as scuba?
			ui.profRuler->setDisabled(false);
			ui.profScaled->setDisabled(false); // measuring and scaling
			ui.profTogglePicture->setDisabled(false);
			ui.profTankbar->setDisabled(true);
			ui.profMod->setDisabled(true);
			ui.profNdl_tts->setDisabled(true);
			ui.profEad->setDisabled(true);
			ui.profSAC->setDisabled(true);
			ui.profHR->setDisabled(false);
			ui.profTissues->setDisabled(true);
		} else {
			ui.profCalcCeiling->setDisabled(false);
			ui.profCalcAllTissues ->setDisabled(false);
			ui.profIncrement3m->setDisabled(false);
			ui.profDcCeiling->setDisabled(false);
			ui.profPhe->setDisabled(false);
			ui.profPn2->setDisabled(false);
			ui.profPO2->setDisabled(false); // partial pressure graphs
			ui.profRuler->setDisabled(false);
			ui.profScaled->setDisabled(false); // measuring and scaling
			ui.profTogglePicture->setDisabled(false);
			ui.profTankbar->setDisabled(false);
			ui.profMod->setDisabled(false);
			ui.profNdl_tts->setDisabled(false); // various values that a user is either interested in or not
			ui.profEad->setDisabled(false);
			ui.profSAC->setDisabled(false);
			ui.profHR->setDisabled(false); // very few dive computers support this
			ui.profTissues->setDisabled(false);; // maybe less frequently used
		}
	}
}

void MainWindow::current_dive_changed(int divenr)
{
	if (divenr >= 0) {
		select_dive(divenr);
	}
	graphics()->plotDive();
	information()->updateDiveInfo();
	configureToolbar();
	MapWidget::instance()->reload();
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
	if (getCloudURL(filename))
		return;

	if (verbose)
		qDebug() << "Opening cloud storage from:" << filename;

	closeCurrentFile();

	int error;

	showProgressBar();
	QByteArray fileNamePtr = QFile::encodeName(filename);
	error = parse_file(fileNamePtr.data());
	if (!error) {
		set_filename(fileNamePtr.data(), true);
		setTitle(MWTF_FILENAME);
	}
	getNotificationWidget()->hideNotification();
	process_dives(false, false);
	hideProgressBar();
	refreshDisplay();
	ui.actionAutoGroup->setChecked(autogroup);
}

void MainWindow::on_actionCloudstoragesave_triggered()
{
	QString filename;
	if (!dive_table.nr) {
		report_error(qPrintable(tr("Don't save an empty log to the cloud")));
		return;
	}
	if (getCloudURL(filename))
		return;

	if (verbose)
		qDebug() << "Saving cloud storage to:" << filename;
	if (information()->isEditing())
		information()->acceptChanges();

	showProgressBar();

	if (save_dives(filename.toUtf8().data()))
		return;

	hideProgressBar();

	set_filename(filename.toUtf8().data(), true);
	setTitle(MWTF_FILENAME);
	mark_divelist_changed(false);
}

void MainWindow::on_actionTake_cloud_storage_online_triggered()
{
	prefs.git_local_only = false;
	ui.actionTake_cloud_storage_online->setEnabled(false);
}

void learnImageDirs(QStringList dirnames)
{
	QList<QFuture<void> > futures;
	foreach (QString dir, dirnames) {
		futures << QtConcurrent::run(learnImages, QDir(dir), 10);
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
	information()->clearTabs();
	information()->updateDiveInfo(true);
	graphics()->setEmptyState();
	dive_list()->reload(DiveTripModel::TREE);
	MapWidget::instance()->reload();
	if (!existing_filename)
		setTitle(MWTF_DEFAULT);
	disableShortcuts();
}

bool MainWindow::okToClose(QString message)
{
	if (DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING ||
		information()->isEditing() ) {
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
	clear_dive_file_data();
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
	PreferencesDialog::instance()->raise();
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
	if (progressDialog)
		// we are accessing the cloud, so let's not switch into Add or Plan mode
		return false;

	if (DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING ||
		information()->isEditing()) {
		QMessageBox::warning(this, tr("Warning"), tr("Please save or cancel the current dive edit before trying to add a dive."));
		return false;
	}
	return true;
}

void MainWindow::refreshProfile()
{
	showProfile();
	configureToolbar();
	graphics()->replot(get_dive(selected_dive));
	DivePictureModel::instance()->updateDivePictures();
}

void MainWindow::planCanceled()
{
	// while planning we might have modified the displayed_dive
	// let's refresh what's shown on the profile
	refreshProfile();
	refreshDisplay(false);
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

	/* render the profile as a pixmap that is inserted as base64 data into a HTML <img> tag
	 * make it fit a page width defined by 2 cm margins via QTextDocument->print() (cannot be changed?)
	 * the height of the profile is 40% of the page height.
	 */
	QSizeF renderSize = printer.pageRect(QPrinter::Inch).size();
	const qreal marginsInch = 1.57480315; // = (2 x 2cm) / 2.45cm/inch
	renderSize.setWidth((renderSize.width() - marginsInch) * printer.resolution());
	renderSize.setHeight(((renderSize.height() - marginsInch) * printer.resolution()) / 2.5);

	QPixmap pixmap(renderSize.toSize());
	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	ProfileWidget2 *profile = graphics();
	QSize origSize = profile->size();
	profile->resize(renderSize.toSize());
	profile->setPrintMode(true);
	profile->render(&painter);
	profile->resize(origSize);
	profile->setPrintMode(false);

	QByteArray byteArray;
	QBuffer buffer(&byteArray);
	pixmap.save(&buffer, "PNG");
	QString profileImage = QString("<img src=\"data:image/png;base64,") + byteArray.toBase64() + "\"/><br><br>";
	withDisclaimer = profileImage + withDisclaimer;

	plannerDetails()->divePlanOutput()->setHtml(withDisclaimer);
	plannerDetails()->divePlanOutput()->print(&printer);
	plannerDetails()->divePlanOutput()->setHtml(displayed_dive.notes);
#endif
}

void MainWindow::setupForAddAndPlan(const char *model)
{
	// clean out the dive and give it an id and the correct dc model
	clear_dive(&displayed_dive);
	clear_dive_site(&displayed_dive_site);
	displayed_dive.id = dive_getUniqID(&displayed_dive);
	displayed_dive.when = QDateTime::currentMSecsSinceEpoch() / 1000L + gettimezoneoffset() + 3600;
	displayed_dive.dc.model = strdup(model); // don't translate! this is stored in the XML file
	dc_number = 1;
	// setup the dive cylinders
	DivePlannerPointsModel::instance()->clear();
	DivePlannerPointsModel::instance()->setupCylinders();

}

void MainWindow::on_actionReplanDive_triggered()
{
	if (!plannerStateClean() || !current_dive)
		return;
	else if (!current_dive->dc.model || strcmp(current_dive->dc.model, "planned dive")) {
		if (QMessageBox::warning(this, tr("Warning"), tr("Trying to replan a dive that's not a planned dive."),
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
	divePlannerWidget()->setupStartTime(QDateTime::fromMSecsSinceEpoch(1000 * current_dive->when, Qt::UTC));
	if (current_dive->surface_pressure.mbar)
		divePlannerWidget()->setSurfacePressure(current_dive->surface_pressure.mbar);
	if (current_dive->salinity)
		divePlannerWidget()->setSalinity(current_dive->salinity);
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
	// plan the dive in the same mode as the currently selected one
	if (current_dive) {
		divePlannerSettingsWidget()->setDiveMode(current_dive->dc.divemode);
		if (current_dive->salinity)
			divePlannerWidget()->setSalinity(current_dive->salinity);
	}
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
	configureToolbar();
	graphics()->plotDive();
	fixup_dc_duration(&displayed_dive.dc);
	displayed_dive.duration = displayed_dive.dc.duration;

	// now that we have the correct depth and duration, update the dive info
	information()->updateDepthDuration();
}

void MainWindow::on_actionEditDive_triggered()
{
	if (information()->isEditing() || DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING) {
		QMessageBox::warning(this, tr("Warning"), tr("Please, first finish the current edition before trying to do another."));
		return;
	}

	const bool isTripEdit = dive_list()->selectedTrips().count() >= 1;
	if (!current_dive || isTripEdit || (current_dive->dc.model && strcmp(current_dive->dc.model, "manually added dive"))) {
		QMessageBox::warning(this, tr("Warning"), tr("Trying to edit a dive that's not a manually added dive."));
		return;
	}

	DivePlannerPointsModel::instance()->clear();
	disableShortcuts();
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::ADD);
	graphics()->setAddState();
	MapWidget::instance()->endGetDiveCoordinates();
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
	d.resize(lrint(width() * .8), height() / 2);
	d.move(lrint(width() * .1), height() / 4);
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), &d);
	connect(close, SIGNAL(activated()), &d, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), &d);
	connect(quit, SIGNAL(activated()), this, SLOT(close()));
	d.setWindowFlags(Qt::Window | Qt::CustomizeWindowHint
		| Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
	d.setWindowTitle(tr("Yearly statistics"));
	d.setWindowIcon(QIcon(":/subsurface-icon"));
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

void MainWindow::on_actionViewMap_triggered()
{
	TOGGLE_COLLAPSABLE( true );
	beginChangeState(MAP_MAXIMIZED);
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
		mainSizes.append(lrint(appH * 0.7));
		mainSizes.append(lrint(appH * 0.3));
	}
	static QList<int> infoProfileSizes;
	if (infoProfileSizes.empty()) {
		infoProfileSizes.append(lrint(appW * 0.3));
		infoProfileSizes.append(lrint(appW * 0.7));
	}

	static QList<int> listGlobeSizes;
	if (listGlobeSizes.empty()) {
		listGlobeSizes.append(lrint(appW * 0.7));
		listGlobeSizes.append(lrint(appW * 0.3));
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
	configureToolbar();
	graphics()->plotDive();
	information()->updateDiveInfo();
}

void MainWindow::on_actionNextDC_triggered()
{
	unsigned nrdc = number_of_computers(current_dive);
	dc_number = (dc_number + 1) % nrdc;
	configureToolbar();
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
	f += "*.sql *.SQL " ;
	f += "*.dld *.DLD ";
	f += "*.jlb *.JLB ";
	f += "*.lvd *.LVD ";
	f += "*.sde *.SDE ";
	f += "*.udcf *.UDCF ";
	f += "*.uddf *.UDDF ";
	f += "*.xml *.XML ";
	f += "*.dlf *.DLF ";
	f += "*.log *.LOG ";
	f += "*.txt *.TXT) ";
	f += "*.apd *.APD) ";
	f += "*.dive *.DIVE ";
	f += "*.zxu *.zxl *.ZXU *.ZXL ";
	f += ");;";

	f += "Subsurface (*.ssrf);;";
	f += "Cochran (*.can *.CAN);;";
	f += "DiveLogs.de (*.dld *.DLD);;";
	f += "JDiveLog  (*.jlb *.JLB);;";
	f += "Liquivision (*.lvd *.LVD);;";
	f += "Suunto (*.sde *.SDE *.db *.DB);;";
	f += "UDCF (*.udcf *.UDCF);;";
	f += "UDDF (*.uddf *.UDDF);;";
	f += "XML (*.xml *.XML);;";
	f += "Divesoft (*.dlf *.DLF);;";
	f += "Datatrak/WLog Files (*.log *.LOG);;";
	f += "MkVI files (*.txt *.TXT);;";
	f += "APD log viewer (*.apd *.APD);;";
	f += "OSTCtools Files (*.dive *.DIVE);;";
	f += "DAN DL7 (*.zxu *.zxl *.ZXU *.ZXL)";

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
	if (settings.value("maximized", isMaximized()).value<bool>()) {
		showMaximized();
	} else {
		restoreGeometry(settings.value("geometry").toByteArray());
		restoreState(settings.value("windowState", 0).toByteArray());
	}

	state = (CurrentState)settings.value("lastState", 0).toInt();
	switch (state) {
	case VIEWALL:
		on_actionViewAll_triggered();
		break;
	case MAP_MAXIMIZED:
		on_actionViewMap_triggered();
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
	show();
}

void MainWindow::readSettings()
{
	static bool firstRun = true;
	init_proxy();

	// now make sure that the cloud menu items are enabled IFF cloud account is verified
	enableDisableCloudActions();

#if !defined(SUBSURFACE_MOBILE)
	QSettings s; //TODO: this 's' exists only for the loadRecentFiles, remove it.

	loadRecentFiles(&s);
	if (firstRun) {
		checkSurvey(&s);
		firstRun = false;
	}
#endif
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
	settings.setValue("geometry", saveGeometry());
	settings.setValue("windowState", saveState());
	settings.setValue("maximized", isMaximized());
	settings.setValue("lastState", (int)state);
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

	// if the default is to save to cloud storage, pick something that will work as local file:
	// simply extract the branch name which should be the users email address
	if (default_filename && strstr(default_filename, prefs.cloud_git_url)) {
		QString filename(default_filename);
		filename.remove(prefs.cloud_git_url);
		filename.remove(0, filename.indexOf("[") + 1);
		filename.replace("]", ".ssrf");
		default_filename = strdup(qPrintable(filename));
	}
	// create a file dialog that allows us to save to a new file
	QFileDialog selection_dialog(this, tr("Save file as"), default_filename,
					 tr("Subsurface XML files (*.ssrf *.xml *.XML)"));
	selection_dialog.setAcceptMode(QFileDialog::AcceptSave);
	selection_dialog.setFileMode(QFileDialog::AnyFile);
	selection_dialog.setDefaultSuffix("");
	if (same_string(default_filename, "")) {
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

	if (information()->isEditing())
		information()->acceptChanges();

	if (save_dives(filename.toUtf8().data()))
		return -1;

	set_filename(filename.toUtf8().data(), true);
	setTitle(MWTF_FILENAME);
	mark_divelist_changed(false);
	addRecentFile(QStringList() << filename);
	return 0;
}

int MainWindow::file_save(void)
{
	const char *current_default;
	bool is_cloud = false;

	if (!existing_filename)
		return file_save_as();

	is_cloud = (strncmp(existing_filename, "http", 4) == 0);

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
	if (is_cloud)
		showProgressBar();
	if (save_dives(existing_filename)) {
		if (is_cloud)
			hideProgressBar();
		return -1;
	}
	if (is_cloud)
		hideProgressBar();
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

	if (fullFilename.contains(prefs.cloud_git_url)) {
		QString email = fileName.left(fileName.indexOf('['));
		if (prefs.git_local_only) {
			ui.actionTake_cloud_storage_online->setEnabled(true);
			return tr("[local cache for] %1").arg(email);
		} else {
			return tr("[cloud storage for] %1").arg(email);
		}
	} else {
		return fileName;
	}
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
	QStringList csvFiles;

	if (fileNames.isEmpty())
		return;

	QByteArray fileNamePtr, csv;

	for (int i = 0; i < fileNames.size(); ++i) {
		fileNamePtr = QFile::encodeName(fileNames.at(i));
		csv = fileNamePtr.data();
		csv.replace(strlen(csv.data()) - 3, 3, "csv");

		QFileInfo check_file(csv);
		if (check_file.exists() && check_file.isFile()) {
			if (parse_txt_file(fileNamePtr.data(), csv) == 0)
				csvFiles += fileNames.at(i);
		} else {
			csvFiles += fileNamePtr;
		}
	}
	if (csvFiles.size()) {
		DiveLogImportDialog *diveLogImport = new DiveLogImportDialog(csvFiles, this);
		diveLogImport->show();
	}
	process_dives(true, false);
	refreshDisplay();
}

void MainWindow::loadFiles(const QStringList fileNames)
{
	if (fileNames.isEmpty()) {
		refreshDisplay();
		return;
	}
	QByteArray fileNamePtr;
	QStringList failedParses;

	showProgressBar();
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
	hideProgressBar();
	process_dives(false, false);
	addRecentFile(fileNames);
	removeRecentFile(failedParses);

	refreshDisplay();
	ui.actionAutoGroup->setChecked(autogroup);

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

void MainWindow::on_actionImportDiveLog_triggered()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open dive log file"), lastUsedDir(),
		tr("Dive log files (*.ssrf *.can *.csv *.db *.sql *.dld *.jlb *.lvd *.sde *.udcf *.uddf *.xml *.txt *.dlf *.apd *.zxu *.zxl"
			"*.SSRF *.CAN *.CSV *.DB *.SQL *.DLD *.JLB *.LVD *.SDE *.UDCF *.UDDF *.xml *.TXT *.DLF *.APD *.ZXU *.ZXL);;"
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
			"DAN DL7 (*.zxu *.zxl *.ZXU *.ZXL);;"
			"All files (*)"));

	if (fileNames.isEmpty())
		return;
	updateLastUsedDir(QFileInfo(fileNames[0]).dir().path());

	QStringList logFiles = fileNames.filter(QRegExp("^(?!.*\\.(csv|txt|apd|zxu|zxl))", Qt::CaseInsensitive));
	QStringList csvFiles = fileNames.filter(".csv", Qt::CaseInsensitive);
	csvFiles += fileNames.filter(".apd", Qt::CaseInsensitive);
	csvFiles += fileNames.filter(".zxu", Qt::CaseInsensitive);
	csvFiles += fileNames.filter(".zxl", Qt::CaseInsensitive);
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

void MainWindow::turnOffNdlTts()
{
	SettingsObjectWrapper::instance()->techDetails->setCalcndltts(false);
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
	if (ui.multiFilter->isVisible()) {
		ui.multiFilter->closeFilter();
		ui.actionFilterTags->setChecked(false);
	} else {
		ui.multiFilter->setVisible(true);
		ui.actionFilterTags->setChecked(true);
	}
}

void MainWindow::setCheckedActionFilterTags(bool checked)
{
	ui.actionFilterTags->setChecked(checked);
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

	if (getCurrentAppState() == state)
		return;

	setCurrentAppState(state);

#define SET_CURRENT_INDEX( X ) \
	if (applicationState[state].X) { \
		ui.X->setCurrentWidget( applicationState[state].X); \
		ui.X->show(); \
	} else { \
		ui.X->hide(); \
	}

	SET_CURRENT_INDEX( topLeft )
	Q_FOREACH(const WidgetProperty& p, stateProperties[state].topLeft) {
		ui.topLeft->currentWidget()->setProperty( p.first.data(), p.second);
	}
	SET_CURRENT_INDEX( topRight )
	Q_FOREACH(const WidgetProperty& p, stateProperties[state].topRight) {
		ui.topRight->currentWidget()->setProperty( p.first.data(), p.second);
	}
	SET_CURRENT_INDEX( bottomLeft )
	Q_FOREACH(const WidgetProperty& p, stateProperties[state].bottomLeft) {
		ui.bottomLeft->currentWidget()->setProperty( p.first.data(), p.second);
	}
	SET_CURRENT_INDEX( bottomRight )
	Q_FOREACH(const WidgetProperty& p, stateProperties[state].bottomRight) {
		ui.bottomRight->currentWidget()->setProperty( p.first.data(), p.second);
	}
#undef SET_CURRENT_INDEX
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
		progressDialog->deleteLater();
		progressDialog = NULL;
	}
}
