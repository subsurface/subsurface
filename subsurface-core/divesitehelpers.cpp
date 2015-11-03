//
// infrastructure to deal with dive sites
//

#include "divesitehelpers.h"

#include "divesite.h"
#include "helpers.h"
#include "membuffer.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>

struct GeoLookupInfo {
	degrees_t lat;
	degrees_t lon;
	uint32_t uuid;
};

QVector<GeoLookupInfo> geo_lookup_data;

ReverseGeoLookupThread* ReverseGeoLookupThread::instance() {
	static ReverseGeoLookupThread* self = new ReverseGeoLookupThread();
	return self;
}

ReverseGeoLookupThread::ReverseGeoLookupThread(QObject *obj) : QThread(obj)
{
}

void ReverseGeoLookupThread::run() {
	if (geo_lookup_data.isEmpty())
		return;

	QNetworkRequest request;
	QNetworkAccessManager *rgl = new QNetworkAccessManager();
	QEventLoop loop;
	QString mapquestURL("http://open.mapquestapi.com/nominatim/v1/reverse.php?format=json&accept-language=%1&lat=%2&lon=%3");
	QString geonamesURL("http://api.geonames.org/findNearbyPlaceNameJSON?language=%1&lat=%2&lng=%3&radius=50&username=dirkhh");
	QString geonamesOceanURL("http://api.geonames.org/oceanJSON?language=%1&lat=%2&lng=%3&radius=50&username=dirkhh");
	QString divelogsURL("https://www.divelogs.de/mapsearch_divespotnames.php?lat=%1&lng=%2&radius=50");
	QTimer timer;

	request.setRawHeader("Accept", "text/json");
	request.setRawHeader("User-Agent", getUserAgent().toUtf8());
	connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

	Q_FOREACH (const GeoLookupInfo& info, geo_lookup_data ) {
		struct dive_site *ds = info.uuid ? get_dive_site_by_uuid(info.uuid) : &displayed_dive_site;

		// first check the findNearbyPlaces API from geonames - that should give us country, state, city
		request.setUrl(geonamesURL.arg(uiLanguage(NULL)).arg(info.lat.udeg / 1000000.0).arg(info.lon.udeg / 1000000.0));

		QNetworkReply *reply = rgl->get(request);
		timer.setSingleShot(true);
		connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
		timer.start(5000);   // 5 secs. timeout
		loop.exec();

		if(timer.isActive()) {
			timer.stop();
			if(reply->error() > 0) {
				report_error("got error accessing geonames.org: %s", qPrintable(reply->errorString()));
				goto clear_reply;
			}
			int v = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
			if (v < 200 || v >= 300)
				goto clear_reply;
			QByteArray fullReply = reply->readAll();
			QJsonParseError errorObject;
			QJsonDocument jsonDoc = QJsonDocument::fromJson(fullReply, &errorObject);
			if (errorObject.error != QJsonParseError::NoError) {
				report_error("error parsing geonames.org response: %s", qPrintable(errorObject.errorString()));
				goto clear_reply;
			}
			QJsonObject obj = jsonDoc.object();
			QVariant geoNamesObject = obj.value("geonames").toVariant();
			QVariantList geoNames = geoNamesObject.toList();
			if (geoNames.count() > 0) {
				QVariantMap firstData = geoNames.at(0).toMap();
				int ri = 0, l3 = -1, lt = -1;
				if (ds->taxonomy.category == NULL) {
					ds->taxonomy.category = alloc_taxonomy();
				} else {
					// clear out the data (except for the ocean data)
					int ocean;
					if ((ocean = taxonomy_index_for_category(&ds->taxonomy, TC_OCEAN)) > 0) {
						ds->taxonomy.category[0] = ds->taxonomy.category[ocean];
						ds->taxonomy.nr = 1;
					} else {
						// ocean is -1 if there is no such entry, and we didn't copy above
						// if ocean is 0, so the following gets us the correct count
						ds->taxonomy.nr = ocean + 1;
					}
				}
				// get all the data - OCEAN is special, so start at COUNTRY
				for (int j = TC_COUNTRY; j < TC_NR_CATEGORIES; j++) {
					if (firstData[taxonomy_api_names[j]].isValid()) {
						ds->taxonomy.category[ri].category = j;
						ds->taxonomy.category[ri].origin = taxonomy::GEOCODED;
						free((void*)ds->taxonomy.category[ri].value);
						ds->taxonomy.category[ri].value = copy_string(qPrintable(firstData[taxonomy_api_names[j]].toString()));
						ri++;
					}
				}
				ds->taxonomy.nr = ri;
				l3 = taxonomy_index_for_category(&ds->taxonomy, TC_ADMIN_L3);
				lt = taxonomy_index_for_category(&ds->taxonomy, TC_LOCALNAME);
				if (l3 == -1 && lt != -1) {
					// basically this means we did get a local name (what we call town), but just like most places
					// we didn't get an adminName_3 - which in some regions is the actual city that town belongs to,
					// then we copy the town into the city
					ds->taxonomy.category[ri].value = copy_string(ds->taxonomy.category[lt].value);
					ds->taxonomy.category[ri].origin = taxonomy::COPIED;
					ds->taxonomy.category[ri].category = TC_ADMIN_L3;
					ds->taxonomy.nr++;
				}
				mark_divelist_changed(true);
			} else {
				report_error("geonames.org did not provide reverse lookup information");
				qDebug() << "no reverse geo lookup; geonames returned\n" << fullReply;
			}
		} else {
			report_error("timeout accessing geonames.org");
			disconnect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
			reply->abort();
		}
		// next check the oceans API to figure out the body of water
		request.setUrl(geonamesOceanURL.arg(uiLanguage(NULL)).arg(info.lat.udeg / 1000000.0).arg(info.lon.udeg / 1000000.0));
		reply = rgl->get(request);
		connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
		timer.start(5000);   // 5 secs. timeout
		loop.exec();
		if(timer.isActive()) {
			timer.stop();
			if(reply->error() > 0) {
				report_error("got error accessing oceans API of geonames.org: %s", qPrintable(reply->errorString()));
				goto clear_reply;
			}
			int v = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
			if (v < 200 || v >= 300)
				goto clear_reply;
			QByteArray fullReply = reply->readAll();
			QJsonParseError errorObject;
			QJsonDocument jsonDoc = QJsonDocument::fromJson(fullReply, &errorObject);
			if (errorObject.error != QJsonParseError::NoError) {
				report_error("error parsing geonames.org response: %s", qPrintable(errorObject.errorString()));
				goto clear_reply;
			}
			QJsonObject obj = jsonDoc.object();
			QVariant oceanObject = obj.value("ocean").toVariant();
			QVariantMap oceanName = oceanObject.toMap();
			if (oceanName["name"].isValid()) {
				int idx;
				if (ds->taxonomy.category == NULL)
					ds->taxonomy.category = alloc_taxonomy();
				idx = taxonomy_index_for_category(&ds->taxonomy, TC_OCEAN);
				if (idx == -1)
					idx = ds->taxonomy.nr;
				if (idx < TC_NR_CATEGORIES) {
					ds->taxonomy.category[idx].category = TC_OCEAN;
					ds->taxonomy.category[idx].origin = taxonomy::GEOCODED;
					ds->taxonomy.category[idx].value = copy_string(qPrintable(oceanName["name"].toString()));
					if (idx == ds->taxonomy.nr)
						ds->taxonomy.nr++;
				}
				mark_divelist_changed(true);
			}
		} else {
			report_error("timeout accessing geonames.org");
			disconnect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
			reply->abort();
		}

clear_reply:
		reply->deleteLater();
	}
	rgl->deleteLater();
}

void ReverseGeoLookupThread::lookup(dive_site *ds)
{
	if (!ds)
		return;
	GeoLookupInfo info;
	info.lat = ds->latitude;
	info.lon = ds->longitude;
	info.uuid = ds->uuid;

	geo_lookup_data.clear();
	geo_lookup_data.append(info);
	run();
}

extern "C" void add_geo_information_for_lookup(degrees_t latitude, degrees_t longitude, uint32_t uuid) {
	GeoLookupInfo info;
	info.lat = latitude;
	info.lon = longitude;
	info.uuid = uuid;

	geo_lookup_data.append(info);
}
