/*
 * mainwindow.cpp
 *
 * classes for the main UI window in Subsurface
 */
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QtDebug>
#include <QDateTime>
#include <QSettings>
#include <QCloseEvent>
#include <QApplication>
#include <QFontMetrics>
#include <QWebView>
#include <QTableView>
#include "divelistview.h"
#include "starwidget.h"

#include "glib.h"
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

#include "glib/gi18n.h"

static MainWindow* instance = 0;

MainWindow* mainWindow()
{
	return instance;
}

MainWindow::MainWindow() : ui(new Ui::MainWindow()), helpView(0)
{
	instance = this;
	ui->setupUi(this);
	setWindowIcon(QIcon(":subsurface-icon"));
	connect(ui->ListWidget, SIGNAL(currentDiveChanged(int)), this, SLOT(current_dive_changed(int)));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), this, SLOT(readSettings()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), ui->ListWidget, SLOT(update()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), ui->ListWidget, SLOT(reloadHeaderActions()));
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), ui->ProfileWidget, SLOT(refresh()));
	ui->mainErrorMessage->hide();
	ui->ListWidget->reload(DiveTripModel::TREE);
	initialUiSetup();
	readSettings();
	ui->ListWidget->reloadHeaderActions();
	ui->ListWidget->setFocus();
	ui->globe->reload();
}

// this gets called after we download dives from a divecomputer
void MainWindow::refreshDisplay()
{
	ui->InfoWidget->reload();
	ui->globe->reload();
	ui->ListWidget->reload(DiveTripModel::TREE);
	ui->ListWidget->setFocus();
	WSInfoModel *wsim = WSInfoModel::instance();
	wsim->updateInfo();
}

void MainWindow::current_dive_changed(int divenr)
{
	if (divenr >= 0) {
		select_dive(divenr);
		ui->globe->centerOn(get_dive(selected_dive));
		redrawProfile();
	}
	ui->InfoWidget->updateDiveInfo(divenr);
}

void MainWindow::redrawProfile()
{
	ui->ProfileWidget->refresh();
}

void MainWindow::on_actionNew_triggered()
{
	on_actionClose_triggered();
}

void MainWindow::on_actionOpen_triggered()
{
	QSettings settings;
	QString lastDir = QDir::homePath();

	settings.beginGroup("FileDialog");
	if (settings.contains("LastDir")) {
		if(QDir::setCurrent(settings.value("LastDir").toString())) {
			lastDir = settings.value("LastDir").toString();
		}
	}
	settings.endGroup();

	QString filename = QFileDialog::getOpenFileName(this, tr("Open File"), lastDir, filter());
	if (filename.isEmpty())
		return;

	// Keep last open dir
	QFileInfo fileInfo(filename);
	settings.beginGroup("FileDialog");
	settings.setValue("LastDir",fileInfo.dir().path());
	settings.endGroup();

	// Needed to convert to char*
	QByteArray fileNamePtr = filename.toLocal8Bit();

	on_actionClose_triggered();
	loadFiles( QStringList() << filename );
}

void MainWindow::on_actionSave_triggered()
{
	file_save();
}

void MainWindow::on_actionSaveAs_triggered()
{
	file_save_as();
}
void MainWindow::on_actionClose_triggered()
{
	if (unsaved_changes() && (askSaveChanges() == FALSE))
		return;

	/* free the dives and trips */
	while (dive_table.nr)
		delete_single_dive(0);

	/* clear the selection and the statistics */
	selected_dive = -1;

	//WARNING: Port this to Qt.
	//process_selected_dives();

	ui->InfoWidget->clearStats();
	ui->InfoWidget->clearInfo();
	ui->InfoWidget->clearEquipment();
	ui->InfoWidget->updateDiveInfo(-1);
	ui->ProfileWidget->clear();
	ui->ListWidget->reload(DiveTripModel::TREE);
	ui->globe->reload();
	setTitle(MWTF_DEFAULT);
	mark_divelist_changed(FALSE);

	clear_events();
}

void MainWindow::on_actionImport_triggered()
{
	QSettings settings;
	QString lastDir = QDir::homePath();

	settings.beginGroup("FileDialog");
	if (settings.contains("LastDir"))
		if (QDir::setCurrent(settings.value("LastDir").toString()))
			lastDir = settings.value("LastDir").toString();
	settings.endGroup();

	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Import Files"), lastDir, filter());
	if (!fileNames.size())
		return; // no selection

	// Keep last open dir
	QFileInfo fileInfo(fileNames.at(0));
	settings.beginGroup("FileDialog");
	settings.setValue("LastDir", fileInfo.dir().path());
	settings.endGroup();

	importFiles(fileNames);
}

void MainWindow::on_actionExportUDDF_triggered()
{
	qDebug("actionExportUDDF");
}

void MainWindow::on_actionPrint_triggered()
{
	PrintDialog::instance()->runDialog();
}

void MainWindow::disableDcShortcuts()
{
	ui->actionPreviousDC->setShortcut(QKeySequence());
	ui->actionNextDC->setShortcut(QKeySequence());
}

void MainWindow::enableDcShortcuts()
{
	ui->actionPreviousDC->setShortcut(Qt::Key_Left);
	ui->actionNextDC->setShortcut(Qt::Key_Right);
}

void MainWindow::on_actionDivePlanner_triggered()
{
	disableDcShortcuts();
	DivePlannerPointsModel::instance()->setPlanMode(true);
	ui->stackedWidget->setCurrentIndex(1);
	ui->infoPane->setCurrentIndex(1);
}

void MainWindow::showProfile()
{
	enableDcShortcuts();
	ui->stackedWidget->setCurrentIndex(0);
	ui->infoPane->setCurrentIndex(0);
}


void MainWindow::on_actionPreferences_triggered()
{
	PreferencesDialog::instance()->show();
}

void MainWindow::on_actionQuit_triggered()
{
	if (unsaved_changes() && (askSaveChanges() == FALSE))
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
	SubsurfaceWebServices::instance()->runDialog();
}

void MainWindow::on_actionEditDeviceNames_triggered()
{
	DiveComputerManagementDialog::instance()->init();
	DiveComputerManagementDialog::instance()->update();
	DiveComputerManagementDialog::instance()->show();
}

void MainWindow::on_actionAddDive_triggered()
{
	// clear the selection
	for (int i = 0; i < dive_table.nr; i++) {
		struct dive *d = get_dive(i);
		if (d && d->selected)
			deselect_dive(i);
	}
	disableDcShortcuts();
	DivePlannerPointsModel::instance()->setPlanMode(false);
	// now cheat - create one dive that we use to store the info tab data in
	struct dive *dive = alloc_dive();
	dive->when = QDateTime::currentMSecsSinceEpoch() / 1000L;
	dive->dc.model = _("manually added dive");
	record_dive(dive);
	select_dive(get_divenr(dive));
	ui->InfoWidget->updateDiveInfo(selected_dive);
	ui->stackedWidget->setCurrentIndex(1);
	ui->infoPane->setCurrentIndex(0);
	DivePlannerPointsModel::instance()->createSimpleDive();
	refreshDisplay();
	ui->InfoWidget->addDiveStarted();
}

void MainWindow::on_actionRenumber_triggered()
{
	RenumberDialog::instance()->show();
}

void MainWindow::on_actionAutoGroup_triggered()
{
	qDebug("actionAutoGroup");
}

void MainWindow::on_actionToggleZoom_triggered()
{
	qDebug("actionToggleZoom");
}

void MainWindow::on_actionYearlyStatistics_triggered()
{
	QTreeView *view = new QTreeView();
	QAbstractItemModel *model = new YearlyStatisticsModel();
	view->setModel(model);
	view->setWindowModality(Qt::NonModal);
	view->setMinimumWidth(600);
	view->setAttribute(Qt::WA_QuitOnClose, false);
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
	ui->listGlobeSplitter->setSizes( BEHAVIOR << EXPANDED << COLLAPSED);
	ui->mainSplitter->setSizes( BEHAVIOR << COLLAPSED << EXPANDED);
}

void MainWindow::on_actionViewProfile_triggered()
{
	ui->infoProfileSplitter->setSizes(BEHAVIOR << COLLAPSED << EXPANDED);
	ui->mainSplitter->setSizes( BEHAVIOR << EXPANDED << COLLAPSED);
	redrawProfile();
}

void MainWindow::on_actionViewInfo_triggered()
{
	ui->infoProfileSplitter->setSizes(BEHAVIOR << EXPANDED << COLLAPSED);
	ui->mainSplitter->setSizes( BEHAVIOR << EXPANDED << COLLAPSED);
}

void MainWindow::on_actionViewGlobe_triggered()
{
	ui->mainSplitter->setSizes(BEHAVIOR << COLLAPSED << EXPANDED);
	ui->listGlobeSplitter->setSizes(BEHAVIOR << COLLAPSED << EXPANDED);
}
#undef BEHAVIOR

void MainWindow::on_actionViewAll_triggered()
{
	// big number squash the info profile to it's minimum.
	ui->infoProfileSplitter->setSizes(QList<int>() << 1 << 20000);

	// big number squash the globe view.
	ui->listGlobeSplitter->setSizes(QList<int>() << 2000 << 1 );

	// half and half?
	ui->mainSplitter->setSizes( QList<int>() << 1 << 1);
	redrawProfile();
}

void MainWindow::on_actionPreviousDC_triggered()
{
	dc_number--;
	redrawProfile();
}

void MainWindow::on_actionNextDC_triggered()
{
	dc_number++;
	redrawProfile();
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
	if(!helpView){
		helpView = new QWebView();
	}
	QString searchPath = getSubsurfaceDataPath("Documentation");
	if (searchPath != "") {
		QUrl url(searchPath.append("/user-manual.html"));
		helpView->setUrl(url);
	} else {
		helpView->setHtml(tr("Cannot find the Subsurface manual"));
	}
	helpView->show();
}

QString MainWindow::filter()
{
	QString f;
	f += "ALL ( *.xml *.XML *.uddf *.udcf *.UDFC *.jlb *.JLB ";
#ifdef LIBZIP
	f += "*.sde *.SDE *.dld *.DLD ";
#endif
#ifdef SQLITE3
	f += "*.db";
#endif
	f += ");;";

	f += "XML (*.xml *.XML);;";
	f += "UDDF (*.uddf);;";
	f += "UDCF (*.udcf *.UDCF);;";
	f += "JLB  (*.jlb *.JLB);;";

#ifdef LIBZIP
	f += "SDE (*.sde *.SDE);;";
	f += "DLD (*.dld *.DLD);;";
#endif
#ifdef SQLITE3
	f += "DB (*.db)";
#endif

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
		prefs.field = v.toInt() ? TRUE : FALSE;		\
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


void MainWindow::initialUiSetup()
{
	QSettings settings;
	int i;

	settings.beginGroup("MainWindow");
	QSize sz = settings.value("size").value<QSize>();
	resize(sz);
	ui->mainSplitter->restoreState(settings.value("mainSplitter").toByteArray());
	ui->infoProfileSplitter->restoreState(settings.value("infoProfileSplitter").toByteArray());
	ui->listGlobeSplitter->restoreState(settings.value("listGlobeSplitter").toByteArray());
	settings.endGroup();

	settings.beginGroup("ListWidget");
	/* if no width are set, use the calculated width for each column;
	 * for that to work we need to temporarily expand all rows */
	ui->ListWidget->expandAll();
	for (i = DiveTripModel::NR; i < DiveTripModel::COLUMNS; i++) {
		QVariant width = settings.value(QString("colwidth%1").arg(i));
		if (width.isValid())
			ui->ListWidget->setColumnWidth(i, width.toInt());
		else
			ui->ListWidget->resizeColumnToContents(i);
	}
	ui->ListWidget->collapseAll();
	ui->ListWidget->expand(ui->ListWidget->model()->index(0,0));
	ui->ListWidget->scrollTo(ui->ListWidget->model()->index(0,0), QAbstractItemView::PositionAtCenter);
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
	s.endGroup();
	s.beginGroup("DisplayListColumns");
	GET_BOOL("CYLINDER", visible_cols.cylinder);
	GET_BOOL("TEMPERATURE", visible_cols.temperature);
	GET_BOOL("TOTALWEIGHT", visible_cols.totalweight);
	GET_BOOL("SUIT", visible_cols.suit);
	GET_BOOL("NITROX", visible_cols.nitrox);
	GET_BOOL("OTU", visible_cols.otu);
	GET_BOOL("MAXCNS", visible_cols.maxcns);
	GET_BOOL("SAC", visible_cols.sac);
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
	GET_BOOL("calcalltissues", calc_all_tissues);
	GET_INT("gflow", gflow);
	GET_INT("gfhigh", gfhigh);
	set_gf(prefs.gflow, prefs.gfhigh);
	GET_BOOL("show_time", show_time);
	s.endGroup();

	s.beginGroup("Display");
	v = s.value(QString("divelist_font"));
	if (v.isValid())
		prefs.divelist_font = strdup(v.toString().toUtf8().data());
}

#define SAVE_VALUE(name, field)				\
	if (prefs.field != default_prefs.field)		\
		settings.setValue(name, prefs.field);	\
	else						\
		settings.remove(name)

void MainWindow::writeSettings()
{
	int i;
	QSettings settings;

	settings.beginGroup("MainWindow");
	settings.setValue("size",size());
	settings.setValue("mainSplitter", ui->mainSplitter->saveState());
	settings.setValue("infoProfileSplitter", ui->infoProfileSplitter->saveState());
	settings.setValue("listGlobeSplitter", ui->listGlobeSplitter->saveState());
	settings.endGroup();

	settings.beginGroup("ListWidget");
	for (i = DiveTripModel::NR; i < DiveTripModel::COLUMNS; i++)
		if (!ui->ListWidget->isColumnHidden(i))
			settings.setValue(QString("colwidth%1").arg(i), ui->ListWidget->columnWidth(i));
	settings.endGroup();
	settings.beginGroup("Units");
	SAVE_VALUE("length", units.length);
	SAVE_VALUE("pressure", units.pressure);
	SAVE_VALUE("volume", units.volume);
	SAVE_VALUE("temperature", units.temperature);
	SAVE_VALUE("weight", units.weight);
	settings.endGroup();
	settings.beginGroup("DisplayListColumns");
	SAVE_VALUE("TEMPERATURE", visible_cols.temperature);
	SAVE_VALUE("TOTALWEIGHT", visible_cols.totalweight);
	SAVE_VALUE("SUIT", visible_cols.suit);
	SAVE_VALUE("CYLINDER", visible_cols.cylinder);
	SAVE_VALUE("NITROX", visible_cols.nitrox);
	SAVE_VALUE("SAC", visible_cols.sac);
	SAVE_VALUE("OTU", visible_cols.otu);
	SAVE_VALUE("MAXCNS", visible_cols.maxcns);
	settings.endGroup();
	settings.beginGroup("TecDetails");
	SAVE_VALUE("po2graph", pp_graphs.po2);
	SAVE_VALUE("pn2graph", pp_graphs.pn2);
	SAVE_VALUE("phegraph", pp_graphs.phe);
	SAVE_VALUE("po2threshold", pp_graphs.po2_threshold);
	SAVE_VALUE("pn2threshold", pp_graphs.pn2_threshold);
	SAVE_VALUE("phethreshold", pp_graphs.phe_threshold);
	SAVE_VALUE("mod", mod);
	SAVE_VALUE("modppO2", mod_ppO2);
	SAVE_VALUE("ead", ead);
	SAVE_VALUE("redceiling", profile_red_ceiling);
	SAVE_VALUE("calcceiling", profile_calc_ceiling);
	SAVE_VALUE("calcceiling3m", calc_ceiling_3m_incr);
	SAVE_VALUE("calcalltissues", calc_all_tissues);
	SAVE_VALUE("dcceiling", profile_dc_ceiling);
	SAVE_VALUE("gflow", gflow);
	SAVE_VALUE("gfhigh", gfhigh);
	settings.endGroup();
	settings.beginGroup("GeneralSettings");
	SAVE_VALUE("default_filename", default_filename);
	settings.endGroup();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (helpView && helpView->isVisible()){
		helpView->close();
		helpView->deleteLater();
	}

	if (unsaved_changes() && (askSaveChanges() == FALSE)) {
		event->ignore();
		return;
	}
	event->accept();
	writeSettings();
}

DiveListView* MainWindow::dive_list()
{
	return ui->ListWidget;
}

GlobeGPS* MainWindow::globe()
{
	return ui->globe;
}

ProfileGraphicsView* MainWindow::graphics()
{
	return ui->ProfileWidget;
}

MainTab* MainWindow::information()
{
	return ui->InfoWidget;
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
		save_dives(filename.toUtf8().data());
		set_filename(filename.toUtf8().data(), TRUE);
		setTitle(MWTF_FILENAME);
		mark_divelist_changed(FALSE);
	}
}

void MainWindow::file_save(void)
{
	const char *current_default;

	if (!existing_filename)
		return file_save_as();

	current_default = prefs.default_filename;
	if (strcmp(existing_filename, current_default) ==  0) {
		/* if we are using the default filename the directory
		 * that we are creating the file in may not exist */
		QDir current_def_dir = QFileInfo(current_default).absoluteDir();
		if (!current_def_dir.exists())
			current_def_dir.mkpath(current_def_dir.absolutePath());
	}
	save_dives(existing_filename);
	mark_divelist_changed(FALSE);
}

void MainWindow::showError(QString message)
{
	if (message.isEmpty())
		return;
	ui->mainErrorMessage->setText(message);
	ui->mainErrorMessage->setCloseButtonVisible(true);
	ui->mainErrorMessage->setMessageType(KMessageWidget::Error);
	ui->mainErrorMessage->animatedShow();
}

void MainWindow::setTitle(enum MainWindowTitleFormat format)
{
	switch (format) {
	case MWTF_DEFAULT:
		setWindowTitle("Subsurface");
		break;
	case MWTF_FILENAME:
		QFile f(existing_filename);
		QFileInfo fileInfo(f);
		QString fileName(fileInfo.fileName());
		setWindowTitle("Subsurface: " + fileName);
		break;
	}
}

void MainWindow::importFiles(const QStringList fileNames)
{
	QByteArray fileNamePtr;
	char *error = NULL;
	for (int i = 0; i < fileNames.size(); ++i) {
		fileNamePtr = fileNames.at(i).toLocal8Bit();
		parse_file(fileNamePtr.data(), &error);
		if (error != NULL) {
			showError(error);
			free(error);
			error = NULL;
		}
	}
	process_dives(TRUE, FALSE);

	ui->InfoWidget->reload();
	ui->globe->reload();
	ui->ListWidget->reload(DiveTripModel::TREE);
	ui->ListWidget->setFocus();
	WSInfoModel *wsim = WSInfoModel::instance();
	wsim->updateInfo();
}

void MainWindow::loadFiles(const QStringList fileNames)
{
	char *error = NULL;
	QByteArray fileNamePtr;

	for (int i = 0; i < fileNames.size(); ++i) {
		fileNamePtr = fileNames.at(i).toLocal8Bit();
		parse_file(fileNamePtr.data(), &error);
		set_filename(fileNamePtr.data(), TRUE);
		setTitle(MWTF_FILENAME);

		if (error != NULL) {
			showError(error);
			free(error);
		}
	}

	process_dives(FALSE, FALSE);

	ui->InfoWidget->reload();
	ui->globe->reload();
	ui->ListWidget->reload(DiveTripModel::TREE);
	ui->ListWidget->setFocus();
	WSInfoModel *wsim = WSInfoModel::instance();
	wsim->updateInfo();
}
