#include "preferences.h"
#include "mainwindow.h"
#include "models.h"
#include "divelocationmodel.h"
#include "prefs-macros.h"
#include "qthelper.h"
#include "subsurfacestartup.h"

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QNetworkProxy>
#include <QNetworkCookieJar>

#include "subsurfacewebservices.h"

#if !defined(Q_OS_ANDROID) && defined(FBSUPPORT)
#include "socialnetworks.h"
#include <QWebView>
#endif

PreferencesDialog *PreferencesDialog::instance()
{
	static PreferencesDialog *dialog = new PreferencesDialog(MainWindow::instance());
	return dialog;
}

PreferencesDialog::PreferencesDialog(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_QuitOnClose, false);

#if defined(Q_OS_ANDROID) || !defined(FBSUPPORT)
	for (int i = 0; i < ui.listWidget->count(); i++) {
		if (ui.listWidget->item(i)->text() == "Facebook") {
			delete ui.listWidget->item(i);
			QWidget *fbpage = ui.stackedWidget->widget(i);
			ui.stackedWidget->removeWidget(fbpage);
		}
	}
#endif

	ui.proxyType->clear();
	ui.proxyType->addItem(tr("No proxy"), QNetworkProxy::NoProxy);
	ui.proxyType->addItem(tr("System proxy"), QNetworkProxy::DefaultProxy);
	ui.proxyType->addItem(tr("HTTP proxy"), QNetworkProxy::HttpProxy);
	ui.proxyType->addItem(tr("SOCKS proxy"), QNetworkProxy::Socks5Proxy);
	ui.proxyType->setCurrentIndex(-1);

	ui.first_item->setModel(GeoReferencingOptionsModel::instance());
	ui.second_item->setModel(GeoReferencingOptionsModel::instance());
	ui.third_item->setModel(GeoReferencingOptionsModel::instance());
	// Facebook stuff:
#if !defined(Q_OS_ANDROID) && defined(FBSUPPORT)
	FacebookManager *fb = FacebookManager::instance();
	facebookWebView = new QWebView(this);
	ui.fbWebviewContainer->layout()->addWidget(facebookWebView);
	if (fb->loggedIn()) {
		facebookLoggedIn();
	} else {
		facebookDisconnect();
	}
	connect(facebookWebView, &QWebView::urlChanged, fb, &FacebookManager::tryLogin);
	connect(fb, &FacebookManager::justLoggedIn, this, &PreferencesDialog::facebookLoggedIn);
	connect(ui.fbDisconnect, &QPushButton::clicked, fb, &FacebookManager::logout);
	connect(fb, &FacebookManager::justLoggedOut, this, &PreferencesDialog::facebookDisconnect);
#endif
	connect(ui.proxyType, SIGNAL(currentIndexChanged(int)), this, SLOT(proxyType_changed(int)));
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	connect(ui.gflow, SIGNAL(valueChanged(int)), this, SLOT(gflowChanged(int)));
	connect(ui.gfhigh, SIGNAL(valueChanged(int)), this, SLOT(gfhighChanged(int)));
	//	connect(ui.defaultSetpoint, SIGNAL(valueChanged(double)), this, SLOT(defaultSetpointChanged(double)));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
	loadSettings();
	setUiFromPrefs();
	rememberPrefs();
}

void PreferencesDialog::facebookLoggedIn()
{
#if !defined(Q_OS_ANDROID) && defined(FBSUPPORT)
	ui.fbDisconnect->show();
	ui.fbWebviewContainer->hide();
	ui.fbWebviewContainer->setEnabled(false);
	ui.FBLabel->setText(tr("To disconnect Subsurface from your Facebook account, use the button below"));
#endif
}

void PreferencesDialog::facebookDisconnect()
{
#if !defined(Q_OS_ANDROID) && defined(FBSUPPORT)
	// remove the connect/disconnect button
	// and instead add the login view
	ui.fbDisconnect->hide();
	ui.fbWebviewContainer->show();
	ui.fbWebviewContainer->setEnabled(true);
	ui.FBLabel->setText(tr("To connect to Facebook, please log in. This enables Subsurface to publish dives to your timeline"));
	if (facebookWebView) {
		facebookWebView->page()->networkAccessManager()->setCookieJar(new QNetworkCookieJar());
		facebookWebView->setUrl(FacebookManager::instance()->connectUrl());
	}
#endif
}

void PreferencesDialog::cloudPinNeeded()
{
	ui.cloud_storage_pin->setEnabled(prefs.cloud_verification_status == CS_NEED_TO_VERIFY);
	ui.cloud_storage_pin->setVisible(prefs.cloud_verification_status == CS_NEED_TO_VERIFY);
	ui.cloud_storage_pin_label->setEnabled(prefs.cloud_verification_status == CS_NEED_TO_VERIFY);
	ui.cloud_storage_pin_label->setVisible(prefs.cloud_verification_status == CS_NEED_TO_VERIFY);
	ui.cloud_storage_new_passwd->setEnabled(prefs.cloud_verification_status == CS_VERIFIED);
	ui.cloud_storage_new_passwd->setVisible(prefs.cloud_verification_status == CS_VERIFIED);
	ui.cloud_storage_new_passwd_label->setEnabled(prefs.cloud_verification_status == CS_VERIFIED);
	ui.cloud_storage_new_passwd_label->setVisible(prefs.cloud_verification_status == CS_VERIFIED);
	if (prefs.cloud_verification_status == CS_VERIFIED) {
		ui.cloudStorageGroupBox->setTitle(tr("Subsurface cloud storage (credentials verified)"));
		ui.cloudDefaultFile->setEnabled(true);
	} else {
		ui.cloudStorageGroupBox->setTitle(tr("Subsurface cloud storage"));
		if (ui.cloudDefaultFile->isChecked())
			ui.noDefaultFile->setChecked(true);
		ui.cloudDefaultFile->setEnabled(false);
	}
	MainWindow::instance()->enableDisableCloudActions();
}

#define DANGER_GF (gf > 100) ? "* { color: red; }" : ""
void PreferencesDialog::gflowChanged(int gf)
{
	ui.gflow->setStyleSheet(DANGER_GF);
}
void PreferencesDialog::gfhighChanged(int gf)
{
	ui.gfhigh->setStyleSheet(DANGER_GF);
}
#undef DANGER_GF

void PreferencesDialog::showEvent(QShowEvent *event)
{
	setUiFromPrefs();
	rememberPrefs();
	QDialog::showEvent(event);
}

void PreferencesDialog::setUiFromPrefs()
{
	// graphs
	ui.pheThreshold->setValue(prefs.pp_graphs.phe_threshold);
	ui.po2Threshold->setValue(prefs.pp_graphs.po2_threshold);
	ui.pn2Threshold->setValue(prefs.pp_graphs.pn2_threshold);
	ui.maxpo2->setValue(prefs.modpO2);
	ui.red_ceiling->setChecked(prefs.redceiling);
	ui.units_group->setEnabled(ui.personalize->isChecked());

	ui.gflow->setValue(prefs.gflow);
	ui.gfhigh->setValue(prefs.gfhigh);
	ui.gf_low_at_maxdepth->setChecked(prefs.gf_low_at_maxdepth);
	ui.show_ccr_setpoint->setChecked(prefs.show_ccr_setpoint);
	ui.show_ccr_sensors->setChecked(prefs.show_ccr_sensors);
	ui.defaultSetpoint->setValue((double)prefs.defaultsetpoint / 1000.0);
	ui.psro2rate->setValue(prefs.o2consumption / 1000.0);
	ui.pscrfactor->setValue(rint(1000.0 / prefs.pscr_ratio));

	// units
	if (prefs.unit_system == METRIC)
		ui.metric->setChecked(true);
	else if (prefs.unit_system == IMPERIAL)
		ui.imperial->setChecked(true);
	else
		ui.personalize->setChecked(true);
	ui.gpsTraditional->setChecked(prefs.coordinates_traditional);
	ui.gpsDecimal->setChecked(!prefs.coordinates_traditional);

	ui.celsius->setChecked(prefs.units.temperature == units::CELSIUS);
	ui.fahrenheit->setChecked(prefs.units.temperature == units::FAHRENHEIT);
	ui.meter->setChecked(prefs.units.length == units::METERS);
	ui.feet->setChecked(prefs.units.length == units::FEET);
	ui.bar->setChecked(prefs.units.pressure == units::BAR);
	ui.psi->setChecked(prefs.units.pressure == units::PSI);
	ui.liter->setChecked(prefs.units.volume == units::LITER);
	ui.cuft->setChecked(prefs.units.volume == units::CUFT);
	ui.kg->setChecked(prefs.units.weight == units::KG);
	ui.lbs->setChecked(prefs.units.weight == units::LBS);

	ui.font->setCurrentFont(QString(prefs.divelist_font));
	ui.fontsize->setValue(prefs.font_size);
	ui.defaultfilename->setText(prefs.default_filename);
	ui.noDefaultFile->setChecked(prefs.default_file_behavior == NO_DEFAULT_FILE);
	ui.cloudDefaultFile->setChecked(prefs.default_file_behavior == CLOUD_DEFAULT_FILE);
	ui.localDefaultFile->setChecked(prefs.default_file_behavior == LOCAL_DEFAULT_FILE);
	ui.default_cylinder->clear();
	for (int i = 0; tank_info[i].name != NULL; i++) {
		ui.default_cylinder->addItem(tank_info[i].name);
		if (prefs.default_cylinder && strcmp(tank_info[i].name, prefs.default_cylinder) == 0)
			ui.default_cylinder->setCurrentIndex(i);
	}
	ui.displayinvalid->setChecked(prefs.display_invalid_dives);
	ui.display_unused_tanks->setChecked(prefs.display_unused_tanks);
	ui.show_average_depth->setChecked(prefs.show_average_depth);
	ui.vertical_speed_minutes->setChecked(prefs.units.vertical_speed_time == units::MINUTES);
	ui.vertical_speed_seconds->setChecked(prefs.units.vertical_speed_time == units::SECONDS);
	ui.velocitySlider->setValue(prefs.animation_speed);

	QSortFilterProxyModel *filterModel = new QSortFilterProxyModel();
	filterModel->setSourceModel(LanguageModel::instance());
	filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	ui.languageView->setModel(filterModel);
	filterModel->sort(0);
	connect(ui.languageFilter, SIGNAL(textChanged(QString)), filterModel, SLOT(setFilterFixedString(QString)));

	QSettings s;

	ui.save_uid_local->setChecked(s.value("save_uid_local").toBool());
	ui.default_uid->setText(s.value("subsurface_webservice_uid").toString().toUpper());

	s.beginGroup("Language");
	ui.languageSystemDefault->setChecked(s.value("UseSystemLanguage", true).toBool());
	QAbstractItemModel *m = ui.languageView->model();
	QModelIndexList languages = m->match(m->index(0, 0), Qt::UserRole, s.value("UiLanguage").toString());
	if (languages.count())
		ui.languageView->setCurrentIndex(languages.first());

	s.endGroup();

	ui.proxyHost->setText(prefs.proxy_host);
	ui.proxyPort->setValue(prefs.proxy_port);
	ui.proxyAuthRequired->setChecked(prefs.proxy_auth);
	ui.proxyUsername->setText(prefs.proxy_user);
	ui.proxyPassword->setText(prefs.proxy_pass);
	ui.proxyType->setCurrentIndex(ui.proxyType->findData(prefs.proxy_type));
	ui.btnUseDefaultFile->setChecked(prefs.use_default_file);

	ui.cloud_storage_email->setText(prefs.cloud_storage_email);
	ui.cloud_storage_password->setText(prefs.cloud_storage_password);
	ui.save_password_local->setChecked(prefs.save_password_local);
	cloudPinNeeded();
	ui.cloud_background_sync->setChecked(prefs.cloud_background_sync);
	ui.default_uid->setText(prefs.userid);

	// GeoManagement
#ifdef DISABLED
	ui.enable_geocoding->setChecked( prefs.geocoding.enable_geocoding );
	ui.parse_without_gps->setChecked(prefs.geocoding.parse_dive_without_gps);
	ui.tag_existing_dives->setChecked(prefs.geocoding.tag_existing_dives);
#endif
	ui.first_item->setCurrentIndex(prefs.geocoding.category[0]);
	ui.second_item->setCurrentIndex(prefs.geocoding.category[1]);
	ui.third_item->setCurrentIndex(prefs.geocoding.category[2]);
}

void PreferencesDialog::restorePrefs()
{
	prefs = oldPrefs;
	setUiFromPrefs();
}

void PreferencesDialog::rememberPrefs()
{
	oldPrefs = prefs;
}

void PreferencesDialog::syncSettings()
{
	QSettings s;

	s.setValue("subsurface_webservice_uid", ui.default_uid->text().toUpper());
	set_save_userid_local(ui.save_uid_local->checkState());

	// Graph
	s.beginGroup("TecDetails");
	SAVE_OR_REMOVE("phethreshold", default_prefs.pp_graphs.phe_threshold, ui.pheThreshold->value());
	SAVE_OR_REMOVE("po2threshold", default_prefs.pp_graphs.po2_threshold, ui.po2Threshold->value());
	SAVE_OR_REMOVE("pn2threshold", default_prefs.pp_graphs.pn2_threshold, ui.pn2Threshold->value());
	SAVE_OR_REMOVE("modpO2", default_prefs.modpO2, ui.maxpo2->value());
	SAVE_OR_REMOVE("redceiling", default_prefs.redceiling, ui.red_ceiling->isChecked());
	SAVE_OR_REMOVE("gflow", default_prefs.gflow, ui.gflow->value());
	SAVE_OR_REMOVE("gfhigh", default_prefs.gfhigh, ui.gfhigh->value());
	SAVE_OR_REMOVE("gf_low_at_maxdepth", default_prefs.gf_low_at_maxdepth, ui.gf_low_at_maxdepth->isChecked());
	SAVE_OR_REMOVE("show_ccr_setpoint", default_prefs.show_ccr_setpoint, ui.show_ccr_setpoint->isChecked());
	SAVE_OR_REMOVE("show_ccr_sensors", default_prefs.show_ccr_sensors, ui.show_ccr_sensors->isChecked());
	SAVE_OR_REMOVE("display_unused_tanks", default_prefs.display_unused_tanks, ui.display_unused_tanks->isChecked());
	SAVE_OR_REMOVE("show_average_depth", default_prefs.show_average_depth, ui.show_average_depth->isChecked());
	s.endGroup();

	// Units
	s.beginGroup("Units");
	QString unitSystem[] = {"metric", "imperial", "personal"};
	short unitValue = ui.metric->isChecked() ? METRIC : (ui.imperial->isChecked() ? IMPERIAL : PERSONALIZE);
	SAVE_OR_REMOVE_SPECIAL("unit_system", default_prefs.unit_system, unitValue, unitSystem[unitValue]);
	s.setValue("temperature", ui.fahrenheit->isChecked() ? units::FAHRENHEIT : units::CELSIUS);
	s.setValue("length", ui.feet->isChecked() ? units::FEET : units::METERS);
	s.setValue("pressure", ui.psi->isChecked() ? units::PSI : units::BAR);
	s.setValue("volume", ui.cuft->isChecked() ? units::CUFT : units::LITER);
	s.setValue("weight", ui.lbs->isChecked() ? units::LBS : units::KG);
	s.setValue("vertical_speed_time", ui.vertical_speed_minutes->isChecked() ? units::MINUTES : units::SECONDS);
	s.setValue("coordinates", ui.gpsTraditional->isChecked());
	s.endGroup();

	// Defaults
	s.beginGroup("GeneralSettings");
	s.setValue("default_filename", ui.defaultfilename->text());
	s.setValue("default_cylinder", ui.default_cylinder->currentText());
	s.setValue("use_default_file", ui.btnUseDefaultFile->isChecked());
	if (ui.noDefaultFile->isChecked())
		s.setValue("default_file_behavior", NO_DEFAULT_FILE);
	else if (ui.localDefaultFile->isChecked())
		s.setValue("default_file_behavior", LOCAL_DEFAULT_FILE);
	else if (ui.cloudDefaultFile->isChecked())
		s.setValue("default_file_behavior", CLOUD_DEFAULT_FILE);
	s.setValue("defaultsetpoint", rint(ui.defaultSetpoint->value() * 1000.0));
	s.setValue("o2consumption", rint(ui.psro2rate->value() *1000.0));
	s.setValue("pscr_ratio", rint(1000.0 / ui.pscrfactor->value()));
	s.endGroup();

	s.beginGroup("Display");
	SAVE_OR_REMOVE_SPECIAL("divelist_font", system_divelist_default_font, ui.font->currentFont().toString(), ui.font->currentFont());
	SAVE_OR_REMOVE("font_size", system_divelist_default_font_size, ui.fontsize->value());
	s.setValue("displayinvalid", ui.displayinvalid->isChecked());
	s.endGroup();
	s.sync();

	// Locale
	QLocale loc;
	s.beginGroup("Language");
	bool useSystemLang = s.value("UseSystemLanguage", true).toBool();
	if (useSystemLang != ui.languageSystemDefault->isChecked() ||
	    (!useSystemLang && s.value("UiLanguage").toString() != ui.languageView->currentIndex().data(Qt::UserRole))) {
		QMessageBox::warning(MainWindow::instance(), tr("Restart required"),
				     tr("To correctly load a new language you must restart Subsurface."));
	}
	s.setValue("UseSystemLanguage", ui.languageSystemDefault->isChecked());
	s.setValue("UiLanguage", ui.languageView->currentIndex().data(Qt::UserRole));
	s.endGroup();

	// Animation
	s.beginGroup("Animations");
	s.setValue("animation_speed", ui.velocitySlider->value());
	s.endGroup();

	s.beginGroup("Network");
	s.setValue("proxy_type", ui.proxyType->itemData(ui.proxyType->currentIndex()).toInt());
	s.setValue("proxy_host", ui.proxyHost->text());
	s.setValue("proxy_port", ui.proxyPort->value());
	SB("proxy_auth", ui.proxyAuthRequired);
	s.setValue("proxy_user", ui.proxyUsername->text());
	s.setValue("proxy_pass", ui.proxyPassword->text());
	s.endGroup();

	s.beginGroup("CloudStorage");
	QString email = ui.cloud_storage_email->text();
	QString password = ui.cloud_storage_password->text();
	QString newpassword = ui.cloud_storage_new_passwd->text();
	if (prefs.cloud_verification_status == CS_VERIFIED && !newpassword.isEmpty()) {
		// deal with password change
		if (!email.isEmpty() && !password.isEmpty()) {
			// connect to backend server to check / create credentials
			QRegularExpression reg("^[a-zA-Z0-9@.+_-]+$");
			if (!reg.match(email).hasMatch() || (!password.isEmpty() && !reg.match(password).hasMatch())) {
				report_error(qPrintable(tr("Cloud storage email and password can only consist of letters, numbers, and '.', '-', '_', and '+'.")));
			} else {
				CloudStorageAuthenticate *cloudAuth = new CloudStorageAuthenticate(this);
				connect(cloudAuth, SIGNAL(finishedAuthenticate()), this, SLOT(cloudPinNeeded()));
				connect(cloudAuth, SIGNAL(passwordChangeSuccessful()), this, SLOT(passwordUpdateSuccessfull()));
				QNetworkReply *reply = cloudAuth->backend(email, password, "", newpassword);
				ui.cloud_storage_new_passwd->setText("");
				free(prefs.cloud_storage_newpassword);
				prefs.cloud_storage_newpassword = strdup(qPrintable(newpassword));
			}
		}
	} else if (prefs.cloud_verification_status == CS_UNKNOWN ||
		   prefs.cloud_verification_status == CS_INCORRECT_USER_PASSWD ||
		   email != prefs.cloud_storage_email ||
		   password != prefs.cloud_storage_password) {

		// different credentials - reset verification status
		prefs.cloud_verification_status = CS_UNKNOWN;
		if (!email.isEmpty() && !password.isEmpty()) {
			// connect to backend server to check / create credentials
			QRegularExpression reg("^[a-zA-Z0-9@.+_-]+$");
			if (!reg.match(email).hasMatch() || (!password.isEmpty() && !reg.match(password).hasMatch())) {
				report_error(qPrintable(tr("Cloud storage email and password can only consist of letters, numbers, and '.', '-', '_', and '+'.")));
			} else {
				CloudStorageAuthenticate *cloudAuth = new CloudStorageAuthenticate(this);
				connect(cloudAuth, SIGNAL(finishedAuthenticate()), this, SLOT(cloudPinNeeded()));
				QNetworkReply *reply = cloudAuth->backend(email, password);
			}
		}
	} else if (prefs.cloud_verification_status == CS_NEED_TO_VERIFY) {
		QString pin = ui.cloud_storage_pin->text();
		if (!pin.isEmpty()) {
			// connect to backend server to check / create credentials
			QRegularExpression reg("^[a-zA-Z0-9@.+_-]+$");
			if (!reg.match(email).hasMatch() || !reg.match(password).hasMatch()) {
				report_error(qPrintable(tr("Cloud storage email and password can only consist of letters, numbers, and '.', '-', '_', and '+'.")));
			}
			CloudStorageAuthenticate *cloudAuth = new CloudStorageAuthenticate(this);
			connect(cloudAuth, SIGNAL(finishedAuthenticate()), this, SLOT(cloudPinNeeded()));
			QNetworkReply *reply = cloudAuth->backend(email, password, pin);
		}
	}
	SAVE_OR_REMOVE("email", default_prefs.cloud_storage_email, email);
	SAVE_OR_REMOVE("save_password_local", default_prefs.save_password_local, ui.save_password_local->isChecked());
	if (ui.save_password_local->isChecked()) {
		SAVE_OR_REMOVE("password", default_prefs.cloud_storage_password, password);
	} else {
		s.remove("password");
		free(prefs.cloud_storage_password);
		prefs.cloud_storage_password = strdup(qPrintable(password));
	}
	SAVE_OR_REMOVE("cloud_verification_status", default_prefs.cloud_verification_status, prefs.cloud_verification_status);
	SAVE_OR_REMOVE("cloud_background_sync", default_prefs.cloud_background_sync, ui.cloud_background_sync->isChecked());

	// at this point we intentionally do not have a UI for changing this
	// it could go into some sort of "advanced setup" or something
	SAVE_OR_REMOVE("cloud_base_url", default_prefs.cloud_base_url, prefs.cloud_base_url);
	s.endGroup();

	s.beginGroup("geocoding");
#ifdef DISABLED
	s.setValue("enable_geocoding", ui.enable_geocoding->isChecked());
	s.setValue("parse_dive_without_gps", ui.parse_without_gps->isChecked());
	s.setValue("tag_existing_dives", ui.tag_existing_dives->isChecked());
#endif
	s.setValue("cat0", ui.first_item->currentIndex());
	s.setValue("cat1", ui.second_item->currentIndex());
	s.setValue("cat2", ui.third_item->currentIndex());
	s.endGroup();

	loadSettings();
	emit settingsChanged();
}

void PreferencesDialog::loadSettings()
{
	// This code was on the mainwindow, it should belong nowhere, but since we didn't
	// correctly fixed this code yet ( too much stuff on the code calling preferences )
	// force this here.
	loadPreferences();
	QSettings s;
	QVariant v;

	ui.save_uid_local->setChecked(s.value("save_uid_local").toBool());
	ui.default_uid->setText(s.value("subsurface_webservice_uid").toString().toUpper());

	ui.defaultfilename->setEnabled(prefs.default_file_behavior == LOCAL_DEFAULT_FILE);
	ui.btnUseDefaultFile->setEnabled(prefs.default_file_behavior == LOCAL_DEFAULT_FILE);
	ui.chooseFile->setEnabled(prefs.default_file_behavior == LOCAL_DEFAULT_FILE);
}

void PreferencesDialog::buttonClicked(QAbstractButton *button)
{
	switch (ui.buttonBox->standardButton(button)) {
	case QDialogButtonBox::Discard:
		restorePrefs();
		syncSettings();
		close();
		break;
	case QDialogButtonBox::Apply:
		syncSettings();
		break;
	case QDialogButtonBox::FirstButton:
		syncSettings();
		close();
		break;
	default:
		break; // ignore warnings.
	}
}
#undef SB

void PreferencesDialog::on_chooseFile_clicked()
{
	QFileInfo fi(system_default_filename());
	QString choosenFileName = QFileDialog::getOpenFileName(this, tr("Open default log file"), fi.absolutePath(), tr("Subsurface XML files (*.ssrf *.xml *.XML)"));

	if (!choosenFileName.isEmpty())
		ui.defaultfilename->setText(choosenFileName);
}

void PreferencesDialog::on_resetSettings_clicked()
{
	QSettings s;

	QMessageBox response(this);
	response.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
	response.setDefaultButton(QMessageBox::Cancel);
	response.setWindowTitle(tr("Warning"));
	response.setText(tr("If you click OK, all settings of Subsurface will be reset to their default values. This will be applied immediately."));
	response.setWindowModality(Qt::WindowModal);

	int result = response.exec();
	if (result == QMessageBox::Ok) {
		copy_prefs(&default_prefs, &prefs);
		setUiFromPrefs();
		Q_FOREACH (QString key, s.allKeys()) {
			s.remove(key);
		}
		syncSettings();
		close();
	}
}

void PreferencesDialog::passwordUpdateSuccessfull()
{
	ui.cloud_storage_password->setText(prefs.cloud_storage_password);
}

void PreferencesDialog::emitSettingsChanged()
{
	emit settingsChanged();
}

void PreferencesDialog::proxyType_changed(int idx)
{
	if (idx == -1) {
		return;
	}

	int proxyType = ui.proxyType->itemData(idx).toInt();
	bool hpEnabled = (proxyType == QNetworkProxy::Socks5Proxy || proxyType == QNetworkProxy::HttpProxy);
	ui.proxyHost->setEnabled(hpEnabled);
	ui.proxyPort->setEnabled(hpEnabled);
	ui.proxyAuthRequired->setEnabled(hpEnabled);
	ui.proxyUsername->setEnabled(hpEnabled & ui.proxyAuthRequired->isChecked());
	ui.proxyPassword->setEnabled(hpEnabled & ui.proxyAuthRequired->isChecked());
	ui.proxyAuthRequired->setChecked(ui.proxyAuthRequired->isChecked());
}

void PreferencesDialog::on_btnUseDefaultFile_toggled(bool toggle)
{
	if (toggle) {
		ui.defaultfilename->setText(system_default_filename());
		ui.defaultfilename->setEnabled(false);
	} else {
		ui.defaultfilename->setEnabled(true);
	}
}

void PreferencesDialog::on_noDefaultFile_toggled(bool toggle)
{
	prefs.default_file_behavior = NO_DEFAULT_FILE;
}

void PreferencesDialog::on_localDefaultFile_toggled(bool toggle)
{
	ui.defaultfilename->setEnabled(toggle);
	ui.btnUseDefaultFile->setEnabled(toggle);
	ui.chooseFile->setEnabled(toggle);
	prefs.default_file_behavior = LOCAL_DEFAULT_FILE;
}

void PreferencesDialog::on_cloudDefaultFile_toggled(bool toggle)
{
	prefs.default_file_behavior = CLOUD_DEFAULT_FILE;
}
