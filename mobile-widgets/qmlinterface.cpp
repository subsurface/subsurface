// SPDX-License-Identifier: GPL-2.0
#include "qmlinterface.h"
#include <QQmlEngine>

QMLInterface *QMLInterface::instance()
{
	static QMLInterface *self = new QMLInterface;
	return self;
}

void QMLInterface::setup(QQmlContext *ct)
{
	// Register interface class
	ct->setContextProperty("Backend", this);

	// Make enums available as types
	qmlRegisterUncreatableType<QMLInterface>("org.subsurfacedivelog.mobile",1,0,"Enums","Enum is not a type");

	// relink signals to QML
	connect(qPrefCloudStorage::instance(), &qPrefCloudStorage::cloud_verification_statusChanged,
		[this] (int value) { emit cloud_verification_statusChanged(CLOUD_STATUS(value)); });
	connect(qPrefUnits::instance(), &qPrefUnits::duration_unitsChanged,
			[this] (int value) { emit duration_unitsChanged(DURATION(value)); });
	connect(qPrefUnits::instance(), &qPrefUnits::lengthChanged,
			[this] (int value) { emit lengthChanged(LENGTH(value)); });
	connect(qPrefUnits::instance(), &qPrefUnits::pressureChanged,
			[this] (int value) { emit pressureChanged(PRESSURE(value)); });
	connect(qPrefUnits::instance(), &qPrefUnits::temperatureChanged,
			[this] (int value) { emit temperatureChanged(TEMPERATURE(value)); });
	connect(qPrefUnits::instance(), &qPrefUnits::unit_systemChanged,
			[this] (int value) { emit unit_systemChanged(UNIT_SYSTEM(value)); });
	connect(qPrefUnits::instance(), &qPrefUnits::vertical_speed_timeChanged,
			[this] (int value) { emit vertical_speed_timeChanged(TIME(value)); });
	connect(qPrefUnits::instance(), &qPrefUnits::volumeChanged,
			[this] (int value) { emit volumeChanged(VOLUME(value)); });
	connect(qPrefUnits::instance(), &qPrefUnits::weightChanged,
			[this] (int value) { emit weightChanged(WEIGHT(value)); });

	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::ascratelast6mChanged,
			instance(), &QMLInterface::ascratelast6mChanged);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::ascratestopsChanged,
			instance(), &QMLInterface::ascratestopsChanged);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::ascrate50Changed,
			instance(), &QMLInterface::ascrate50Changed);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::ascrate75Changed,
			instance(), &QMLInterface::ascrate75Changed);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::descrateChanged,
			instance(), &QMLInterface::descrateChanged);

	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::safetystopChanged,
			instance(), &QMLInterface::safetystopChanged);
	connect(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::gflowChanged,
			instance(), &QMLInterface::gflowChanged);
	connect(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::gfhighChanged,
			instance(), &QMLInterface::gfhighChanged);
	connect(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::vpmb_conservatismChanged,
			instance(), &QMLInterface::vpmb_conservatismChanged);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::drop_stone_modeChanged,
			instance(), &QMLInterface::drop_stone_modeChanged);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::last_stopChanged,
			instance(), &QMLInterface::last_stop6mChanged);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::switch_at_req_stopChanged,
			instance(), &QMLInterface::switch_at_req_stopChanged);

	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::problemsolvingtimeChanged,
			instance(), &QMLInterface::problemsolvingtimeChanged);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::bottomsacChanged,
			instance(), &QMLInterface::bottomsacChanged);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::decosacChanged,
			instance(), &QMLInterface::decosacChanged);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::sacfactorChanged,
			instance(), &QMLInterface::sacfactorChanged);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::problemsolvingtimeChanged,
			instance(), &QMLInterface::problemsolvingtimeChanged);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::o2narcoticChanged,
			instance(), &QMLInterface::o2narcoticChanged);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::bottompo2Changed,
			instance(), &QMLInterface::bottompo2Changed);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::decopo2Changed,
			instance(), &QMLInterface::decopo2Changed);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::bestmixendChanged,
			instance(), &QMLInterface::bestmixendChanged);

	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::display_runtimeChanged,
			instance(), &QMLInterface::display_runtimeChanged);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::display_durationChanged,
			instance(), &QMLInterface::display_durationChanged);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::display_transitionsChanged,
			instance(), &QMLInterface::display_transitionsChanged);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::verbatim_planChanged,
			instance(), &QMLInterface::verbatim_planChanged);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::display_variationsChanged,
			instance(), &QMLInterface::display_variationsChanged);

	// calculate divesummary first time.
	// this is needed in order to load the divesummary page
	diveSummary::summaryCalculation(0, 3);
}


void QMLInterface::summaryCalculation(int primaryPeriod, int secondaryPeriod)
{
	diveSummary::summaryCalculation(primaryPeriod, secondaryPeriod);
	emit diveSummaryTextChanged(diveSummary::diveSummaryText);
}
