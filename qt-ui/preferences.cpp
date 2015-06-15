#include "preferences.h"
#include "mainwindow.h"
#include "models.h"

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QNetworkProxy>
#include <QNetworkCookieJar>

#include "subsurfacewebservices.h"

#if defined(FBSUPPORT)
#include "socialnetworks.h"
#endif

#ifndef Q_OS_ANDROID
#include <QWebView>
#endif

PreferencesDialog *PreferencesDialog::instance()
{
	static PreferencesDialog *dialog = new PreferencesDialog(MainWindow::instance());
	dialog->setAttribute(Qt::WA_QuitOnClose, false);
	LanguageModel::instance();
	return dialog;
}

PreferencesDialog::PreferencesDialog(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	ui.setupUi(this);

#if defined(Q_OS_ANDROID) || !defined(FBSUPPORT)
	for (int i = 0; i < ui.listWidget->count(); i++) {
		if (ui.listWidget->item(i)->text() == "Facebook")
			delete ui.listWidget->item(i);
	}
#endif

	ui.proxyType->clear();
	ui.proxyType->addItem(tr("No proxy"), QNetworkProxy::NoProxy);
	ui.proxyType->addItem(tr("System proxy"), QNetworkProxy::DefaultProxy);
	ui.proxyType->addItem(tr("HTTP proxy"), QNetworkProxy::HttpProxy);
	ui.proxyType->addItem(tr("SOCKS proxy"), QNetworkProxy::Socks5Proxy);
	ui.proxyType->setCurrentIndex(-1);

	// Facebook stuff:
#if !defined(Q_OS_ANDROID) && defined(FBSUPPORT)
	FacebookManager *fb = FacebookManager::instance();
	facebookWebView = new QWebView(this);
	if (fb->loggedIn()) {
		facebookLoggedIn();
	} else {
		facebookDisconnect();
	}
	connect(facebookWebView, &QWebView::urlChanged, fb, &FacebookManager::tryLogin);
	connect(fb, &FacebookManager::justLoggedIn, this, &PreferencesDialog::facebookLoggedIn);
	connect(ui.btnDisconnectFacebook, &QPushButton::clicked, fb, &FacebookManager::logout);
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
#ifndef Q_OS_ANDROID
	// remove the login view and add the disconnect button
	ui.fbLayout->removeItem(ui.fbLayout->itemAt(1));
	ui.fbLayout->addWidget(ui.fbConnected);
	ui.fbConnected->show();
	ui.FBLabel->setText(tr("To disconnect Subsurface from your Facebook account, use the button below"));
	if (facebookWebView)
		facebookWebView->hide();
#endif
}

void PreferencesDialog::facebookDisconnect()
{
#if !defined(Q_OS_ANDROID) && defined(FBSUPPORT)
	// remove the connect/disconnect button
	// and instead add the login view
	ui.fbLayout->removeItem(ui.fbLayout->itemAt(1));
	ui.fbLayout->addWidget(facebookWebView);
	ui.fbConnected->hide();
	ui.FBLabel->setText(tr("To connect to Facebook, please log in. This enables Subsurface to publish dives to your timeline"));
	if (facebookWebView) {
		facebookWebView->page()->networkAccessManager()->setCookieJar(new QNetworkCookieJar());
		facebookWebView->setUrl(FacebookManager::instance()->connectUrl());
		facebookWebView->show();
	}
#endif
}

void PreferencesDialog::cloudPinNeeded()
{
	ui.cloud_storage_pin->setEnabled(prefs.cloud_verification_status == CS_NEED_TO_VERIFY);
	ui.cloud_storage_pin->setVisible(prefs.cloud_verification_status == CS_NEED_TO_VERIFY);
	ui.cloud_storage_pin_label->setEnabled(prefs.cloud_verification_status == CS_NEED_TO_VERIFY);
	ui.cloud_storage_pin_label->setVisible(prefs.cloud_verification_status == CS_NEED_TO_VERIFY);
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

#define SB(V, B) s.setValue(V, (int)(B->isChecked() ? 1 : 0))


#define GET_UNIT(name, field, f, t)                                 \
	v = s.value(QString(name));                                 \
	if (v.isValid())                                            \
		prefs.units.field = (v.toInt() == (t)) ? (t) : (f); \
	else                                                        \
		prefs.units.field = default_prefs.units.field

#define GET_BOOL(name, field)                           \
	v = s.value(QString(name));                     \
	if (v.isValid())                                \
		prefs.field = v.toBool();               \
	else                                            \
		prefs.field = default_prefs.field

#define GET_DOUBLE(name, field)             \
	v = s.value(QString(name));         \
	if (v.isValid())                    \
		prefs.field = v.toDouble(); \
	else                                \
		prefs.field = default_prefs.field

#define GET_INT(name, field)             \
	v = s.value(QString(name));      \
	if (v.isValid())                 \
		prefs.field = v.toInt(); \
	else                             \
		prefs.field = default_prefs.field

#define GET_INT_DEF(name, field, defval)                                             \
	v = s.value(QString(name));                                      \
	if (v.isValid())                                                 \
		prefs.field = v.toInt(); \
	else                                                             \
		prefs.field = defval

#define GET_TXT(name, field)                                             \
	v = s.value(QString(name));                                      \
	if (v.isValid())                                                 \
		prefs.field = strdup(v.toString().toUtf8().constData()); \
	else                                                             \
		prefs.field = default_prefs.field

#define SAVE_OR_REMOVE_SPECIAL(_setting, _default, _compare, _value)     \
	if (_compare != _default)                                        \
		s.setValue(_setting, _value);                            \
	else                                                             \
		s.remove(_setting)

#define SAVE_OR_REMOVE(_setting, _default, _value)                       \
	if (_value != _default)                                          \
		s.setValue(_setting, _value);                            \
	else                                                             \
		s.remove(_setting)

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
	if (prefs.cloud_verification_status == CS_UNKNOWN ||
	    prefs.cloud_verification_status == CS_INCORRECT_USER_PASSWD ||
	    email != prefs.cloud_storage_email ||
	    password != prefs.cloud_storage_password) {
		// different credentials - reset verification status
		prefs.cloud_verification_status = CS_UNKNOWN;

		// connect to backend server to check / create credentials
		QRegularExpression reg("^[a-zA-Z0-9@.+_-]+$");
		if (!reg.match(email).hasMatch() || !reg.match(password).hasMatch()) {
			report_error(qPrintable(tr("Cloud storage email and password can only consist of letters, numbers, and '.', '-', '_', and '+'.")));
		}
		CloudStorageAuthenticate *cloudAuth = new CloudStorageAuthenticate(this);
		connect(cloudAuth, SIGNAL(finishedAuthenticate()), this, SLOT(cloudPinNeeded()));
		QNetworkReply *reply = cloudAuth->authenticate(email, password);
	} else if (prefs.cloud_verification_status == CS_NEED_TO_VERIFY) {
		QString pin = ui.cloud_storage_pin->text();
		if (!pin.isEmpty()) {
			// connect to backend server to check / create credentials
			QRegularExpression reg("^[a-zA-Z0-9@.+_-]+$");
			if (!reg.match(email).hasMatch() || !reg.match(password).hasMatch()) {
				report_error(qPrintable(tr("Cloud storage email and password can only consist of letters, numbers, and '.', '-', '_', and '+'.")));
			}
			CloudStorageAuthenticate *cloudAuth = new CloudStorageAuthenticate(this);
			QNetworkReply *reply = cloudAuth->authenticate(email, password, pin);
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
	loadSettings();
	emit settingsChanged();
}

void PreferencesDialog::loadSettings()
{
	// This code was on the mainwindow, it should belong nowhere, but since we didn't
	// correctly fixed this code yet ( too much stuff on the code calling preferences )
	// force this here.

	QSettings s;
	QVariant v;

	ui.save_uid_local->setChecked(s.value("save_uid_local").toBool());
	ui.default_uid->setText(s.value("subsurface_webservice_uid").toString().toUpper());

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
	GET_BOOL("coordinates", coordinates_traditional);
	s.endGroup();
	s.beginGroup("TecDetails");
	GET_BOOL("po2graph", pp_graphs.po2);
	GET_BOOL("pn2graph", pp_graphs.pn2);
	GET_BOOL("phegraph", pp_graphs.phe);
	GET_DOUBLE("po2threshold", pp_graphs.po2_threshold);
	GET_DOUBLE("pn2threshold", pp_graphs.pn2_threshold);
	GET_DOUBLE("phethreshold", pp_graphs.phe_threshold);
	GET_BOOL("mod", mod);
	GET_DOUBLE("modpO2", modpO2);
	GET_BOOL("ead", ead);
	GET_BOOL("redceiling", redceiling);
	GET_BOOL("dcceiling", dcceiling);
	GET_BOOL("calcceiling", calcceiling);
	GET_BOOL("calcceiling3m", calcceiling3m);
	GET_BOOL("calcndltts", calcndltts);
	GET_BOOL("calcalltissues", calcalltissues);
	GET_BOOL("hrgraph", hrgraph);
	GET_BOOL("tankbar", tankbar);
	GET_BOOL("percentagegraph", percentagegraph);
	GET_INT("gflow", gflow);
	GET_INT("gfhigh", gfhigh);
	GET_BOOL("gf_low_at_maxdepth", gf_low_at_maxdepth);
	GET_BOOL("show_ccr_setpoint",show_ccr_setpoint);
	GET_BOOL("show_ccr_sensors",show_ccr_sensors);
	GET_BOOL("zoomed_plot", zoomed_plot);
	set_gf(prefs.gflow, prefs.gfhigh, prefs.gf_low_at_maxdepth);
	GET_BOOL("show_sac", show_sac);
	GET_BOOL("display_unused_tanks", display_unused_tanks);
	GET_BOOL("show_average_depth", show_average_depth);
	s.endGroup();

	s.beginGroup("GeneralSettings");
	GET_TXT("default_filename", default_filename);
	GET_INT("default_file_behavior", default_file_behavior);
	if (prefs.default_file_behavior == UNDEFINED_DEFAULT_FILE) {
		// undefined, so check if there's a filename set and
		// use that, otherwise go with no default file
		if (QString(prefs.default_filename).isEmpty())
			prefs.default_file_behavior = NO_DEFAULT_FILE;
		else
			prefs.default_file_behavior = LOCAL_DEFAULT_FILE;
	}
	ui.defaultfilename->setEnabled(prefs.default_file_behavior == LOCAL_DEFAULT_FILE);
	ui.btnUseDefaultFile->setEnabled(prefs.default_file_behavior == LOCAL_DEFAULT_FILE);
	ui.chooseFile->setEnabled(prefs.default_file_behavior == LOCAL_DEFAULT_FILE);
	GET_TXT("default_cylinder", default_cylinder);
	GET_BOOL("use_default_file", use_default_file);
	GET_INT("defaultsetpoint", defaultsetpoint);
	GET_INT("o2consumption", o2consumption);
	GET_INT("pscr_ratio", pscr_ratio);
	s.endGroup();

	s.beginGroup("Display");
	// get the font from the settings or our defaults
	// respect the system default font size if none is explicitly set
	QFont defaultFont = s.value("divelist_font", prefs.divelist_font).value<QFont>();
	if (IS_FP_SAME(system_divelist_default_font_size, -1.0)) {
		prefs.font_size = qApp->font().pointSizeF();
		system_divelist_default_font_size = prefs.font_size; // this way we don't save it on exit
	}
	prefs.font_size = s.value("font_size", prefs.font_size).toFloat();
	// painful effort to ignore previous default fonts on Windows - ridiculous
	QString fontName = defaultFont.toString();
	if (fontName.contains(","))
		fontName = fontName.left(fontName.indexOf(","));
	if (subsurface_ignore_font(fontName.toUtf8().constData())) {
		defaultFont = QFont(prefs.divelist_font);
	} else {
		free((void *)prefs.divelist_font);
		prefs.divelist_font = strdup(fontName.toUtf8().constData());
	}
	defaultFont.setPointSizeF(prefs.font_size);
	qApp->setFont(defaultFont);
	GET_INT("displayinvalid", display_invalid_dives);
	s.endGroup();

	s.beginGroup("Animations");
	GET_INT("animation_speed", animation_speed);
	s.endGroup();

	s.beginGroup("Network");
	GET_INT_DEF("proxy_type", proxy_type, QNetworkProxy::DefaultProxy);
	GET_TXT("proxy_host", proxy_host);
	GET_INT("proxy_port", proxy_port);
	GET_BOOL("proxy_auth", proxy_auth);
	GET_TXT("proxy_user", proxy_user);
	GET_TXT("proxy_pass", proxy_pass);
	s.endGroup();

	s.beginGroup("CloudStorage");
	GET_TXT("email", cloud_storage_email);
	GET_BOOL("save_password_local", save_password_local);
	if (prefs.save_password_local) { // GET_TEXT macro is not a single statement
		GET_TXT("password", cloud_storage_password);
	}
	GET_INT("cloud_verification_status", cloud_verification_status);
	GET_BOOL("cloud_background_sync", cloud_background_sync);

	// creating the git url here is simply a convenience when C code wants
	// to compare against that git URL - it's always derived from the base URL
	GET_TXT("cloud_base_url", cloud_base_url);
	prefs.cloud_git_url = strdup(qPrintable(QString(prefs.cloud_base_url) + "/git"));
	s.endGroup();
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
		prefs = default_prefs;
		setUiFromPrefs();
		Q_FOREACH (QString key, s.allKeys()) {
			s.remove(key);
		}
		syncSettings();
		close();
	}
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
