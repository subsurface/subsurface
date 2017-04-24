#include "TabDiveStatistics.h"
#include "ui_TabDiveStatistics.h"

#include <core/helpers.h>
#include <core/display.h>
#include <core/statistics.h>

TabDiveStatistics::TabDiveStatistics(QWidget *parent) : TabBase(parent), ui(new Ui::TabDiveStatistics())
{
	ui->setupUi(this);
	ui->sacLimits->overrideMaxToolTipText(tr("Highest total SAC of a dive"));
	ui->sacLimits->overrideMinToolTipText(tr("Lowest total SAC of a dive"));
	ui->sacLimits->overrideAvgToolTipText(tr("Average total SAC of all selected dives"));
	ui->tempLimits->overrideMaxToolTipText(tr("Highest temperature"));
	ui->tempLimits->overrideMinToolTipText(tr("Lowest temperature"));
	ui->tempLimits->overrideAvgToolTipText(tr("Average temperature of all selected dives"));
	ui->depthLimits->overrideMaxToolTipText(tr("Deepest dive"));
	ui->depthLimits->overrideMinToolTipText(tr("Shallowest dive"));
	ui->timeLimits->overrideMaxToolTipText(tr("Longest dive"));
	ui->timeLimits->overrideMinToolTipText(tr("Shortest dive"));
	ui->timeLimits->overrideAvgToolTipText(tr("Average length of all selected dives"));

	Q_FOREACH (QObject *obj, children()) {
		if (QLabel *label = qobject_cast<QLabel *>(obj))
			label->setAlignment(Qt::AlignHCenter);
	}
}

TabDiveStatistics::~TabDiveStatistics()
{
	delete ui;
}

void TabDiveStatistics::clear()
{
	ui->depthLimits->clear();
	ui->sacLimits->clear();
	ui->divesAllText->clear();
	ui->tempLimits->clear();
	ui->totalTimeAllText->clear();
	ui->timeLimits->clear();
}

void TabDiveStatistics::updateData()
{
	clear();
	ui->depthLimits->setMaximum(get_depth_string(stats_selection.max_depth, true));
	ui->depthLimits->setMinimum(get_depth_string(stats_selection.min_depth, true));
	// the overall average depth is really confusing when listed between the
	// deepest and shallowest dive - let's just not set it
	// ui->depthLimits->setAverage(get_depth_string(stats_selection.avg_depth, true));


	if (stats_selection.max_sac.mliter)
		ui->sacLimits->setMaximum(get_volume_string(stats_selection.max_sac, true).append(tr("/min")));
	else
		ui->sacLimits->setMaximum("");
	if (stats_selection.min_sac.mliter)
		ui->sacLimits->setMinimum(get_volume_string(stats_selection.min_sac, true).append(tr("/min")));
	else
		ui->sacLimits->setMinimum("");
	if (stats_selection.avg_sac.mliter)
		ui->sacLimits->setAverage(get_volume_string(stats_selection.avg_sac, true).append(tr("/min")));
	else
		ui->sacLimits->setAverage("");

	temperature_t temp;
	temp.mkelvin = stats_selection.max_temp;
	ui->tempLimits->setMaximum(get_temperature_string(temp, true));
	temp.mkelvin = stats_selection.min_temp;
	ui->tempLimits->setMinimum(get_temperature_string(temp, true));
	if (stats_selection.combined_temp && stats_selection.combined_count) {
		const char *unit;
		get_temp_units(0, &unit);
		ui->tempLimits->setAverage(QString("%1%2").arg(stats_selection.combined_temp / stats_selection.combined_count, 0, 'f', 1).arg(unit));
	}


	ui->divesAllText->setText(QString::number(stats_selection.selection_size));
	ui->totalTimeAllText->setText(get_time_string_s(stats_selection.total_time.seconds, 0, (displayed_dive.dc.divemode == FREEDIVE)));
	int seconds = stats_selection.total_time.seconds;
	if (stats_selection.selection_size)
		seconds /= stats_selection.selection_size;
	ui->timeLimits->setAverage(get_time_string_s(seconds, 0,(displayed_dive.dc.divemode == FREEDIVE)));
	if (amount_selected > 1) {
		ui->timeLimits->setMaximum(get_time_string_s(stats_selection.longest_time.seconds, 0, (displayed_dive.dc.divemode == FREEDIVE)));
		ui->timeLimits->setMinimum(get_time_string_s(stats_selection.shortest_time.seconds, 0, (displayed_dive.dc.divemode == FREEDIVE)));
	} else {
		ui->timeLimits->setMaximum("");
		ui->timeLimits->setMinimum("");
	}

	QVector<QPair<QString, int> > gasUsed;
	QString gasUsedString;
	volume_t vol;
	selectedDivesGasUsed(gasUsed);
	for (int j = 0; j < 20; j++) {
		if (gasUsed.isEmpty())
			break;
		QPair<QString, int> gasPair = gasUsed.last();
		gasUsed.pop_back();
		vol.mliter = gasPair.second;
		gasUsedString.append(gasPair.first).append(": ").append(get_volume_string(vol, true)).append("\n");
	}
	if (!gasUsed.isEmpty())
		gasUsedString.append("...");
	volume_t o2_tot = {}, he_tot = {};
	selected_dives_gas_parts(&o2_tot, &he_tot);

	/* No need to show the gas mixing information if diving
		* with pure air, and only display the he / O2 part when
		* it is used.
		*/
	if (he_tot.mliter || o2_tot.mliter) {
		gasUsedString.append(tr("These gases could be\nmixed from Air and using:\n"));
		if (he_tot.mliter)
			gasUsedString.append(QString("He: %1").arg(get_volume_string(he_tot, true)));
		if (he_tot.mliter && o2_tot.mliter)
			gasUsedString.append(tr(" and "));
		if (o2_tot.mliter)
			gasUsedString.append(QString("O2: %2\n").arg(get_volume_string(o2_tot, true)));
	}
	ui->gasConsumption->setText(gasUsedString);
}

