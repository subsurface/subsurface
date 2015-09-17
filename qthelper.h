#ifndef QTHELPER_H
#define QTHELPER_H

#include <QMultiMap>
#include <QString>
#include <stdint.h>
#include "dive.h"
#include "divelist.h"
#include <QTranslator>
#include <QDir>

class Dive {
private:
	int m_number;
	int m_id;
	int m_rating;
	QString m_date;
	timestamp_t m_timestamp;
	QString m_time;
	QString m_location;
	QString m_duration;
	QString m_depth;
	QString m_divemaster;
	QString m_buddy;
	QString m_airTemp;
	QString m_waterTemp;
	QString m_notes;
	QString m_tags;
	QString m_gas;
	QString m_sac;
	QString m_weight;
	QString m_suit;
	QString m_cylinder;
	QString m_trip;
	struct dive *dive;
	void put_date_time();
	void put_timestamp();
	void put_location();
	void put_duration();
	void put_depth();
	void put_divemaster();
	void put_buddy();
	void put_temp();
	void put_notes();
	void put_tags();
	void put_gas();
	void put_sac();
	void put_weight();
	void put_suit();
	void put_cylinder();
	void put_trip();

public:
	Dive(struct dive *dive)
		: dive(dive)
	{
		m_number = dive->number;
		m_id = dive->id;
		m_rating = dive->rating;
		put_date_time();
		put_location();
		put_duration();
		put_depth();
		put_divemaster();
		put_buddy();
		put_temp();
		put_notes();
		put_tags();
		put_gas();
		put_sac();
		put_timestamp();
		put_weight();
		put_suit();
		put_cylinder();
		put_trip();
	}
	Dive();
	~Dive();
	int number() const;
	int id() const;
	int rating() const;
	QString date() const;
	timestamp_t timestamp() const;
	QString time() const;
	QString location() const;
	QString duration() const;
	QString depth() const;
	QString divemaster() const;
	QString buddy() const;
	QString airTemp() const;
	QString waterTemp() const;
	QString notes() const;
	QString tags() const;
	QString gas() const;
	QString sac() const;
	QString weight() const;
	QString suit() const;
	QString cylinder() const;
	QString trip() const;
};

// global pointers for our translation
extern QTranslator *qtTranslator, *ssrfTranslator;

QString weight_string(int weight_in_grams);
QString distance_string(int distanceInMeters);
bool gpsHasChanged(struct dive *dive, struct dive *master, const QString &gps_text, bool *parsed_out = 0);
extern "C" const char *printGPSCoords(int lat, int lon);
QList<int> getDivesInTrip(dive_trip_t *trip);
QString get_gas_string(struct gasmix gas);
QString get_divepoint_gas_string(const divedatapoint& dp);
void read_hashes();
void write_hashes();
void updateHash(struct picture *picture);
QByteArray hashFile(const QString filename);
void learnImages(const QDir dir, int max_recursions, bool recursed);
void add_hash(const QString filename, QByteArray hash);
QString localFilePath(const QString originalFilename);
QString fileFromHash(char *hash);
void learnHash(struct picture *picture, QByteArray hash);
extern "C" void cache_picture(struct picture *picture);
weight_t string_to_weight(const char *str);
depth_t string_to_depth(const char *str);
pressure_t string_to_pressure(const char *str);
volume_t string_to_volume(const char *str, pressure_t workp);
fraction_t string_to_fraction(const char *str);
int getCloudURL(QString &filename);
void loadPreferences();
bool parseGpsText(const QString &gps_text, double *latitude, double *longitude);
QByteArray getCurrentAppState();
void setCurrentAppState(QByteArray state);
extern "C" bool in_planner();

#endif // QTHELPER_H
