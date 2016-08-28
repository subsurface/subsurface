#include "SettingsObjectWrapper.h"
#include <QSettings>
#include <QApplication>
#include <QFont>
#include <QDate>
#include <QNetworkProxy>

#include "core/dive.h" // TODO: remove copy_string from dive.h
#include "core/helpers.h"

DiveComputerSettings::DiveComputerSettings(QObject *parent):
	QObject(parent), group(QStringLiteral("DiveComputer"))
{
}

QString DiveComputerSettings::dc_vendor() const
{
	return prefs.dive_computer.vendor;
}

QString DiveComputerSettings::dc_product() const
{
	return prefs.dive_computer.product;
}

QString DiveComputerSettings::dc_device() const
{
	return prefs.dive_computer.device;
}

int DiveComputerSettings::downloadMode() const
{
	return prefs.dive_computer.download_mode;
}

void DiveComputerSettings::setVendor(const QString& vendor)
{
	if (vendor == prefs.dive_computer.vendor)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("dive_computer_vendor", vendor);
	free(prefs.dive_computer.vendor);
	prefs.dive_computer.vendor = copy_string(qPrintable(vendor));
}

void DiveComputerSettings::setProduct(const QString& product)
{
	if (product == prefs.dive_computer.product)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("dive_computer_product", product);
	free(prefs.dive_computer.product);
	prefs.dive_computer.product = copy_string(qPrintable(product));
}

void DiveComputerSettings::setDevice(const QString& device)
{
	if (device == prefs.dive_computer.device)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("dive_computer_device", device);
	free(prefs.dive_computer.device);
	prefs.dive_computer.device = copy_string(qPrintable(device));
}

void DiveComputerSettings::setDownloadMode(int mode)
{
	if (mode == prefs.dive_computer.download_mode)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("dive_computer_download_mode", mode);
	prefs.dive_computer.download_mode = mode;
}

UpdateManagerSettings::UpdateManagerSettings(QObject *parent) : QObject(parent), group("UpdateManager")
{

}

bool UpdateManagerSettings::dontCheckForUpdates() const
{
	return prefs.update_manager.dont_check_for_updates;
}

bool UpdateManagerSettings::dontCheckExists() const
{
	return prefs.update_manager.dont_check_exists;
}

QString UpdateManagerSettings::lastVersionUsed() const
{
	return prefs.update_manager.last_version_used;
}

QDate UpdateManagerSettings::nextCheck() const
{
	return QDate::fromString(QString(prefs.update_manager.next_check));
}

void UpdateManagerSettings::setDontCheckForUpdates(bool value)
{
	if (value == prefs.update_manager.dont_check_for_updates)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("DontCheckForUpdates", value);
	prefs.update_manager.dont_check_for_updates = value;
	prefs.update_manager.dont_check_exists = true;
	emit dontCheckForUpdatesChanged(value);
}

void UpdateManagerSettings::setLastVersionUsed(const QString& value)
{
	if (value == prefs.update_manager.last_version_used)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("LastVersionUsed", value);
	free (prefs.update_manager.last_version_used);
	prefs.update_manager.last_version_used = copy_string(qPrintable(value));
	emit lastVersionUsedChanged(value);
}

void UpdateManagerSettings::setNextCheck(const QDate& date)
{
	if (date.toString() == prefs.update_manager.next_check)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("NextCheck", date);
	free (prefs.update_manager.next_check);
	prefs.update_manager.next_check = copy_string(qPrintable(date.toString()));
	emit nextCheckChanged(date);
}

static QString tecDetails = QStringLiteral("TecDetails");

PartialPressureGasSettings::PartialPressureGasSettings(QObject* parent):
	QObject(parent),
	group("TecDetails")
{

}

short PartialPressureGasSettings::showPo2() const
{
	return prefs.pp_graphs.po2;
}

short PartialPressureGasSettings::showPn2() const
{
	return prefs.pp_graphs.pn2;
}

short PartialPressureGasSettings::showPhe() const
{
	return prefs.pp_graphs.phe;
}

double PartialPressureGasSettings::po2Threshold() const
{
	return prefs.pp_graphs.po2_threshold;
}

double PartialPressureGasSettings::pn2Threshold() const
{
	return prefs.pp_graphs.pn2_threshold;
}

double PartialPressureGasSettings::pheThreshold() const
{
	return prefs.pp_graphs.phe_threshold;
}

void PartialPressureGasSettings::setShowPo2(short value)
{
	if (value == prefs.pp_graphs.po2)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("po2graph", value);
	prefs.pp_graphs.po2 = value;
	emit showPo2Changed(value);
}

void PartialPressureGasSettings::setShowPn2(short value)
{
	if (value == prefs.pp_graphs.pn2)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("pn2graph", value);
	prefs.pp_graphs.pn2 = value;
	emit showPn2Changed(value);
}

void PartialPressureGasSettings::setShowPhe(short value)
{
	if (value == prefs.pp_graphs.phe)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("phegraph", value);
	prefs.pp_graphs.phe = value;
	emit showPheChanged(value);
}

void PartialPressureGasSettings::setPo2Threshold(double value)
{
	if (value == prefs.pp_graphs.po2_threshold)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("po2threshold", value);
	prefs.pp_graphs.po2_threshold = value;
	emit po2ThresholdChanged(value);
}

void PartialPressureGasSettings::setPn2Threshold(double value)
{
	if (value == prefs.pp_graphs.pn2_threshold)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("pn2threshold", value);
	prefs.pp_graphs.pn2_threshold = value;
	emit pn2ThresholdChanged(value);
}

void PartialPressureGasSettings::setPheThreshold(double value)
{
	if (value == prefs.pp_graphs.phe_threshold)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("phethreshold", value);
	prefs.pp_graphs.phe_threshold = value;
	emit pheThresholdChanged(value);
}


TechnicalDetailsSettings::TechnicalDetailsSettings(QObject* parent): QObject(parent)
{

}

double TechnicalDetailsSettings:: modp02() const
{
	return prefs.modpO2;
}

bool TechnicalDetailsSettings::ead() const
{
	return prefs.ead;
}

bool TechnicalDetailsSettings::dcceiling() const
{
	return prefs.dcceiling;
}

bool TechnicalDetailsSettings::redceiling() const
{
	return prefs.redceiling;
}

bool TechnicalDetailsSettings::calcceiling() const
{
	return prefs.calcceiling;
}

bool TechnicalDetailsSettings::calcceiling3m() const
{
	return prefs.calcceiling3m;
}

bool TechnicalDetailsSettings::calcalltissues() const
{
	return prefs.calcalltissues;
}

bool TechnicalDetailsSettings::calcndltts() const
{
	return prefs.calcndltts;
}

bool TechnicalDetailsSettings::gflow() const
{
	return prefs.gflow;
}

bool TechnicalDetailsSettings::gfhigh() const
{
	return prefs.gfhigh;
}

bool TechnicalDetailsSettings::hrgraph() const
{
	return prefs.hrgraph;
}

bool TechnicalDetailsSettings::tankBar() const
{
	return prefs.tankbar;
}

bool TechnicalDetailsSettings::percentageGraph() const
{
	return prefs.percentagegraph;
}

bool TechnicalDetailsSettings::rulerGraph() const
{
	return prefs.rulergraph;
}

bool TechnicalDetailsSettings::showCCRSetpoint() const
{
	return prefs.show_ccr_setpoint;
}

bool TechnicalDetailsSettings::showCCRSensors() const
{
	return prefs.show_ccr_sensors;
}

bool TechnicalDetailsSettings::zoomedPlot() const
{
	return prefs.zoomed_plot;
}

bool TechnicalDetailsSettings::showSac() const
{
	return prefs.show_sac;
}

bool TechnicalDetailsSettings::gfLowAtMaxDepth() const
{
	return prefs.gf_low_at_maxdepth;
}

bool TechnicalDetailsSettings::displayUnusedTanks() const
{
	return prefs.display_unused_tanks;
}

bool TechnicalDetailsSettings::showAverageDepth() const
{
	return prefs.show_average_depth;
}

bool TechnicalDetailsSettings::mod() const
{
	return prefs.mod;
}

bool TechnicalDetailsSettings::showPicturesInProfile() const
{
	return prefs.show_pictures_in_profile;
}

void TechnicalDetailsSettings::setModp02(double value)
{
	if (value == prefs.modpO2)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("modpO2", value);
	prefs.modpO2 = value;
	emit modpO2Changed(value);
}

void TechnicalDetailsSettings::setShowPicturesInProfile(bool value)
{
	if (value == prefs.show_pictures_in_profile)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("show_pictures_in_profile", value);
	prefs.show_pictures_in_profile = value;
	emit showPicturesInProfileChanged(value);
}

void TechnicalDetailsSettings::setEad(bool value)
{
	if (value == prefs.ead)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("ead", value);
	prefs.ead = value;
	emit eadChanged(value);
}

void TechnicalDetailsSettings::setMod(bool value)
{
	if (value == prefs.mod)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("mod", value);
	prefs.mod = value;
	emit modChanged(value);
}

void TechnicalDetailsSettings::setDCceiling(bool value)
{
	if (value == prefs.dcceiling)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("dcceiling", value);
	prefs.dcceiling = value;
	emit dcceilingChanged(value);
}

void TechnicalDetailsSettings::setRedceiling(bool value)
{
	if (value == prefs.redceiling)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("redceiling", value);
	prefs.redceiling = value;
	emit redceilingChanged(value);
}

void TechnicalDetailsSettings::setCalcceiling(bool value)
{
	if (value == prefs.calcceiling)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("calcceiling", value);
	prefs.calcceiling = value;
	emit calcceilingChanged(value);
}

void TechnicalDetailsSettings::setCalcceiling3m(bool value)
{
	if (value == prefs.calcceiling3m)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("calcceiling3m", value);
	prefs.calcceiling3m = value;
	emit calcceiling3mChanged(value);
}

void TechnicalDetailsSettings::setCalcalltissues(bool value)
{
	if (value == prefs.calcalltissues)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("calcalltissues", value);
	prefs.calcalltissues = value;
	emit calcalltissuesChanged(value);
}

void TechnicalDetailsSettings::setCalcndltts(bool value)
{
	if (value == prefs.calcndltts)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("calcndltts", value);
	prefs.calcndltts = value;
	emit calcndlttsChanged(value);
}

void TechnicalDetailsSettings::setGflow(bool value)
{
	if (value == prefs.gflow)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("gflow", value);
	prefs.gflow = value;
	set_gf(prefs.gflow, prefs.gfhigh, prefs.gf_low_at_maxdepth);
	emit gflowChanged(value);
}

void TechnicalDetailsSettings::setGfhigh(bool value)
{
	if (value == prefs.gfhigh)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("gfhigh", value);
	prefs.gfhigh = value;
	set_gf(prefs.gflow, prefs.gfhigh, prefs.gf_low_at_maxdepth);
	emit gfhighChanged(value);
}

void TechnicalDetailsSettings::setHRgraph(bool value)
{
	if (value == prefs.hrgraph)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("hrgraph", value);
	prefs.hrgraph = value;
	emit hrgraphChanged(value);
}

void TechnicalDetailsSettings::setTankBar(bool value)
{
	if (value == prefs.tankbar)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("tankbar", value);
	prefs.tankbar = value;
	emit tankBarChanged(value);
}

void TechnicalDetailsSettings::setPercentageGraph(bool value)
{
	if (value == prefs.percentagegraph)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("percentagegraph", value);
	prefs.percentagegraph = value;
	emit percentageGraphChanged(value);
}

void TechnicalDetailsSettings::setRulerGraph(bool value)
{
	if (value == prefs.rulergraph)
		return;
	/* TODO: search for the QSettings of the RulerBar */
	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("RulerBar", value);
	prefs.rulergraph = value;
	emit rulerGraphChanged(value);
}

void TechnicalDetailsSettings::setShowCCRSetpoint(bool value)
{
	if (value == prefs.show_ccr_setpoint)
		return;

	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("show_ccr_setpoint", value);
	prefs.show_ccr_setpoint = value;
	emit showCCRSetpointChanged(value);
}

void TechnicalDetailsSettings::setShowCCRSensors(bool value)
{
	if (value == prefs.show_ccr_sensors)
		return;
	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("show_ccr_sensors", value);
	prefs.show_ccr_sensors = value;
	emit showCCRSensorsChanged(value);
}

void TechnicalDetailsSettings::setZoomedPlot(bool value)
{
	if (value == prefs.zoomed_plot)
		return;
	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("zoomed_plot", value);
	prefs.zoomed_plot = value;
	emit zoomedPlotChanged(value);
}

void TechnicalDetailsSettings::setShowSac(bool value)
{
	if (value == prefs.show_sac)
		return;
	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("show_sac", value);
	prefs.show_sac = value;
	emit showSacChanged(value);
}

void TechnicalDetailsSettings::setGfLowAtMaxDepth(bool value)
{
	if (value == prefs.gf_low_at_maxdepth)
		return;
	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("gf_low_at_maxdepth", value);
	prefs.gf_low_at_maxdepth = value;
	set_gf(prefs.gflow, prefs.gfhigh, prefs.gf_low_at_maxdepth);
	emit gfLowAtMaxDepthChanged(value);
}

void TechnicalDetailsSettings::setDisplayUnusedTanks(bool value)
{
	if (value == prefs.display_unused_tanks)
		return;
	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("display_unused_tanks", value);
	prefs.display_unused_tanks = value;
	emit displayUnusedTanksChanged(value);
}

void TechnicalDetailsSettings::setShowAverageDepth(bool value)
{
	if (value == prefs.show_average_depth)
		return;
	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("show_average_depth", value);
	prefs.show_average_depth = value;
	emit showAverageDepthChanged(value);
}

FacebookSettings::FacebookSettings(QObject *parent) :
	QObject(parent),
	group(QStringLiteral("WebApps")),
	subgroup(QStringLiteral("Facebook"))
{
}

QString FacebookSettings::accessToken() const
{
	return QString(prefs.facebook.access_token);
}

QString FacebookSettings::userId() const
{
	return QString(prefs.facebook.user_id);
}

QString FacebookSettings::albumId() const
{
	return QString(prefs.facebook.album_id);
}

void FacebookSettings::setAccessToken (const QString& value)
{
#if SAVE_FB_CREDENTIALS
	QSettings s;
	s.beginGroup(group);
	s.beginGroup(subgroup);
	s.setValue("ConnectToken", value);
#endif
	prefs.facebook.access_token = copy_string(qPrintable(value));
	emit accessTokenChanged(value);
}

void FacebookSettings::setUserId(const QString& value)
{
	if (value == prefs.facebook.user_id)
		return;
#if SAVE_FB_CREDENTIALS
	QSettings s;
	s.beginGroup(group);
	s.beginGroup(subgroup);
	s.setValue("UserId", value);
#endif
	prefs.facebook.user_id = copy_string(qPrintable(value));
	emit userIdChanged(value);
}

void FacebookSettings::setAlbumId(const QString& value)
{
	if (value == prefs.facebook.album_id)
		return;
#if SAVE_FB_CREDENTIALS
	QSettings s;
	s.beginGroup(group);
	s.beginGroup(subgroup);
	s.setValue("AlbumId", value);
#endif
	prefs.facebook.album_id = copy_string(qPrintable(value));
	emit albumIdChanged(value);
}


GeocodingPreferences::GeocodingPreferences(QObject *parent) :
	QObject(parent),
	group(QStringLiteral("geocoding"))
{

}

bool GeocodingPreferences::enableGeocoding() const
{
	return prefs.geocoding.enable_geocoding;
}

bool GeocodingPreferences::parseDiveWithoutGps() const
{
	return prefs.geocoding.parse_dive_without_gps;
}

bool GeocodingPreferences::tagExistingDives() const
{
	return prefs.geocoding.tag_existing_dives;
}

taxonomy_category GeocodingPreferences::firstTaxonomyCategory() const
{
	return prefs.geocoding.category[0];
}

taxonomy_category GeocodingPreferences::secondTaxonomyCategory() const
{
	return prefs.geocoding.category[1];
}

taxonomy_category GeocodingPreferences::thirdTaxonomyCategory() const
{
	return prefs.geocoding.category[2];
}

void GeocodingPreferences::setEnableGeocoding(bool value)
{
	if (value == prefs.geocoding.enable_geocoding)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("enable_geocoding", value);
	prefs.geocoding.enable_geocoding = value;
	emit enableGeocodingChanged(value);
}

void GeocodingPreferences::setParseDiveWithoutGps(bool value)
{
	if (value == prefs.geocoding.parse_dive_without_gps)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("parse_dives_without_gps", value);
	prefs.geocoding.parse_dive_without_gps = value;
	emit parseDiveWithoutGpsChanged(value);
}

void GeocodingPreferences::setTagExistingDives(bool value)
{
	if (value == prefs.geocoding.tag_existing_dives)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("tag_existing_dives", value);
	prefs.geocoding.tag_existing_dives = value;
	emit tagExistingDivesChanged(value);
}

void GeocodingPreferences::setFirstTaxonomyCategory(taxonomy_category value)
{
	if (value == prefs.geocoding.category[0])
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("cat0", value);
	prefs.geocoding.category[0] = value;
	emit firstTaxonomyCategoryChanged(value);
}

void GeocodingPreferences::setSecondTaxonomyCategory(taxonomy_category value)
{
	if (value == prefs.geocoding.category[1])
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("cat1", value);
	prefs.geocoding.category[1]= value;
	emit secondTaxonomyCategoryChanged(value);
}

void GeocodingPreferences::setThirdTaxonomyCategory(taxonomy_category value)
{
	if (value == prefs.geocoding.category[2])
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("cat2", value);
	prefs.geocoding.category[2] = value;
	emit thirdTaxonomyCategoryChanged(value);
}

ProxySettings::ProxySettings(QObject *parent) :
	QObject(parent),
	group(QStringLiteral("Network"))
{
}

int ProxySettings::type() const
{
	return prefs.proxy_type;
}

QString ProxySettings::host() const
{
	return prefs.proxy_host;
}

int ProxySettings::port() const
{
	return prefs.proxy_port;
}

short ProxySettings::auth() const
{
	return prefs.proxy_auth;
}

QString ProxySettings::user() const
{
	return prefs.proxy_user;
}

QString ProxySettings::pass() const
{
	return prefs.proxy_pass;
}

void ProxySettings::setType(int value)
{
	if (value == prefs.proxy_type)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("proxy_type", value);
	prefs.proxy_type = value;
	emit typeChanged(value);
}

void ProxySettings::setHost(const QString& value)
{
	if (value == prefs.proxy_host)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("proxy_host", value);
	free(prefs.proxy_host);
	prefs.proxy_host = copy_string(qPrintable(value));;
	emit hostChanged(value);
}

void ProxySettings::setPort(int value)
{
	if (value == prefs.proxy_port)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("proxy_port", value);
	prefs.proxy_port = value;
	emit portChanged(value);
}

void ProxySettings::setAuth(short value)
{
	if (value == prefs.proxy_auth)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("proxy_auth", value);
	prefs.proxy_auth = value;
	emit authChanged(value);
}

void ProxySettings::setUser(const QString& value)
{
	if (value == prefs.proxy_user)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("proxy_user", value);
	free(prefs.proxy_user);
	prefs.proxy_user = copy_string(qPrintable(value));
	emit userChanged(value);
}

void ProxySettings::setPass(const QString& value)
{
	if (value == prefs.proxy_pass)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("proxy_pass", value);
	free(prefs.proxy_pass);
	prefs.proxy_pass = copy_string(qPrintable(value));
	emit passChanged(value);
}

CloudStorageSettings::CloudStorageSettings(QObject *parent) :
	QObject(parent),
	group(QStringLiteral("CloudStorage"))
{

}

bool CloudStorageSettings::gitLocalOnly() const
{
	return prefs.git_local_only;
}

QString CloudStorageSettings::password() const
{
	return QString(prefs.cloud_storage_password);
}

QString CloudStorageSettings::newPassword() const
{
	return QString(prefs.cloud_storage_newpassword);
}

QString CloudStorageSettings::email() const
{
	return QString(prefs.cloud_storage_email);
}

QString CloudStorageSettings::emailEncoded() const
{
	return QString(prefs.cloud_storage_email_encoded);
}

bool CloudStorageSettings::savePasswordLocal() const
{
	return prefs.save_password_local;
}

short CloudStorageSettings::verificationStatus() const
{
	return prefs.cloud_verification_status;
}

bool CloudStorageSettings::backgroundSync() const
{
	return prefs.cloud_background_sync;
}

QString CloudStorageSettings::userId() const
{
	return QString(prefs.userid);
}

QString CloudStorageSettings::baseUrl() const
{
	return QString(prefs.cloud_base_url);
}

QString CloudStorageSettings::gitUrl() const
{
	return QString(prefs.cloud_git_url);
}

void CloudStorageSettings::setPassword(const QString& value)
{
	if (value == prefs.cloud_storage_password)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("password", value);
	free(prefs.cloud_storage_password);
	prefs.cloud_storage_password = copy_string(qPrintable(value));
	emit passwordChanged(value);
}

void CloudStorageSettings::setNewPassword(const QString& value)
{
	if (value == prefs.cloud_storage_newpassword)
		return;
	/*TODO: This looks like wrong, but 'new password' is not saved on disk, why it's on prefs? */
	free(prefs.cloud_storage_newpassword);
	prefs.cloud_storage_newpassword = copy_string(qPrintable(value));
	emit newPasswordChanged(value);
}

void CloudStorageSettings::setEmail(const QString& value)
{
	if (value == prefs.cloud_storage_email)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("email", value);
	free(prefs.cloud_storage_email);
	prefs.cloud_storage_email = copy_string(qPrintable(value));
	emit emailChanged(value);
}

void CloudStorageSettings::setUserId(const QString& value)
{
	if (value == prefs.userid)
		return;
	//WARNING: UserId is stored outside of any group, but it belongs to Cloud Storage.
	QSettings s;
	s.setValue("subsurface_webservice_uid", value);
	free(prefs.userid);
	prefs.userid = copy_string(qPrintable(value));
	emit userIdChanged(value);
}

void CloudStorageSettings::setEmailEncoded(const QString& value)
{
	if (value == prefs.cloud_storage_email_encoded)
		return;
	/*TODO: This looks like wrong, but 'email encoded' is not saved on disk, why it's on prefs? */
	free(prefs.cloud_storage_email_encoded);
	prefs.cloud_storage_email_encoded = copy_string(qPrintable(value));
	emit emailEncodedChanged(value);
}

void CloudStorageSettings::setSavePasswordLocal(bool value)
{
	if (value == prefs.save_password_local)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("save_password_local", value);
	prefs.save_password_local = value;
	emit savePasswordLocalChanged(value);
}

void CloudStorageSettings::setVerificationStatus(short value)
{
	if (value == prefs.cloud_verification_status)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("cloud_verification_status", value);
	prefs.cloud_verification_status = value;
	emit verificationStatusChanged(value);
}

void CloudStorageSettings::setBackgroundSync(bool value)
{
	if (value == prefs.cloud_background_sync)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("cloud_background_sync", value);
	prefs.cloud_background_sync = value;
	emit backgroundSyncChanged(value);
}

void CloudStorageSettings::setSaveUserIdLocal(short int value)
{
	//TODO: this is not saved on disk?
	if (value == prefs.save_userid_local)
		return;
	prefs.save_userid_local = value;
	emit saveUserIdLocalChanged(value);
}

short int CloudStorageSettings::saveUserIdLocal() const
{
	return prefs.save_userid_local;
}

void CloudStorageSettings::setBaseUrl(const QString& value)
{
	if (value == prefs.cloud_base_url)
		return;

	// dont free data segment.
	if (prefs.cloud_base_url != default_prefs.cloud_base_url) {
		free((void*)prefs.cloud_base_url);
		free((void*)prefs.cloud_git_url);
	}
	QSettings s;
	s.beginGroup(group);
	s.setValue("cloud_base_url", value);
	prefs.cloud_base_url = copy_string(qPrintable(value));
	prefs.cloud_git_url = copy_string(qPrintable(QString(prefs.cloud_base_url) + "/git"));
}

void CloudStorageSettings::setGitUrl(const QString& value)
{
	Q_UNUSED(value); /* no op */
}

void CloudStorageSettings::setGitLocalOnly(bool value)
{
	if (value == prefs.git_local_only)
		return;
	prefs.git_local_only = value;
	emit gitLocalOnlyChanged(value);
}

DivePlannerSettings::DivePlannerSettings(QObject *parent) :
	QObject(parent),
	group(QStringLiteral("Planner"))
{
}

bool DivePlannerSettings::lastStop() const
{
	return prefs.last_stop;
}

bool DivePlannerSettings::verbatimPlan() const
{
	return prefs.verbatim_plan;
}

bool DivePlannerSettings::displayRuntime() const
{
	return prefs.display_runtime;
}

bool DivePlannerSettings::displayDuration() const
{
	return prefs.display_duration;
}

bool DivePlannerSettings::displayTransitions() const
{
	return prefs.display_transitions;
}

bool DivePlannerSettings::doo2breaks() const
{
	return prefs.doo2breaks;
}

bool DivePlannerSettings::dropStoneMode() const
{
	return prefs.drop_stone_mode;
}

bool DivePlannerSettings::safetyStop() const
{
	return prefs.safetystop;
}

bool DivePlannerSettings::switchAtRequiredStop() const
{
	return prefs.switch_at_req_stop;
}

int DivePlannerSettings::ascrate75() const
{
	return prefs.ascrate75;
}

int DivePlannerSettings::ascrate50() const
{
	return prefs.ascrate50;
}

int DivePlannerSettings::ascratestops() const
{
	return prefs.ascratestops;
}

int DivePlannerSettings::ascratelast6m() const
{
	return prefs.ascratelast6m;
}

int DivePlannerSettings::descrate() const
{
	return prefs.descrate;
}

int DivePlannerSettings::bottompo2() const
{
	return prefs.bottompo2;
}

int DivePlannerSettings::decopo2() const
{
	return prefs.decopo2;
}

int DivePlannerSettings::bestmixend() const
{
	return prefs.bestmixend.mm;
}

int DivePlannerSettings::reserveGas() const
{
	return prefs.reserve_gas;
}

int DivePlannerSettings::minSwitchDuration() const
{
	return prefs.min_switch_duration;
}

int DivePlannerSettings::bottomSac() const
{
	return prefs.bottomsac;
}

int DivePlannerSettings::decoSac() const
{
	return prefs.decosac;
}

short DivePlannerSettings::conservatismLevel() const
{
	return prefs.conservatism_level;
}

deco_mode DivePlannerSettings::decoMode() const
{
	return prefs.deco_mode;
}

void DivePlannerSettings::setLastStop(bool value)
{
	if (value == prefs.last_stop)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("last_stop", value);
	prefs.last_stop = value;
	emit lastStopChanged(value);
}

void DivePlannerSettings::setVerbatimPlan(bool value)
{
	if (value == prefs.verbatim_plan)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("verbatim_plan", value);
	prefs.verbatim_plan = value;
	emit verbatimPlanChanged(value);
}

void DivePlannerSettings::setDisplayRuntime(bool value)
{
	if (value == prefs.display_runtime)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("display_runtime", value);
	prefs.display_runtime = value;
	emit displayRuntimeChanged(value);
}

void DivePlannerSettings::setDisplayDuration(bool value)
{
	if (value == prefs.display_duration)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("display_duration", value);
	prefs.display_duration = value;
	emit displayDurationChanged(value);
}

void DivePlannerSettings::setDisplayTransitions(bool value)
{
	if (value == prefs.display_transitions)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("display_transitions", value);
	prefs.display_transitions = value;
	emit displayTransitionsChanged(value);
}

void DivePlannerSettings::setDoo2breaks(bool value)
{
	if (value == prefs.doo2breaks)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("doo2breaks", value);
	prefs.doo2breaks = value;
	emit doo2breaksChanged(value);
}

void DivePlannerSettings::setDropStoneMode(bool value)
{
	if (value == prefs.drop_stone_mode)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("drop_stone_mode", value);
	prefs.drop_stone_mode = value;
	emit dropStoneModeChanged(value);
}

void DivePlannerSettings::setSafetyStop(bool value)
{
	if (value == prefs.safetystop)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("safetystop", value);
	prefs.safetystop = value;
	emit safetyStopChanged(value);
}

void DivePlannerSettings::setSwitchAtRequiredStop(bool value)
{
	if (value == prefs.switch_at_req_stop)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("switch_at_req_stop", value);
	prefs.switch_at_req_stop = value;
	emit switchAtRequiredStopChanged(value);
}

void DivePlannerSettings::setAscrate75(int value)
{
	if (value == prefs.ascrate75)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("ascrate75", value);
	prefs.ascrate75 = value;
	emit ascrate75Changed(value);
}

void DivePlannerSettings::setAscrate50(int value)
{
	if (value == prefs.ascrate50)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("ascrate50", value);
	prefs.ascrate50 = value;
	emit ascrate50Changed(value);
}

void DivePlannerSettings::setAscratestops(int value)
{
	if (value == prefs.ascratestops)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("ascratestops", value);
	prefs.ascratestops = value;
	emit ascratestopsChanged(value);
}

void DivePlannerSettings::setAscratelast6m(int value)
{
	if (value == prefs.ascratelast6m)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("ascratelast6m", value);
	prefs.ascratelast6m = value;
	emit ascratelast6mChanged(value);
}

void DivePlannerSettings::setDescrate(int value)
{
	if (value == prefs.descrate)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("descrate", value);
	prefs.descrate = value;
	emit descrateChanged(value);
}

void DivePlannerSettings::setBottompo2(int value)
{
	if (value == prefs.bottompo2)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("bottompo2", value);
	prefs.bottompo2 = value;
	emit bottompo2Changed(value);
}

void DivePlannerSettings::setDecopo2(int value)
{
	if (value == prefs.decopo2)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("decopo2", value);
	prefs.decopo2 = value;
	emit decopo2Changed(value);
}

void DivePlannerSettings::setBestmixend(int value)
{
	if (value == prefs.bestmixend.mm)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("bestmixend", value);
	prefs.bestmixend.mm = value;
	emit bestmixendChanged(value);
}

void DivePlannerSettings::setReserveGas(int value)
{
	if (value == prefs.reserve_gas)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("reserve_gas", value);
	prefs.reserve_gas = value;
	emit reserveGasChanged(value);
}

void DivePlannerSettings::setMinSwitchDuration(int value)
{
	if (value == prefs.min_switch_duration)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("min_switch_duration", value);
	prefs.min_switch_duration = value;
	emit minSwitchDurationChanged(value);
}

void DivePlannerSettings::setBottomSac(int value)
{
	if (value == prefs.bottomsac)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("bottomsac", value);
	prefs.bottomsac = value;
	emit bottomSacChanged(value);
}

void DivePlannerSettings::setSecoSac(int value)
{
	if (value == prefs.decosac)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("decosac", value);
	prefs.decosac = value;
	emit decoSacChanged(value);
}

void DivePlannerSettings::setConservatismLevel(int value)
{
	if (value == prefs.conservatism_level)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("conservatism", value);
	prefs.conservatism_level = value;
	emit conservatismLevelChanged(value);
}

void DivePlannerSettings::setDecoMode(deco_mode value)
{
	if (value == prefs.deco_mode)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("deco_mode", value);
	prefs.deco_mode = value;
	emit decoModeChanged(value);
}

UnitsSettings::UnitsSettings(QObject *parent) :
	QObject(parent),
	group(QStringLiteral("Units"))
{

}

int UnitsSettings::length() const
{
	return prefs.units.length;
}

int UnitsSettings::pressure() const
{
	return prefs.units.pressure;
}

int UnitsSettings::volume() const
{
	return prefs.units.volume;
}

int UnitsSettings::temperature() const
{
	return prefs.units.temperature;
}

int UnitsSettings::weight() const
{
	return prefs.units.weight;
}

int UnitsSettings::verticalSpeedTime() const
{
	return prefs.units.vertical_speed_time;
}

QString UnitsSettings::unitSystem() const
{
	return QString(); /*FIXME: there's no char * units on the prefs. */
}

bool UnitsSettings::coordinatesTraditional() const
{
	return prefs.coordinates_traditional;
}

void UnitsSettings::setLength(int value)
{
	if (value == prefs.units.length)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("length", value);
	prefs.units.length = (units::LENGHT) value;
	emit lengthChanged(value);
}

void UnitsSettings::setPressure(int value)
{
	if (value == prefs.units.pressure)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("pressure", value);
	prefs.units.pressure = (units::PRESSURE) value;
	emit pressureChanged(value);
}

void UnitsSettings::setVolume(int value)
{
	if (value == prefs.units.volume)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("volume", value);
	prefs.units.volume = (units::VOLUME) value;
	emit volumeChanged(value);
}

void UnitsSettings::setTemperature(int value)
{
	if (value == prefs.units.temperature)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("temperature", value);
	prefs.units.temperature = (units::TEMPERATURE) value;
	emit temperatureChanged(value);
}

void UnitsSettings::setWeight(int value)
{
	if (value == prefs.units.weight)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("weight", value);
	prefs.units.weight = (units::WEIGHT) value;
	emit weightChanged(value);
}

void UnitsSettings::setVerticalSpeedTime(int value)
{
	if (value == prefs.units.vertical_speed_time)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("vertical_speed_time", value);
	prefs.units.vertical_speed_time = (units::TIME) value;
	emit verticalSpeedTimeChanged(value);
}

void UnitsSettings::setCoordinatesTraditional(bool value)
{
	if (value == prefs.coordinates_traditional)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("coordinates", value);
	prefs.coordinates_traditional = value;
	emit coordinatesTraditionalChanged(value);
}

void UnitsSettings::setUnitSystem(const QString& value)
{
	short int v = value == QStringLiteral("metric") ? METRIC
	              : value == QStringLiteral("imperial")? IMPERIAL
	              : PERSONALIZE;

	if (v == prefs.unit_system)
		return;

	QSettings s;

	s.setValue("unit_system", value);

	if (value == QStringLiteral("metric")) {
		prefs.unit_system = METRIC;
		prefs.units = SI_units;
	} else if (value == QStringLiteral("imperial")) {
		prefs.unit_system = IMPERIAL;
		prefs.units = IMPERIAL_units;
	} else {
		prefs.unit_system = PERSONALIZE;
	}

	emit unitSystemChanged(value);
	// TODO: emit the other values here?
}

GeneralSettingsObjectWrapper::GeneralSettingsObjectWrapper(QObject *parent) :
	QObject(parent),
	group(QStringLiteral("GeneralSettings"))
{
}

QString GeneralSettingsObjectWrapper::defaultFilename() const
{
	return prefs.default_filename;
}

QString GeneralSettingsObjectWrapper::defaultCylinder() const
{
	return prefs.default_cylinder;
}

short GeneralSettingsObjectWrapper::defaultFileBehavior() const
{
	return prefs.default_file_behavior;
}

bool GeneralSettingsObjectWrapper::useDefaultFile() const
{
	return prefs.use_default_file;
}

int GeneralSettingsObjectWrapper::defaultSetPoint() const
{
	return prefs.defaultsetpoint;
}

int GeneralSettingsObjectWrapper::o2Consumption() const
{
	return prefs.o2consumption;
}

int GeneralSettingsObjectWrapper::pscrRatio() const
{
	return prefs.pscr_ratio;
}

void GeneralSettingsObjectWrapper::setDefaultFilename(const QString& value)
{
	if (value == prefs.default_filename)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("default_filename", value);
	prefs.default_filename = copy_string(qPrintable(value));
	emit defaultFilenameChanged(value);
}

void GeneralSettingsObjectWrapper::setDefaultCylinder(const QString& value)
{
	if (value == prefs.default_cylinder)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("default_cylinder", value);
	prefs.default_cylinder = copy_string(qPrintable(value));
	emit defaultCylinderChanged(value);
}

void GeneralSettingsObjectWrapper::setDefaultFileBehavior(short value)
{
	if (value == prefs.default_file_behavior && prefs.default_file_behavior != UNDEFINED_DEFAULT_FILE)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("default_file_behavior", value);
	prefs.default_file_behavior = value;
	if (prefs.default_file_behavior == UNDEFINED_DEFAULT_FILE) {
		// undefined, so check if there's a filename set and
		// use that, otherwise go with no default file
		if (QString(prefs.default_filename).isEmpty())
			prefs.default_file_behavior = NO_DEFAULT_FILE;
		else
			prefs.default_file_behavior = LOCAL_DEFAULT_FILE;
	}
	emit defaultFileBehaviorChanged(value);
}

void GeneralSettingsObjectWrapper::setUseDefaultFile(bool value)
{
	if (value == prefs.use_default_file)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("use_default_file", value);
	prefs.use_default_file = value;
	emit useDefaultFileChanged(value);
}

void GeneralSettingsObjectWrapper::setDefaultSetPoint(int value)
{
	if (value == prefs.defaultsetpoint)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("defaultsetpoint", value);
	prefs.defaultsetpoint = value;
	emit defaultSetPointChanged(value);
}

void GeneralSettingsObjectWrapper::setO2Consumption(int value)
{
	if (value == prefs.o2consumption)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("o2consumption", value);
	prefs.o2consumption = value;
	emit o2ConsumptionChanged(value);
}

void GeneralSettingsObjectWrapper::setPscrRatio(int value)
{
	if (value == prefs.pscr_ratio)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("pscr_ratio", value);
	prefs.pscr_ratio = value;
	emit pscrRatioChanged(value);
}

DisplaySettingsObjectWrapper::DisplaySettingsObjectWrapper(QObject *parent) :
	QObject(parent),
	group(QStringLiteral("Display"))
{
}

QString DisplaySettingsObjectWrapper::divelistFont() const
{
	return prefs.divelist_font;
}

double DisplaySettingsObjectWrapper::fontSize() const
{
	return prefs.font_size;
}

short DisplaySettingsObjectWrapper::displayInvalidDives() const
{
	return prefs.display_invalid_dives;
}

void DisplaySettingsObjectWrapper::setDivelistFont(const QString& value)
{

	QString newValue = value;
	if (value.contains(","))
		newValue = value.left(value.indexOf(","));

	if (newValue == prefs.divelist_font)
		return;

	QSettings s;
	s.setValue("divelist_font", value);

	if (!subsurface_ignore_font(newValue.toUtf8().constData())) {
		free((void *)prefs.divelist_font);
		prefs.divelist_font = strdup(newValue.toUtf8().constData());
		qApp->setFont(QFont(newValue));
	}
	emit divelistFontChanged(newValue);
}

void DisplaySettingsObjectWrapper::setFontSize(double value)
{
	if (value == prefs.font_size)
		return;

	QSettings s;
	s.setValue("font_size", value);
	prefs.font_size = value;
	QFont defaultFont = qApp->font();
	defaultFont.setPointSizeF(prefs.font_size);
	qApp->setFont(defaultFont);
	emit fontSizeChanged(value);
}

void DisplaySettingsObjectWrapper::setDisplayInvalidDives(short value)
{
	if (value == prefs.display_invalid_dives)
		return;

	QSettings s;
	s.setValue("displayinvalid", value);
	prefs.display_invalid_dives = value;
	emit displayInvalidDivesChanged(value);
}

LanguageSettingsObjectWrapper::LanguageSettingsObjectWrapper(QObject *parent) :
	QObject(parent),
	group("Language")
{
}

QString LanguageSettingsObjectWrapper::language() const
{
	return prefs.locale.language;
}

QString LanguageSettingsObjectWrapper::timeFormat() const
{
	return prefs.time_format;
}

QString LanguageSettingsObjectWrapper::dateFormat() const
{
	return prefs.date_format;
}

QString LanguageSettingsObjectWrapper::dateFormatShort() const
{
	return prefs.date_format_short;
}

bool LanguageSettingsObjectWrapper::timeFormatOverride() const
{
	return prefs.time_format_override;
}

bool LanguageSettingsObjectWrapper::dateFormatOverride() const
{
	return prefs.date_format_override;
}

bool LanguageSettingsObjectWrapper::useSystemLanguage() const
{
	return prefs.locale.use_system_language;
}

void LanguageSettingsObjectWrapper::setUseSystemLanguage(bool value)
{
	if (value == prefs.locale.use_system_language)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("UseSystemLanguage", value);
	prefs.locale.use_system_language = copy_string(qPrintable(value));
	emit useSystemLanguageChanged(value);
}

void  LanguageSettingsObjectWrapper::setLanguage(const QString& value)
{
	if (value == prefs.locale.language)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("UiLanguage", value);
	prefs.locale.language = copy_string(qPrintable(value));
	emit languageChanged(value);
}

void  LanguageSettingsObjectWrapper::setTimeFormat(const QString& value)
{
	if (value == prefs.time_format)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("time_format", value);
	prefs.time_format = copy_string(qPrintable(value));;
	emit timeFormatChanged(value);
}

void  LanguageSettingsObjectWrapper::setDateFormat(const QString& value)
{
	if (value == prefs.date_format)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("date_format", value);
	prefs.date_format = copy_string(qPrintable(value));;
	emit dateFormatChanged(value);
}

void  LanguageSettingsObjectWrapper::setDateFormatShort(const QString& value)
{
	if (value == prefs.date_format_short)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("date_format_short", value);
	prefs.date_format_short = copy_string(qPrintable(value));;
	emit dateFormatShortChanged(value);
}

void  LanguageSettingsObjectWrapper::setTimeFormatOverride(bool value)
{
	if (value == prefs.time_format_override)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("time_format_override", value);
	prefs.time_format_override = value;
	emit timeFormatOverrideChanged(value);
}

void  LanguageSettingsObjectWrapper::setDateFormatOverride(bool value)
{
	if (value == prefs.date_format_override)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("date_format_override", value);
	prefs.date_format_override = value;
	emit dateFormatOverrideChanged(value);
}

AnimationsSettingsObjectWrapper::AnimationsSettingsObjectWrapper(QObject* parent):
	QObject(parent),
	group("Animations")

{
}

int AnimationsSettingsObjectWrapper::animationSpeed() const
{
	return prefs.animation_speed;
}

void AnimationsSettingsObjectWrapper::setAnimationSpeed(int value)
{
	if (value == prefs.animation_speed)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("animation_speed", value);
	prefs.animation_speed = value;
	emit animationSpeedChanged(value);
}

LocationServiceSettingsObjectWrapper::LocationServiceSettingsObjectWrapper(QObject* parent):
	QObject(parent),
	group("locationService")
{
}

int LocationServiceSettingsObjectWrapper::distanceThreshold() const
{
	return prefs.distance_threshold;
}

int LocationServiceSettingsObjectWrapper::timeThreshold() const
{
	return prefs.time_threshold;
}

void LocationServiceSettingsObjectWrapper::setDistanceThreshold(int value)
{
	if (value == prefs.distance_threshold)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("distance_threshold", value);
	prefs.distance_threshold = value;
	emit distanceThresholdChanged(value);
}

void LocationServiceSettingsObjectWrapper::setTimeThreshold(int value)
{
	if (value == prefs.time_threshold)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("time_threshold", value);
	prefs.time_threshold = value;
	emit timeThresholdChanged(value);
}

SettingsObjectWrapper::SettingsObjectWrapper(QObject* parent):
QObject(parent),
	techDetails(new TechnicalDetailsSettings(this)),
	pp_gas(new PartialPressureGasSettings(this)),
	facebook(new FacebookSettings(this)),
	geocoding(new GeocodingPreferences(this)),
	proxy(new ProxySettings(this)),
	cloud_storage(new CloudStorageSettings(this)),
	planner_settings(new DivePlannerSettings(this)),
	unit_settings(new UnitsSettings(this)),
	general_settings(new GeneralSettingsObjectWrapper(this)),
	display_settings(new DisplaySettingsObjectWrapper(this)),
	language_settings(new LanguageSettingsObjectWrapper(this)),
	animation_settings(new AnimationsSettingsObjectWrapper(this)),
	location_settings(new LocationServiceSettingsObjectWrapper(this)),
	update_manager_settings(new UpdateManagerSettings(this)),
	dive_computer_settings(new DiveComputerSettings(this))
{
}

void SettingsObjectWrapper::load()
{
	QSettings s;
	QVariant v;

	uiLanguage(NULL);
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
	GET_BOOL("RulerBar", rulergraph);
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
	GET_BOOL("show_pictures_in_profile", show_pictures_in_profile);
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
#ifndef SUBSURFACE_MOBILE
	GET_BOOL("save_password_local", save_password_local);
#else
	// always save the password in Subsurface-mobile
	prefs.save_password_local = true;
#endif
	if (prefs.save_password_local) { // GET_TEXT macro is not a single statement
		GET_TXT("password", cloud_storage_password);
	}
	GET_INT("cloud_verification_status", cloud_verification_status);
	GET_BOOL("cloud_background_sync", cloud_background_sync);
	GET_BOOL("git_local_only", git_local_only);

	// creating the git url here is simply a convenience when C code wants
	// to compare against that git URL - it's always derived from the base URL
	GET_TXT("cloud_base_url", cloud_base_url);
	prefs.cloud_git_url = strdup(qPrintable(QString(prefs.cloud_base_url) + "/git"));
	s.endGroup();

	// Subsurface webservice id is stored outside of the groups
	GET_TXT("subsurface_webservice_uid", userid);

	// but the related time / distance threshold (only used in the mobile app)
	// are in their own group
	s.beginGroup("locationService");
	GET_INT("distance_threshold", distance_threshold);
	GET_INT("time_threshold", time_threshold);
	s.endGroup();

	// GeoManagement
	s.beginGroup("geocoding");
#ifdef DISABLED
	GET_BOOL("enable_geocoding", geocoding.enable_geocoding);
	GET_BOOL("parse_dive_without_gps", geocoding.parse_dive_without_gps);
	GET_BOOL("tag_existing_dives", geocoding.tag_existing_dives);
#else
	prefs.geocoding.enable_geocoding = true;
#endif
	GET_ENUM("cat0", taxonomy_category, geocoding.category[0]);
	GET_ENUM("cat1", taxonomy_category, geocoding.category[1]);
	GET_ENUM("cat2", taxonomy_category, geocoding.category[2]);
	s.endGroup();

	// GPS service time and distance thresholds
	s.beginGroup("LocationService");
	GET_INT("time_threshold", time_threshold);
	GET_INT("distance_threshold", distance_threshold);
	s.endGroup();

	s.beginGroup("Planner");
	prefs.last_stop = s.value("last_stop", prefs.last_stop).toBool();
	prefs.verbatim_plan = s.value("verbatim_plan", prefs.verbatim_plan).toBool();
	prefs.display_duration = s.value("display_duration", prefs.display_duration).toBool();
	prefs.display_runtime = s.value("display_runtime", prefs.display_runtime).toBool();
	prefs.display_transitions = s.value("display_transitions", prefs.display_transitions).toBool();
	prefs.deco_mode = deco_mode(s.value("deco_mode", prefs.deco_mode).toInt());
	prefs.safetystop = s.value("safetystop", prefs.safetystop).toBool();
	prefs.reserve_gas = s.value("reserve_gas", prefs.reserve_gas).toInt();
	prefs.ascrate75 = s.value("ascrate75", prefs.ascrate75).toInt();
	prefs.ascrate50 = s.value("ascrate50", prefs.ascrate50).toInt();
	prefs.ascratestops = s.value("ascratestops", prefs.ascratestops).toInt();
	prefs.ascratelast6m = s.value("ascratelast6m", prefs.ascratelast6m).toInt();
	prefs.descrate = s.value("descrate", prefs.descrate).toInt();
	prefs.bottompo2 = s.value("bottompo2", prefs.bottompo2).toInt();
	prefs.decopo2 = s.value("decopo2", prefs.decopo2).toInt();
	prefs.bestmixend.mm = s.value("bestmixend", prefs.bestmixend.mm).toInt();
	prefs.doo2breaks = s.value("doo2breaks", prefs.doo2breaks).toBool();
	prefs.switch_at_req_stop = s.value("switch_at_req_stop", prefs.switch_at_req_stop).toBool();
	prefs.min_switch_duration = s.value("min_switch_duration", prefs.min_switch_duration).toInt();
	prefs.drop_stone_mode = s.value("drop_stone_mode", prefs.drop_stone_mode).toBool();
	prefs.bottomsac = s.value("bottomsac", prefs.bottomsac).toInt();
	prefs.decosac = s.value("decosac", prefs.decosac).toInt();
	prefs.conservatism_level = s.value("conservatism", prefs.conservatism_level).toInt();
	s.endGroup();

	s.beginGroup("UpdateManager");
	prefs.update_manager.dont_check_exists = s.contains("DontCheckForUpdates");
	prefs.update_manager.dont_check_for_updates = s.value("DontCheckForUpdates").toBool();
	prefs.update_manager.last_version_used = copy_string(qPrintable(s.value("LastVersionUsed").toString()));
	prefs.update_manager.next_check = copy_string(qPrintable(s.value("NextCheck").toString()));
	s.endGroup();
}

void SettingsObjectWrapper::sync()
{
	QSettings s;
	s.beginGroup("Planner");
	s.setValue("last_stop", prefs.last_stop);
	s.setValue("verbatim_plan", prefs.verbatim_plan);
	s.setValue("display_duration", prefs.display_duration);
	s.setValue("display_runtime", prefs.display_runtime);
	s.setValue("display_transitions", prefs.display_transitions);
	s.setValue("safetystop", prefs.safetystop);
	s.setValue("reserve_gas", prefs.reserve_gas);
	s.setValue("ascrate75", prefs.ascrate75);
	s.setValue("ascrate50", prefs.ascrate50);
	s.setValue("ascratestops", prefs.ascratestops);
	s.setValue("ascratelast6m", prefs.ascratelast6m);
	s.setValue("descrate", prefs.descrate);
	s.setValue("bottompo2", prefs.bottompo2);
	s.setValue("decopo2", prefs.decopo2);
	s.setValue("bestmixend", prefs.bestmixend.mm);
	s.setValue("doo2breaks", prefs.doo2breaks);
	s.setValue("drop_stone_mode", prefs.drop_stone_mode);
	s.setValue("switch_at_req_stop", prefs.switch_at_req_stop);
	s.setValue("min_switch_duration", prefs.min_switch_duration);
	s.setValue("bottomsac", prefs.bottomsac);
	s.setValue("decosac", prefs.decosac);
	s.setValue("deco_mode", int(prefs.deco_mode));
	s.setValue("conservatism", prefs.conservatism_level);
	s.endGroup();
}

SettingsObjectWrapper* SettingsObjectWrapper::instance()
{
	static SettingsObjectWrapper settings;
	return &settings;
}
