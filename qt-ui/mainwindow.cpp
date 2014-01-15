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
#include "divelistview.h"
#include "starwidget.h"

#include "../dive.h"
#include "../divelist.h"
#include "../pref.h"
#include "../helpers.h"
#include "modeldelegates.h"
#include "models.h"
#include "downloadfromdivecomputer.h"
#include "preferences.h"
#include "subsurfacewebservices.h"
#include "divecomputermanagementdialog.h"
#include "simplewidgets.h"
#include "diveplanner.h"
#include "about.h"
#include "printdialog.h"
#include "divelogimportdialog.h"

static MainWindow* instance = 0;

MainWindow* mainWindow()
{
	return instance;
}

MainWindow::MainWindow() : helpView(0)
{
	instance = this;
	ui.setupUi(this);
	setWindowIcon(QIcon(":subsurface-icon"));
	connect(ui.ListWidget, SIGNAL(currentDiveChanged(int)), this, SLOT(current_dive_changed(int)));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), this, SLOT(readSettings()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), ui.ListWidget, SLOT(update()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), ui.ListWidget, SLOT(reloadHeaderActions()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), ui.ProfileWidget, SLOT(refresh()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), ui.InfoWidget, SLOT(updateDiveInfo()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), ui.divePlanner, SLOT(settingsChanged()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), ui.divePlannerWidget, SLOT(settingsChanged()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), TankInfoModel::instance(), SLOT(update()));

	ui.mainErrorMessage->hide();
	initialUiSetup();
	readSettings();
	ui.ListWidget->reload(DiveTripModel::TREE);
	ui.ListWidget->reloadHeaderActions();
	ui.ListWidget->setFocus();
	ui.globe->reload();
	ui.ListWidget->expand(ui.ListWidget->model()->index(0,0));
	ui.ListWidget->scrollTo(ui.ListWidget->model()->index(0,0), QAbstractItemView::PositionAtCenter);
	ui.divePlanner->settingsChanged();
	ui.divePlannerWidget->settingsChanged();
}

// this gets called after we download dives from a divecomputer
void MainWindow::refreshDisplay(bool recreateDiveList)
{
	ui.InfoWidget->reload();
	TankInfoModel::instance()->update();
	ui.ProfileWidget->refresh();
	ui.globe->reload();
	if (recreateDiveList)
		ui.ListWidget->reload(DiveTripModel::CURRENT);
	ui.ListWidget->setFocus();
	WSInfoModel::instance()->updateInfo();
}

void MainWindow::current_dive_changed(int divenr)
{
	if (divenr >= 0) {
		select_dive(divenr);
		ui.globe->centerOn(get_dive(selected_dive));
	}
	redrawProfile();

	/* It looks like it's a bit too cumberstone to send *one* dive using a QList,
	 * but this is just futureproofness, it's the best way in the future to show more than
	 * a single profile plot on the canvas. I know that we are using only one right now,
	 * but let's keep like this so it's easy to change when we need? :)
	 */
	ui.graphicsView->plotDives( QList<dive*>() << (current_dive) );
	ui.InfoWidget->updateDiveInfo(divenr);
}

void MainWindow::redrawProfile()
{
	ui.ProfileWidget->refresh();
}

void MainWindow::on_actionNew_triggered()
{
	on_actionClose_triggered();
}

void MainWindow::on_actionOpen_triggered()
{
	if (DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING ||
	    ui.InfoWidget->isEditing()) {
		QMessageBox::warning(this, tr("Warning"), tr("Please save or cancel the current dive edit before opening a new file."));
		return;
	}
	QString filename = QFileDialog::getOpenFileName(this, tr("Open File"), lastUsedDir(), filter());
	if (filename.isEmpty())
		return;
	updateLastUsedDir(QFileInfo(filename).dir().path());
	on_actionClose_triggered();
	loadFiles( QStringList() << filename );
}

QTabWidget *MainWindow::tabWidget()
{
	return ui.tabWidget;
}
void MainWindow::on_actionSave_triggered()
{
	file_save();
}

void MainWindow::on_actionSaveAs_triggered()
{
	file_save_as();
}

void MainWindow::cleanUpEmpty()
{
	ui.InfoWidget->clearStats();
	ui.InfoWidget->clearInfo();
	ui.InfoWidget->clearEquipment();
	ui.InfoWidget->updateDiveInfo(-1);
	ui.ProfileWidget->clear();
	ui.ListWidget->reload(DiveTripModel::TREE);
	ui.globe->reload();
	if (!existing_filename)
		setTitle(MWTF_DEFAULT);
}

void MainWindow::on_actionClose_triggered()
{
	if (DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING ||
	    ui.InfoWidget->isEditing()) {
		QMessageBox::warning(this, tr("Warning"), tr("Please save or cancel the current dive edit before closing the file."));
		return;
	}
	if (unsaved_changes() && (askSaveChanges() == false))
		return;

	/* free the dives and trips */
	while (dive_table.nr)
		delete_single_dive(0);

	dive_list()->clearSelection();
	/* clear the selection and the statistics */
	selected_dive = -1;

	if (existing_filename) {
		free((void *)existing_filename);
		existing_filename = NULL;
	}
	cleanUpEmpty();
	mark_divelist_changed(false);

	clear_events();
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

void MainWindow::updateLastUsedDir(const QString& dir)
{
	QSettings s;
	s.beginGroup("FileDialog");
	s.setValue("LastDir", dir);
}

void MainWindow::on_actionExportUDDF_triggered()
{
	QFileInfo fi(system_default_filename());
	QString filename = QFileDialog::getSaveFileName(this, tr("Save File as"), fi.absolutePath(),
							tr("UDDF files (*.uddf *.UDDF)"));
	if (!filename.isNull() && !filename.isEmpty())
		export_dives_uddf(filename.toUtf8(), false);
}

void MainWindow::on_actionPrint_triggered()
{
	PrintDialog::instance()->runDialog();
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

void MainWindow::on_actionDivePlanner_triggered()
{
	if (DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING ||
	    ui.InfoWidget->isEditing()) {
		QMessageBox::warning(this, tr("Warning"), tr("Please save or cancel the current dive edit before trying to plan a dive."));
		return;
	}
	disableDcShortcuts();
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::PLAN);
	DivePlannerPointsModel::instance()->clear();
	CylindersModel::instance()->clear();
	ui.stackedWidget->setCurrentIndex(PLANNERPROFILE);
	ui.infoPane->setCurrentIndex(PLANNERWIDGET);
}

void MainWindow::showProfile()
{
	enableDcShortcuts();
	ui.stackedWidget->setCurrentIndex(PROFILE);
	ui.infoPane->setCurrentIndex(MAINTAB);
}

void MainWindow::on_actionPreferences_triggered()
{
	PreferencesDialog::instance()->show();
}

void MainWindow::on_actionQuit_triggered()
{
	if (DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING ||
	    ui.InfoWidget->isEditing()) {
		QMessageBox::warning(this, tr("Warning"), tr("Please save or cancel the current dive edit before closing the file."));
		return;
	}
	if (unsaved_changes() && (askSaveChanges() == false))
		return;
	writeSettings();
	QApplication::quit();
}

void MainWindow::on_actionDownloadDC_triggered()
{
	DownloadFromDCWidget* downloadWidget = DownloadFromDCWidget::instance();
	downloadWidget->runDialog();
}

void MainWindow::on_actionDownloadWeb_triggered()
{
	SubsurfaceWebServices::instance()->exec();
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

void MainWindow::on_actionAddDive_triggered()
{
	if (DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING ||
	    ui.InfoWidget->isEditing()) {
		QMessageBox::warning(this, tr("Warning"), tr("Please save or cancel the current dive edit before trying to add a dive."));
		return;
	}
	dive_list()->rememberSelection();
	dive_list()->unselectDives();
	disableDcShortcuts();
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::ADD);

	// now cheat - create one dive that we use to store the info tab data in
	struct dive *dive = alloc_dive();
	dive->when = QDateTime::currentMSecsSinceEpoch() / 1000L + gettimezoneoffset();
	dive->dc.model = "manually added dive"; // don't translate! this is stored in the XML file

	dive->latitude.udeg = 0;
	dive->longitude.udeg = 0;
	record_dive(dive);
	// this isn't in the UI yet, so let's call the C helper function - we'll fix this up when
	// accepting the dive
	select_dive(get_divenr(dive));
	ui.InfoWidget->setCurrentIndex(0);
	ui.InfoWidget->updateDiveInfo(selected_dive);
	ui.InfoWidget->addDiveStarted();
	ui.stackedWidget->setCurrentIndex(PLANNERPROFILE); // Planner.
	ui.infoPane->setCurrentIndex(MAINTAB);
	DivePlannerPointsModel::instance()->clear();
	DivePlannerPointsModel::instance()->createSimpleDive();
	ui.ListWidget->reload(DiveTripModel::CURRENT);
}

void MainWindow::on_actionRenumber_triggered()
{
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

void MainWindow::on_actionToggleZoom_triggered()
{
	zoomed_plot = !zoomed_plot;
	ui.ProfileWidget->refresh();
}

void MainWindow::on_actionYearlyStatistics_triggered()
{
	QTreeView *view = new QTreeView();
	QAbstractItemModel *model = new YearlyStatisticsModel();
	view->setModel(model);
	view->setWindowModality(Qt::NonModal);
	view->setMinimumWidth(600);
	view->setAttribute(Qt::WA_QuitOnClose, false);
	view->setWindowTitle(tr("Yearly Statistics"));
	view->setWindowIcon(QIcon(":subsurface-icon"));
	view->show();
}

void MainWindow::on_mainSplitter_splitterMoved(int pos, int idx)
{
	redrawProfile();
}

void MainWindow::on_infoProfileSplitter_splitterMoved(int pos, int idx)
{
	redrawProfile();
}

/**
 * So, here's the deal.
 * We have a few QSplitters that takes care of helping us with the
 * size of a few widgets, they are ok, and we should continue using them
 * to manage the visibility of them too. But the way that we did before was to
 * widget->hide(); something, and if you hided something using the splitter,
 * by holding it's handle and collapsing the widget, then you used the 'ctrl+number'
 * shortcut to show it, it whould only show a gray panel.
 *
 * This patch makes everything behave using the splitters.
 */

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
	redrawProfile();
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
	redrawProfile();
}

void MainWindow::beginChangeState(CurrentState s) {
	if (state == VIEWALL && state != s) {
		saveSplitterSizes();
	}
	state = s;
}

void MainWindow::saveSplitterSizes() {
	QSettings settings;
	settings.beginGroup("MainWindow");
	settings.setValue("mainSplitter", ui.mainSplitter->saveState());
	settings.setValue("infoProfileSplitter", ui.infoProfileSplitter->saveState());
	settings.setValue("listGlobeSplitter", ui.listGlobeSplitter->saveState());
}

void MainWindow::on_actionPreviousDC_triggered()
{
	dc_number--;
	ui.InfoWidget->updateDiveInfo(selected_dive);
	redrawProfile();
}

void MainWindow::on_actionNextDC_triggered()
{
	dc_number++;
	ui.InfoWidget->updateDiveInfo(selected_dive);
	redrawProfile();
}

void MainWindow::on_actionFullScreen_triggered(bool checked)
{
	if (checked) {
		setWindowState(windowState() | Qt::WindowFullScreen);
	} else {
		setWindowState(windowState() & ~Qt::WindowFullScreen);
	}
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
	SubsurfaceAbout::instance()->show();
}

void MainWindow::on_actionUserManual_triggered()
{
	if (!helpView) {
		helpView = new UserManual();
	}
	helpView->show();
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
	QMessageBox response;

	if (existing_filename)
		message = tr("Do you want to save the changes you made in the file %1?").arg(existing_filename);
	else
		message = tr("Do you want to save the changes you made in the datafile?");

	response.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
	response.setDefaultButton(QMessageBox::Save);
	response.setText(message);
	response.setWindowTitle(tr("Save Changes?")); // Not displayed on MacOSX as described in Qt API
	response.setInformativeText(tr("Changes will be lost if you don't save them."));
	response.setIcon(QMessageBox::Warning);
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

#define GET_UNIT(name, field, f, t)				\
	v = s.value(QString(name));				\
	if (v.isValid())					\
		prefs.units.field = (v.toInt() == (t)) ? (t) : (f); \
	else							\
		prefs.units.field = default_prefs.units.field

#define GET_BOOL(name, field)					\
	v = s.value(QString(name));				\
	if (v.isValid())					\
		prefs.field = v.toInt() ? true : false;		\
	else							\
		prefs.field = default_prefs.field

#define GET_DOUBLE(name, field)					\
	v = s.value(QString(name));				\
	if (v.isValid())					\
		prefs.field = v.toDouble();			\
	else							\
		prefs.field = default_prefs.field

#define GET_INT(name, field)					\
	v = s.value(QString(name));				\
	if (v.isValid())					\
		prefs.field = v.toInt();			\
	else							\
		prefs.field = default_prefs.field

#define GET_TXT(name, field)					\
	v = s.value(QString(name));				\
	if (v.isValid())					\
		prefs.field = strdup(v.toString().toUtf8().constData());			\
	else							\
		prefs.field = default_prefs.field

#define GET_TXT(name, field)					\
	v = s.value(QString(name));				\
	if (v.isValid())					\
		prefs.field = strdup(v.toString().toUtf8().constData());			\
	else							\
		prefs.field = default_prefs.field


void MainWindow::initialUiSetup()
{
	QSettings settings;
	settings.beginGroup("MainWindow");
	QSize sz = settings.value("size", qApp->desktop()->size()).value<QSize>();
	if (settings.value("maximized", isMaximized()).value<bool>())
		showMaximized();
	else
		resize(sz);

	state = (CurrentState) settings.value("lastState", 0).toInt();
	switch (state) {
		case VIEWALL: on_actionViewAll_triggered(); break;
		case GLOBE_MAXIMIZED : on_actionViewGlobe_triggered(); break;
		case INFO_MAXIMIZED : on_actionViewInfo_triggered(); break;
		case LIST_MAXIMIZED : on_actionViewList_triggered(); break;
		case PROFILE_MAXIMIZED : on_actionViewProfile_triggered(); break;
	}
	settings.endGroup();
}

void MainWindow::readSettings()
{
	QVariant v;
	QSettings s;

	s.beginGroup("Units");
	if (s.value("unit_system").toString() == "metric") {
		prefs.unit_system = METRIC;
		prefs.units = SI_units;
	} else if (s.value("unit_system").toString() == "imperial") {
		prefs.unit_system = IMPERIAL;
		prefs.units = IMPERIAL_units;
	} else {
		prefs.unit_system = PERSONALIZE;
		GET_UNIT("length", length, units::FEET, units::METERS);
		GET_UNIT("pressure", pressure, units::PSI, units::BAR);
		GET_UNIT("volume", volume, units::CUFT, units::LITER);
		GET_UNIT("temperature", temperature, units::FAHRENHEIT, units::CELSIUS);
		GET_UNIT("weight", weight, units::LBS, units::KG);
	}
	GET_UNIT("vertical_speed_time", vertical_speed_time, units::MINUTES, units::SECONDS);
	s.endGroup();
	s.beginGroup("TecDetails");
	GET_BOOL("po2graph", pp_graphs.po2);
	GET_BOOL("pn2graph", pp_graphs.pn2);
	GET_BOOL("phegraph", pp_graphs.phe);
	GET_DOUBLE("po2threshold", pp_graphs.po2_threshold);
	GET_DOUBLE("pn2threshold", pp_graphs.pn2_threshold);
	GET_DOUBLE("phethreshold", pp_graphs.phe_threshold);
	GET_BOOL("mod", mod);
	GET_DOUBLE("modppO2", mod_ppO2);
	GET_BOOL("ead", ead);
	GET_BOOL("redceiling", profile_red_ceiling);
	GET_BOOL("dcceiling", profile_dc_ceiling);
	GET_BOOL("calcceiling", profile_calc_ceiling);
	GET_BOOL("calcceiling3m", calc_ceiling_3m_incr);
	GET_BOOL("calcndltts", calc_ndl_tts);
	GET_BOOL("calcalltissues", calc_all_tissues);
	GET_INT("gflow", gflow);
	GET_INT("gfhigh", gfhigh);
	GET_BOOL("gf_low_at_maxdepth", gf_low_at_maxdepth);
	set_gf(prefs.gflow, prefs.gfhigh, prefs.gf_low_at_maxdepth);
	GET_BOOL("show_sac", show_sac);
	GET_BOOL("display_unused_tanks", display_unused_tanks);
	s.endGroup();

	s.beginGroup("GeneralSettings");
	GET_TXT("default_filename", default_filename);
	GET_TXT("default_cylinder", default_cylinder);
	s.endGroup();

	s.beginGroup("Display");
	QFont defaultFont = s.value("divelist_font", qApp->font()).value<QFont>();
	defaultFont.setPointSizeF(s.value("font_size", qApp->font().pointSizeF()).toFloat());
	qApp->setFont(defaultFont);
	GET_TXT("divelist_font", divelist_font);
	GET_INT("font_size", font_size);
	GET_INT("displayinvalid", display_invalid_dives);
	s.endGroup();
}

void MainWindow::writeSettings()
{
	QSettings settings;

	settings.beginGroup("MainWindow");
	settings.setValue("lastState", (int) state);
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
		QMessageBox::warning(this, tr("Warning"), tr("Please save or cancel the current dive edit before closing the file."));
		event->ignore();
		return;
	}

	if (helpView && helpView->isVisible()) {
		helpView->close();
		helpView->deleteLater();
	}

	if (unsaved_changes() && (askSaveChanges() == false)) {
		event->ignore();
		return;
	}
	event->accept();
	writeSettings();
}

DiveListView* MainWindow::dive_list()
{
	return ui.ListWidget;
}

GlobeGPS* MainWindow::globe()
{
	return ui.globe;
}

ProfileGraphicsView* MainWindow::graphics()
{
	return ui.ProfileWidget;
}

MainTab* MainWindow::information()
{
	return ui.InfoWidget;
}

void MainWindow::file_save_as(void)
{
	QString filename;
	const char *default_filename;

	if (existing_filename)
		default_filename = existing_filename;
	else
		default_filename = prefs.default_filename;
	filename = QFileDialog::getSaveFileName(this, tr("Save File as"), default_filename,
						tr("Subsurface XML files (*.ssrf *.xml *.XML)"));
	if (!filename.isNull() && !filename.isEmpty()) {

		if (ui.InfoWidget->isEditing())
			ui.InfoWidget->acceptChanges();

		save_dives(filename.toUtf8().data());
		set_filename(filename.toUtf8().data(), true);
		setTitle(MWTF_FILENAME);
		mark_divelist_changed(false);
	}
}

void MainWindow::file_save(void)
{
	const char *current_default;

	if (!existing_filename)
		return file_save_as();

	if (ui.InfoWidget->isEditing())
		ui.InfoWidget->acceptChanges();

	current_default = prefs.default_filename;
	if (strcmp(existing_filename, current_default) ==  0) {
		/* if we are using the default filename the directory
		 * that we are creating the file in may not exist */
		QDir current_def_dir = QFileInfo(current_default).absoluteDir();
		if (!current_def_dir.exists())
			current_def_dir.mkpath(current_def_dir.absolutePath());
	}
	save_dives(existing_filename);
	mark_divelist_changed(false);
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
	char *error = NULL;
	for (int i = 0; i < fileNames.size(); ++i) {
		fileNamePtr = QFile::encodeName(fileNames.at(i));
		parse_file(fileNamePtr.data(), &error);
		if (error != NULL) {
			showError(error);
			free(error);
			error = NULL;
		}
	}
	process_dives(true, false);
	refreshDisplay();
}

void MainWindow::loadFiles(const QStringList fileNames)
{
	if (fileNames.isEmpty())
		return;

	char *error = NULL;
	QByteArray fileNamePtr;

	for (int i = 0; i < fileNames.size(); ++i) {
		fileNamePtr = QFile::encodeName(fileNames.at(i));
		parse_file(fileNamePtr.data(), &error);
		set_filename(fileNamePtr.data(), true);
		setTitle(MWTF_FILENAME);

		if (error != NULL) {
			showError(error);
			free(error);
		}
	}

	process_dives(false, false);

	refreshDisplay();
	ui.actionAutoGroup->setChecked(autogroup);
}

void MainWindow::on_actionImportDiveLog_triggered()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open Dive Log File"), lastUsedDir(), tr("Dive Log Files (*.xml *.uddf *.udcf *.csv *.jlb *.dld *.sde *.db);;XML Files (*.xml);;UDDF/UDCF Files(*.uddf *.udcf);;JDiveLog Files(*.jlb);;Suunto Files(*.sde *.db);;CSV Files(*.csv);;All Files(*)"));

	if (fileNames.isEmpty())
		return;
	updateLastUsedDir(QFileInfo(fileNames[0]).dir().path());

	QStringList logFiles = fileNames.filter( QRegExp("^.*\\.(?!csv)", Qt::CaseInsensitive) ) ;
	QStringList csvFiles = fileNames.filter(".csv", Qt::CaseInsensitive);
	if (logFiles.size()) {
		importFiles(logFiles);
	}

	if (csvFiles.size()) {
		DiveLogImportDialog *diveLogImport = new DiveLogImportDialog(&csvFiles);
		diveLogImport->show();
		process_dives(true, false);
		refreshDisplay();
	}
}

void MainWindow::editCurrentDive()
{
	if (information()->isEditing() || DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING) {
		QMessageBox::warning(this, tr("Warning"), tr("First finish the current edition before trying to do another."));
		return;
	}

	struct dive *d = current_dive;
	QString defaultDC(d->dc.model);
	DivePlannerPointsModel::instance()->clear();
	if (defaultDC == "manually added dive") {
		disableDcShortcuts();
		DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::ADD);
		ui.stackedWidget->setCurrentIndex(PLANNERPROFILE); // Planner.
		ui.infoPane->setCurrentIndex(MAINTAB);
		DivePlannerPointsModel::instance()->loadFromDive(d);
		ui.InfoWidget->enableEdition(MainTab::MANUALLY_ADDED_DIVE);
	} else if (defaultDC == "planned dive") {
		disableDcShortcuts();
		DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::PLAN);
		ui.stackedWidget->setCurrentIndex(PLANNERPROFILE); // Planner.
		ui.infoPane->setCurrentIndex(PLANNERWIDGET);
		DivePlannerPointsModel::instance()->loadFromDive(d);
		ui.InfoWidget->enableEdition(MainTab::MANUALLY_ADDED_DIVE);
	}
}
