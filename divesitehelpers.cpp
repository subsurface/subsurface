//
// infrastructure to deal with dive sites
//

#include "divesitehelpers.h"

#include "divesite.h"
#include "helpers.h"
#include "usersurvey.h"
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
	request.setRawHeader("Accept", "text/json");
	request.setRawHeader("User-Agent", getUserAgent().toUtf8());
	QEventLoop loop;
	QString apiCall("http://open.mapquestapi.com/nominatim/v1/reverse.php?format=json&accept-language=%1&lat=%2&lon=%3");
	Q_FOREACH (const GeoLookupInfo& info, geo_lookup_data ) {
		request.setUrl(apiCall.arg(uiLanguage(NULL)).arg(info.lat.udeg / 1000000.0).arg(info.lon.udeg / 1000000.0));

		QNetworkReply *reply = rgl->get(request);
		QTimer timer;
		timer.setSingleShot(true);

		QEventLoop loop;
		connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
		connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
		timer.start(500);   // 30 secs. timeout
		loop.exec();

		if(timer.isActive()) {
			timer.stop();
			if(reply->error() > 0)
				goto clear_reply;

			int v = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
			if (v < 200 || v >= 300)
				goto clear_reply;

			QJsonParseError errorObject;
			QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &errorObject);
			if (errorObject.error != QJsonParseError::NoError)
				goto clear_reply;

			QJsonObject obj = jsonDoc.object();
			QJsonObject address = obj.value("address").toObject();

			struct dive_site *ds = get_dive_site_by_uuid(info.uuid);
			ds->notes = add_to_string(ds->notes, "countrytag: %s", address.value("country").toString().toUtf8().data());

		} else {
			disconnect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
			reply->abort();
		}

		clear_reply:
		reply->deleteLater();
	}
	rgl->deleteLater();
}

extern "C" void add_geo_information_for_lookup(degrees_t latitude, degrees_t longitude, uint32_t uuid) {
	GeoLookupInfo info;
	info.lat = latitude;
	info.lon = longitude;
	info.uuid = uuid;

	geo_lookup_data.append(info);
}
