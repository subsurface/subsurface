// SPDX-License-Identifier: GPL-2.0
#include "TabDiveStatistics.h"
#include "ui_TabDiveStatistics.h"

#include <core/qthelper.h>
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

	connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &TabDiveStatistics::divesChanged);

	const auto l = findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly);
	for (QLabel *label: l) {
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

// This function gets called if a field gets updated by an undo command.
// Refresh the corresponding UI field.
void TabDiveStatistics::divesChanged(const QVector<dive *> &dives, DiveField field)
{
	// If none of the changed dives is selected, do nothing
	if (std::none_of(dives.begin(), dives.end(), [] (const dive *d) { return d->selected; }))
		return;

	// TODO: make this more fine grained. Currently, the core can only calculate *all* statistics.
	if (field.duration || field.depth || field.mode || field.air_temp || field.water_temp)
		updateData();
}

void TabDiveStatistics::updateData()
{
	stats_t stats_selection;
	calculate_stats_selected(&stats_selection);
	clear();
	if (amount_selected > 1) {
		ui->depthLimits->setMaximum(get_depth_string(stats_selection.max_depth, true));
		ui->depthLimits->setMinimum(get_depth_string(stats_selection.min_depth, true));
		ui->depthLimits->setAverage(get_depth_string(stats_selection.combined_max_depth.mm / stats_selection.selection_size, true));
	} else {
		ui->depthLimits->setMaximum("");
		ui->depthLimits->setMinimum("");
		ui->depthLimits->setAverage(get_depth_string(stats_selection.max_depth, true));
	}

	if (stats_selection.max_sac.mliter && (stats_selection.max_sac.mliter != stats_selection.avg_sac.mliter))
		ui->sacLimits->setMaximum(get_volume_string(stats_selection.max_sac, true).append(tr("/min")));
	else
		ui->sacLimits->setMaximum("");
	if (stats_selection.min_sac.mliter && (stats_selection.min_sac.mliter != stats_selection.avg_sac.mliter))
		ui->sacLimits->setMinimum(get_volume_string(stats_selection.min_sac, true).append(tr("/min")));
	else
		ui->sacLimits->setMinimum("");
	if (stats_selection.avg_sac.mliter)
		ui->sacLimits->setAverage(get_volume_string(stats_selection.avg_sac, true).append(tr("/min")));
	else
		ui->sacLimits->setAverage("");

	if (stats_selection.combined_count > 1) {
		ui->tempLimits->setMaximum(get_temperature_string(stats_selection.max_temp, true));
		ui->tempLimits->setMinimum(get_temperature_string(stats_selection.min_temp, true));
	}
	if (stats_selection.combined_temp.mkelvin && stats_selection.combined_count) {
		temperature_t avg_temp;
		avg_temp.mkelvin = stats_selection.combined_temp.mkelvin / stats_selection.combined_count;
		ui->tempLimits->setAverage(get_temperature_string(avg_temp, true));
	}


	ui->divesAllText->setText(QString::number(stats_selection.selection_size));
	ui->totalTimeAllText->setText(get_dive_duration_string(stats_selection.total_time.seconds, tr("h"), tr("min"), tr("sec"),
		" ", displayed_dive.dc.divemode == FREEDIVE));
	
	int seconds = stats_selection.total_time.seconds;
	if (stats_selection.selection_size)
		seconds /= stats_selection.selection_size;
	ui->timeLimits->setAverage(get_dive_duration_string(seconds, tr("h"), tr("min"), tr("sec"),
			" ", displayed_dive.dc.divemode == FREEDIVE));
	if (amount_selected > 1) {
		ui->timeLimits->setMaximum(get_dive_duration_string(stats_selection.longest_time.seconds, tr("h"), tr("min"), tr("sec"),
			" ", displayed_dive.dc.divemode == FREEDIVE));
		ui->timeLimits->setMinimum(get_dive_duration_string(stats_selection.shortest_time.seconds, tr("h"), tr("min"), tr("sec"),
			" ", displayed_dive.dc.divemode == FREEDIVE));
	} else {
		ui->timeLimits->setMaximum("");
		ui->timeLimits->setMinimum("");
	}

	QVector<QPair<QString, int> > gasUsed = selectedDivesGasUsed();
	QString gasUsedString;
	volume_t vol;
	for (int j = 0; j < MAX_CYLINDERS; j++) {
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
		if (he_tot.mliter) {
			gasUsedString.append(tr("He"));
			gasUsedString.append(QString(": %1").arg(get_volume_string(he_tot, true)));
		}
		if (he_tot.mliter && o2_tot.mliter)
			gasUsedString.append(" ").append(tr("and")).append(" ");
		if (o2_tot.mliter) {
			gasUsedString.append(tr("O₂"));
			gasUsedString.append(QString(": %2\n").arg(get_volume_string(o2_tot, true)));
		}
	}
	ui->gasConsumption->setText(gasUsedString);
}

