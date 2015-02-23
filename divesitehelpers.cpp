//
// infrastructure to deal with dive sites
//
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

extern "C" void reverseGeoLookup(degrees_t latitude, degrees_t longitude, uint32_t uuid)
{
	QNetworkRequest request;
	QNetworkAccessManager *rgl = new QNetworkAccessManager();
	request.setUrl(QString("http://open.mapquestapi.com/nominatim/v1/reverse.php?format=json&accept-language=%1&lat=%2&lon=%3")
		       .arg(uiLanguage(NULL)).arg(latitude.udeg / 1000000.0).arg(longitude.udeg / 1000000.0));
	request.setRawHeader("Accept", "text/json");
	request.setRawHeader("User-Agent", getUserAgent().toUtf8());
	QNetworkReply *reply = rgl->get(request);
	QEventLoop loop;
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
		struct dive_site *ds = get_dive_site_by_uuid(uuid);
		ds->notes = add_to_string(ds->notes, "countrytag: %s", address.value("country").toString().toUtf8().data());
	}
}

