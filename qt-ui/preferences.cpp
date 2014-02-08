#include "preferences.h"
#include "mainwindow.h"
#include <QSettings>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QSortFilterProxyModel>

PreferencesDialog* PreferencesDialog::instance()
{
	static PreferencesDialog *dialog = new PreferencesDialog(mainWindow());
	dialog->setAttribute(Qt::WA_QuitOnClose, false);
	LanguageModel::instance();
	return dialog;
}

PreferencesDialog::PreferencesDialog(QWidget* parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
	connect(ui.gflow, SIGNAL(valueChanged(int)), this, SLOT(gflowChanged(int)));
	connect(ui.gfhigh, SIGNAL(valueChanged(int)), this, SLOT(gfhighChanged(int)));
	loadSettings();
	setUiFromPrefs();
	rememberPrefs();
}

#define DANGER_GF ( gf > 100 ) ? "* { color: red; }" : ""
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
	ui.phe->setChecked(prefs.pp_graphs.phe);
	ui.pheThreshold->setEnabled(ui.phe->isChecked());
	ui.po2->setChecked(prefs.pp_graphs.po2);
	ui.po2Threshold->setEnabled(ui.po2->isChecked());
	ui.pn2->setChecked(prefs.pp_graphs.pn2);
	ui.pn2Threshold->setEnabled(ui.pn2->isChecked());
	ui.pheThreshold->setValue(prefs.pp_graphs.phe_threshold);
	ui.po2Threshold->setValue(prefs.pp_graphs.po2_threshold);
	ui.pn2Threshold->setValue(prefs.pp_graphs.pn2_threshold);
	ui.ead_end_eadd->setChecked(prefs.ead);
	ui.mod->setChecked(prefs.mod);
	ui.maxppo2->setEnabled(ui.mod->isChecked());
	ui.maxppo2->setValue(prefs.mod_ppO2);
	ui.dc_reported_ceiling->setChecked(prefs.profile_dc_ceiling);
	ui.red_ceiling->setEnabled(ui.dc_reported_ceiling->isChecked());
	ui.red_ceiling->setChecked(prefs.profile_red_ceiling);
	ui.calculated_ceiling->setChecked(prefs.profile_calc_ceiling);
	ui.increment_3m->setEnabled(ui.calculated_ceiling->isChecked());
	ui.increment_3m->setChecked(prefs.calc_ceiling_3m_incr);
	ui.all_tissues->setEnabled(ui.calculated_ceiling->isChecked());
	ui.all_tissues->setChecked(prefs.calc_all_tissues);
	ui.calc_ndl_tts->setEnabled(ui.calculated_ceiling->isChecked());
	ui.calc_ndl_tts->setChecked(prefs.calc_ndl_tts);
	ui.units_group->setEnabled(ui.personalize->isChecked());

	ui.gflow->setValue(prefs.gflow);
	ui.gfhigh->setValue(prefs.gfhigh);
	ui.gf_low_at_maxdepth->setChecked(prefs.gf_low_at_maxdepth);

	// units
	if (prefs.unit_system == METRIC)
		ui.metric->setChecked(true);
	else if (prefs.unit_system == IMPERIAL)
		ui.imperial->setChecked(true);
	else
		ui.personalize->setChecked(true);

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
	ui.default_cylinder->clear();
	for(int i=0; tank_info[i].name != NULL; i++) {
		ui.default_cylinder->addItem(tank_info[i].name);
		if (prefs.default_cylinder && strcmp(tank_info[i].name, prefs.default_cylinder) == 0)
			ui.default_cylinder->setCurrentIndex(i);
	}
	ui.displayinvalid->setChecked(prefs.display_invalid_dives);
	ui.display_unused_tanks->setChecked(prefs.display_unused_tanks);
	ui.show_sac->setChecked(prefs.show_sac);
	ui.vertical_speed_minutes->setChecked(prefs.units.vertical_speed_time == units::MINUTES);
	ui.vertical_speed_seconds->setChecked(prefs.units.vertical_speed_time == units::SECONDS);

	QSortFilterProxyModel *filterModel = new QSortFilterProxyModel();
	filterModel->setSourceModel(LanguageModel::instance());
	filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	ui.languageView->setModel(filterModel);
	filterModel->sort(0);
	connect(ui.languageFilter, SIGNAL(textChanged(QString)), filterModel, SLOT(setFilterFixedString(QString)));

	QSettings s;
	s.beginGroup("Language");
	ui.languageSystemDefault->setChecked(s.value("UseSystemLanguage", true).toBool());
	QAbstractItemModel *m = ui.languageView->model();
	QModelIndexList languages = m->match( m->index(0,0), Qt::UserRole, s.value("UiLanguage").toString());
	if (languages.count())
		ui.languageView->setCurrentIndex(languages.first());
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

void PreferencesDialog::syncSettings()
{
	QSettings s;

	// Graph
	s.beginGroup("TecDetails");

	SB("phegraph", ui.phe);
	SB("po2graph", ui.po2);
	SB("pn2graph", ui.pn2);
	s.setValue("phethreshold", ui.pheThreshold->value());
	s.setValue("po2threshold", ui.po2Threshold->value());
	s.setValue("pn2threshold", ui.pn2Threshold->value());
	SB("ead", ui.ead_end_eadd);
	SB("mod", ui.mod);
	s.setValue("modppO2", ui.maxppo2->value());
	SB("dcceiling", ui.dc_reported_ceiling);
	SB("redceiling", ui.red_ceiling);
	SB("calcceiling", ui.calculated_ceiling);
	SB("calcceiling3m", ui.increment_3m);
	prefs.calc_ceiling_3m_incr = ui.increment_3m->isChecked() ? 1 : 0;
	SB("calcndltts", ui.calc_ndl_tts);
	SB("calcalltissues", ui.all_tissues);
	s.setValue("gflow", ui.gflow->value());
	s.setValue("gfhigh", ui.gfhigh->value());
	SB("gf_low_at_maxdepth", ui.gf_low_at_maxdepth);
	SB("show_sac", ui.show_sac);
	SB("display_unused_tanks", ui.display_unused_tanks);
	s.endGroup();

	// Units
	s.beginGroup("Units");
	QString unitSystem = ui.metric->isChecked() ? "metric" : (ui.imperial->isChecked() ? "imperial" : "personal");
	s.setValue("unit_system", unitSystem);
	s.setValue("temperature", ui.fahrenheit->isChecked() ? units::FAHRENHEIT : units::CELSIUS);
	s.setValue("length", ui.feet->isChecked() ? units::FEET : units::METERS);
	s.setValue("pressure", ui.psi->isChecked() ? units::PSI : units::BAR);
	s.setValue("volume", ui.cuft->isChecked() ? units::CUFT : units::LITER);
	s.setValue("weight", ui.lbs->isChecked() ? units::LBS : units::KG);
	s.setValue("vertical_speed_time", ui.vertical_speed_minutes->isChecked() ? units::MINUTES : units::SECONDS);
	s.endGroup();
	// Defaults
	s.beginGroup("GeneralSettings");
	s.setValue("default_filename", ui.defaultfilename->text());
	s.setValue("default_cylinder", ui.default_cylinder->currentText());
	s.endGroup();

	s.beginGroup("Display");
	s.setValue("divelist_font", ui.font->currentFont());
	s.setValue("font_size", ui.fontsize->value());
	s.setValue("displayinvalid", ui.displayinvalid->isChecked());
	s.endGroup();
	s.sync();

	QLocale loc;
	s.beginGroup("Language");
	bool useSystemLang = s.value("UseSystemLanguage", true).toBool();
	if (useSystemLang != ui.languageSystemDefault->isChecked() ||
	    (!useSystemLang && s.value("UiLanguage").toString() != ui.languageView->currentIndex().data(Qt::UserRole))) {
		QMessageBox::warning(mainWindow(), tr("Restart required"),
				     tr("To correctly load a new language you must restart Subsurface."));
	}
	s.setValue("UseSystemLanguage", ui.languageSystemDefault->isChecked());
	s.setValue("UiLanguage", ui.languageView->currentIndex().data(Qt::UserRole));
	s.endGroup();

	loadSettings();
	emit settingsChanged();
}

void PreferencesDialog::loadSettings()
{
	// This code was on the mainwindow, it should belong nowhere, but since we dind't
	// correctly fixed this code yet ( too much stuff on the code calling preferences )
	// force this here.

	QSettings s;
	QVariant v;
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

void PreferencesDialog::buttonClicked(QAbstractButton* button)
{
	switch (ui.buttonBox->standardButton(button)) {
	case QDialogButtonBox::Discard:
		restorePrefs();
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
	ui.defaultfilename->setText(QFileDialog::getOpenFileName(this, tr("Open Default Log File"), fi.absolutePath(), tr("Subsurface XML files (*.ssrf *.xml *.XML)")));
}

void PreferencesDialog::emitSettingsChanged()
{
	emit settingsChanged();
}
