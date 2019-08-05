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

void reverseGeoLookup(degrees_t latitude, degrees_t longitude, taxonomy_data *taxonomy)
{
	// By making the QNetworkAccessManager static and local to this function,
	// only one manager exists for all geo-lookups and it is only initialized
	// on first call to this function.
	static QNetworkAccessManager rgl;
	QNetworkRequest request;
	QEventLoop loop;
	QString geonamesURL("http://api.geonames.org/findNearbyPlaceNameJSON?lang=%1&lat=%2&lng=%3&radius=50&username=dirkhh");
	QString geonamesOceanURL("http://api.geonames.org/oceanJSON?lang=%1&lat=%2&lng=%3&radius=50&username=dirkhh");
	QTimer timer;

	request.setRawHeader("Accept", "text/json");
	request.setRawHeader("User-Agent", getUserAgent().toUtf8());
	QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

	// first check the findNearbyPlaces API from geonames - that should give us country, state, city
	request.setUrl(geonamesURL.arg(uiLanguage(NULL).section(QRegExp("[-_ ]"), 0, 0)).arg(latitude.udeg / 1000000.0).arg(longitude.udeg / 1000000.0));

	// By using a std::unique_ptr<>, we can exit from the function at any point
	// and the reply will be freed. Likewise, when overwriting the pointer with
	// a new value. According to Qt's documentation it is fine the delete the
	// reply as long as we're not in a slot connected to error() or finish().
	std::unique_ptr<QNetworkReply> reply(rgl.get(request));
	timer.setSingleShot(true);
	QObject::connect(&*reply, SIGNAL(finished()), &loop, SLOT(quit()));
	timer.start(5000);   // 5 secs. timeout
	loop.exec();

	if (timer.isActive()) {
		timer.stop();
		if (reply->error() > 0) {
			report_error("got error accessing geonames.org: %s", qPrintable(reply->errorString()));
			return;
		}
		int v = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		if (v < 200 || v >= 300)
			return;
		QByteArray fullReply = reply->readAll();
		QJsonParseError errorObject;
		QJsonDocument jsonDoc = QJsonDocument::fromJson(fullReply, &errorObject);
		if (errorObject.error != QJsonParseError::NoError) {
			report_error("error parsing geonames.org response: %s", qPrintable(errorObject.errorString()));
			return;
		}
		QJsonObject obj = jsonDoc.object();
		QVariant geoNamesObject = obj.value("geonames").toVariant();
		QVariantList geoNames = geoNamesObject.toList();
		if (geoNames.count() > 0) {
			QVariantMap firstData = geoNames.at(0).toMap();
			int ri = 0, l3 = -1, lt = -1;
			if (taxonomy->category == NULL) {
				taxonomy->category = alloc_taxonomy();
			} else {
				// clear out the data (except for the ocean data)
				int ocean;
				if ((ocean = taxonomy_index_for_category(taxonomy, TC_OCEAN)) > 0) {
					taxonomy->category[0] = taxonomy->category[ocean];
					taxonomy->nr = 1;
				} else {
					// ocean is -1 if there is no such entry, and we didn't copy above
					// if ocean is 0, so the following gets us the correct count
					taxonomy->nr = ocean + 1;
				}
			}
			// get all the data - OCEAN is special, so start at COUNTRY
			for (int j = TC_COUNTRY; j < TC_NR_CATEGORIES; j++) {
				if (firstData[taxonomy_api_names[j]].isValid()) {
					taxonomy->category[ri].category = j;
					taxonomy->category[ri].origin = taxonomy_origin::GEOCODED;
					free((void *)taxonomy->category[ri].value);
					taxonomy->category[ri].value = copy_qstring(firstData[taxonomy_api_names[j]].toString());
					ri++;
				}
			}
			taxonomy->nr = ri;
			l3 = taxonomy_index_for_category(taxonomy, TC_ADMIN_L3);
			lt = taxonomy_index_for_category(taxonomy, TC_LOCALNAME);
			if (l3 == -1 && lt != -1) {
				// basically this means we did get a local name (what we call town), but just like most places
				// we didn't get an adminName_3 - which in some regions is the actual city that town belongs to,
				// then we copy the town into the city
				taxonomy->category[ri].value = copy_string(taxonomy->category[lt].value);
				taxonomy->category[ri].origin = taxonomy_origin::GEOCOPIED;
				taxonomy->category[ri].category = TC_ADMIN_L3;
				taxonomy->nr++;
			}
		} else {
			report_error("geonames.org did not provide reverse lookup information");
			qDebug() << "no reverse geo lookup; geonames returned\n" << fullReply;
		}
	} else {
		report_error("timeout accessing geonames.org");
		QObject::disconnect(&*reply, SIGNAL(finished()), &loop, SLOT(quit()));
		reply->abort();
	}
	// next check the oceans API to figure out the body of water
	request.setUrl(geonamesOceanURL.arg(uiLanguage(NULL).section(QRegExp("[-_ ]"), 0, 0)).arg(latitude.udeg / 1000000.0).arg(longitude.udeg / 1000000.0));
	reply.reset(rgl.get(request)); // Note: frees old reply.
	QObject::connect(&*reply, SIGNAL(finished()), &loop, SLOT(quit()));
	timer.start(5000);   // 5 secs. timeout
	loop.exec();
	if (timer.isActive()) {
		timer.stop();
		if (reply->error() > 0) {
			report_error("got error accessing oceans API of geonames.org: %s", qPrintable(reply->errorString()));
			return;
		}
		int v = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		if (v < 200 || v >= 300)
			return;
		QByteArray fullReply = reply->readAll();
		QJsonParseError errorObject;
		QJsonDocument jsonDoc = QJsonDocument::fromJson(fullReply, &errorObject);
		if (errorObject.error != QJsonParseError::NoError) {
			report_error("error parsing geonames.org response: %s", qPrintable(errorObject.errorString()));
			return;
		}
		QJsonObject obj = jsonDoc.object();
		QVariant oceanObject = obj.value("ocean").toVariant();
		QVariantMap oceanName = oceanObject.toMap();
		if (oceanName["name"].isValid()) {
			int idx;
			if (taxonomy->category == NULL)
				taxonomy->category = alloc_taxonomy();
			idx = taxonomy_index_for_category(taxonomy, TC_OCEAN);
			if (idx == -1)
				idx = taxonomy->nr;
			if (idx < TC_NR_CATEGORIES) {
				taxonomy->category[idx].category = TC_OCEAN;
				taxonomy->category[idx].origin = taxonomy_origin::GEOCODED;
				taxonomy->category[idx].value = copy_qstring(oceanName["name"].toString());
				if (idx == taxonomy->nr)
					taxonomy->nr++;
			}
		}
	} else {
		report_error("timeout accessing geonames.org");
		QObject::disconnect(&*reply, SIGNAL(finished()), &loop, SLOT(quit()));
		reply->abort();
	}
}
