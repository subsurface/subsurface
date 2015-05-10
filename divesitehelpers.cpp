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

struct GeoLoockupInfo {
	degrees_t lat;
	degrees_t lon;
	uint32_t uuid;
};

QVector<GeoLoockupInfo> geo_loockup_data;

ReverseGeoLoockupThread* ReverseGeoLoockupThread::instance() {
	static ReverseGeoLoockupThread* self = new ReverseGeoLoockupThread();
	return self;
}

ReverseGeoLoockupThread::ReverseGeoLoockupThread(QObject *obj) : QThread(obj)
{
}

void ReverseGeoLoockupThread::run() {
	if (geo_loockup_data.isEmpty())
		return;

	QNetworkRequest request;
	QNetworkAccessManager *rgl = new QNetworkAccessManager();
	request.setRawHeader("Accept", "text/json");
	request.setRawHeader("User-Agent", getUserAgent().toUtf8());
	QEventLoop loop;
	QString apiCall("http://open.mapquestapi.com/nominatim/v1/reverse.php?format=json&accept-language=%1&lat=%2&lon=%3");
	Q_FOREACH (const GeoLoockupInfo& info, geo_loockup_data ) {
		request.setUrl(apiCall.arg(uiLanguage(NULL)).arg(info.lat.udeg / 1000000.0).arg(info.lon.udeg / 1000000.0));
		QNetworkReply *reply = rgl->get(request);
		QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
		loop.exec();
		QJsonParseError errorObject;
		QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &errorObject);
		if (errorObject.error != QJsonParseError::NoError) {
			qDebug() << errorObject.errorString();
		} else {
			QJsonObject obj = jsonDoc.object();
			QJsonObject address = obj.value("address").toObject();
			qDebug() << "found country:" << address.value("country").toString();
			struct dive_site *ds = get_dive_site_by_uuid(info.uuid);
			ds->notes = add_to_string(ds->notes, "countrytag: %s", address.value("country").toString().toUtf8().data());
		}

		reply->deleteLater();
	}
	rgl->deleteLater();
}

extern "C" void add_geo_information_for_loockup(degrees_t latitude, degrees_t longitude, uint32_t uuid) {
	GeoLoockupInfo info;
	info.lat = latitude;
	info.lon = longitude;
	info.uuid = uuid;

	geo_loockup_data.append(info);
}
