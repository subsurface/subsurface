// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFGEOCODING_H
#define QPREFGEOCODING_H

#include <QObject>
#include "core/taxonomy.h"


class qPrefGeocoding : public QObject {
	Q_OBJECT
	Q_PROPERTY(taxonomy_category first_category READ firstTaxonomyCategory WRITE setFirstTaxonomyCategory NOTIFY firstTaxonomyCategoryChanged);
	Q_PROPERTY(taxonomy_category second_category READ secondTaxonomyCategory WRITE setSecondTaxonomyCategory NOTIFY secondTaxonomyCategoryChanged);
	Q_PROPERTY(taxonomy_category third_category READ thirdTaxonomyCategory WRITE setThirdTaxonomyCategory NOTIFY thirdTaxonomyCategoryChanged);

public:
	qPrefGeocoding(QObject *parent = NULL) : QObject(parent) {};
	~qPrefGeocoding() {};
	static qPrefGeocoding *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

public:
	taxonomy_category firstTaxonomyCategory() const;
	taxonomy_category secondTaxonomyCategory() const;
	taxonomy_category thirdTaxonomyCategory() const;

public slots:
	void setFirstTaxonomyCategory(taxonomy_category value);
	void setSecondTaxonomyCategory(taxonomy_category value);
	void setThirdTaxonomyCategory(taxonomy_category value);

signals:
	void firstTaxonomyCategoryChanged(taxonomy_category value);
	void secondTaxonomyCategoryChanged(taxonomy_category value);
	void thirdTaxonomyCategoryChanged(taxonomy_category value);

private:
	const QString group = QStringLiteral("geocoding");
	static qPrefGeocoding *m_instance;

	void diskFirstTaxonomyCategory(bool doSync);
	void diskSecondTaxonomyCategory(bool doSync);
	void diskThirdTaxonomyCategory(bool doSync);
};

#endif
