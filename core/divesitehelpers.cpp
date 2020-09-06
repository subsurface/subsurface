// SPDX-License-Identifier: GPL-2.0
//
// infrastructure to deal with dive sites
//

#include "divesitehelpers.h"

#include "divesite.h"
#include "errorhelper.h"
#include "subsurface-string.h"
#include "qthelper.h"
#include "membuffer.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>
#include <memory>

/** Performs a REST get request to a service returning a JSON object. */
static QJsonObject doAsyncRESTGetRequest(const QString& url, int msTimeout)
{
	// By making the QNetworkAccessManager static and local to this function,
	// only one manager exists for all geo-lookups and it is only initialized
	// on first call to this function.
	static QNetworkAccessManager rgl;
	QNetworkRequest request;
	QEventLoop loop;
	QTimer timer;

	request.setRawHeader("Accept", "text/json");
	request.setRawHeader("User-Agent", getUserAgent().toUtf8());
	QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

	request.setUrl(url);

	// By using a std::unique_ptr<>, we can exit from the function at any point
	// and the reply will be freed. Likewise, when overwriting the pointer with
	// a new value. According to Qt's documentation it is fine the delete the
	// reply as long as we're not in a slot connected to error() or finish().
	std::unique_ptr<QNetworkReply> reply(rgl.get(request));
	timer.setSingleShot(true);
	QObject::connect(&*reply, SIGNAL(finished()), &loop, SLOT(quit()));
	timer.start(msTimeout);
	loop.exec();

	if (!timer.isActive()) {
		report_error("timeout accessing %s", qPrintable(url));
		QObject::disconnect(&*reply, SIGNAL(finished()), &loop, SLOT(quit()));
		reply->abort();
		return QJsonObject{};
	}

	timer.stop();
	if (reply->error() > 0) {
		report_error("got error accessing %s: %s", qPrintable(url), qPrintable(reply->errorString()));
		return QJsonObject{};
	}
	int v = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (v < 200 || v >= 300) {
		return QJsonObject{};
	}
	QByteArray fullReply = reply->readAll();
	QJsonParseError errorObject;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(fullReply, &errorObject);
	if (errorObject.error != QJsonParseError::NoError) {
		report_error("error parsing JSON response: %s", qPrintable(errorObject.errorString()));
		return QJsonObject{};
	}
	// Success, return JSON response from server
	return jsonDoc.object();
}

/// Performs a reverse-geo-lookup of the coordinates and returns the taxonomy data.
taxonomy_data reverseGeoLookup(degrees_t latitude, degrees_t longitude)
{
	const QString geonamesNearbyURL = QStringLiteral("http://api.geonames.org/findNearbyJSON?lang=%1&lat=%2&lng=%3&radius=50&username=dirkhh");
	const QString geonamesNearbyPlaceNameURL = QStringLiteral("http://api.geonames.org/findNearbyPlaceNameJSON?lang=%1&lat=%2&lng=%3&radius=50&username=dirkhh");
	const QString geonamesOceanURL = QStringLiteral("http://api.geonames.org/oceanJSON?lang=%1&lat=%2&lng=%3&radius=50&username=dirkhh");

	QString url;
	QJsonObject obj;
	taxonomy_data taxonomy = { 0, 0 };

	// check the oceans API to figure out the body of water
	url = geonamesOceanURL.arg(getUiLanguage().section(QRegExp("[-_ ]"), 0, 0)).arg(latitude.udeg / 1000000.0).arg(longitude.udeg / 1000000.0);
	obj = doAsyncRESTGetRequest(url, 5000); // 5 secs. timeout
	QVariantMap oceanName = obj.value("ocean").toVariant().toMap();
	if (oceanName["name"].isValid())
		taxonomy_set_category(&taxonomy, TC_OCEAN, qPrintable(oceanName["name"].toString()), taxonomy_origin::GEOCODED);

	// check the findNearbyPlaces API from geonames - that should give us country, state, city
	url = geonamesNearbyPlaceNameURL.arg(getUiLanguage().section(QRegExp("[-_ ]"), 0, 0)).arg(latitude.udeg / 1000000.0).arg(longitude.udeg / 1000000.0);
	obj = doAsyncRESTGetRequest(url, 5000); // 5 secs. timeout
	QVariantList geoNames = obj.value("geonames").toVariant().toList();
	if (geoNames.count() == 0) {
		// check the findNearby API from geonames if the previous search came up empty - that should give us country, state, location
		url = geonamesNearbyURL.arg(getUiLanguage().section(QRegExp("[-_ ]"), 0, 0)).arg(latitude.udeg / 1000000.0).arg(longitude.udeg / 1000000.0);
		obj = doAsyncRESTGetRequest(url, 5000); // 5 secs. timeout
		geoNames = obj.value("geonames").toVariant().toList();
	}
	if (geoNames.count() > 0) {
		QVariantMap firstData = geoNames.at(0).toMap();

		// fill out all the data - start at COUNTRY since we already got OCEAN above
		for (int idx = TC_COUNTRY; idx < TC_NR_CATEGORIES; idx++) {
			if (firstData[taxonomy_api_names[idx]].isValid()) {
				QString value = firstData[taxonomy_api_names[idx]].toString();
				taxonomy_set_category(&taxonomy, (taxonomy_category)idx, qPrintable(value), taxonomy_origin::GEOCODED);
			}
		}
		const char *l3 = taxonomy_get_value(&taxonomy, TC_ADMIN_L3);
		const char *lt = taxonomy_get_value(&taxonomy, TC_LOCALNAME);
		if (empty_string(l3) && !empty_string(lt)) {
			// basically this means we did get a local name (what we call town), but just like most places
			// we didn't get an adminName_3 - which in some regions is the actual city that town belongs to,
			// then we copy the town into the city
			taxonomy_set_category(&taxonomy, TC_ADMIN_L3, lt, taxonomy_origin::GEOCOPIED);
		}
	} else {
		report_error("geonames.org did not provide reverse lookup information");
		//qDebug() << "no reverse geo lookup; geonames returned\n" << fullReply;
	}

	return taxonomy;
}
