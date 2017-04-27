// SPDX-License-Identifier: GPL-2.0
#include "TabDiveInformation.h"
#include "ui_TabDiveInformation.h"
#include "../tagwidget.h"

#include <core/helpers.h>
#include <core/statistics.h>
#include <core/display.h>

TabDiveInformation::TabDiveInformation(QWidget *parent) : TabBase(parent), ui(new Ui::TabDiveInformation())
{
	ui->setupUi(this);
}

TabDiveInformation::~TabDiveInformation()
{
	delete ui;
}

void TabDiveInformation::clear()
{
	ui->sacText->clear();
	ui->otuText->clear();
	ui->maxcnsText->clear();
	ui->oxygenHeliumText->clear();
	ui->gasUsedText->clear();
	ui->dateText->clear();
	ui->diveTimeText->clear();
	ui->surfaceIntervalText->clear();
	ui->maximumDepthText->clear();
	ui->averageDepthText->clear();
	ui->waterTemperatureText->clear();
	ui->airTemperatureText->clear();
	ui->airPressureText->clear();
	ui->salinityText->clear();
}

void TabDiveInformation::updateData()
{
	clear();

	ui->maxcnsText->setText(QString("%1\%").arg(displayed_dive.maxcns));
	ui->otuText->setText(QString("%1").arg(displayed_dive.otu));
	ui->maximumDepthText->setText(get_depth_string(displayed_dive.maxdepth, true));
	ui->averageDepthText->setText(get_depth_string(displayed_dive.meandepth, true));
	ui->dateText->setText(get_short_dive_date_string(displayed_dive.when));
	ui->waterTemperatureText->setText(get_temperature_string(displayed_dive.watertemp, true));
	ui->airTemperatureText->setText(get_temperature_string(displayed_dive.airtemp, true));

	volume_t gases[MAX_CYLINDERS] = {};
	get_gas_used(&displayed_dive, gases);
	QString volumes;
	int mean[MAX_CYLINDERS], duration[MAX_CYLINDERS];
	per_cylinder_mean_depth(&displayed_dive, select_dc(&displayed_dive), mean, duration);
	volume_t sac;
	QString gaslist, SACs, separator;

	gaslist = ""; SACs = ""; volumes = ""; separator = "";
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		if (!is_cylinder_used(&displayed_dive, i))
			continue;
		gaslist.append(separator); volumes.append(separator); SACs.append(separator);
		separator = "\n";

		gaslist.append(gasname(&displayed_dive.cylinder[i].gasmix));
		if (!gases[i].mliter)
			continue;
		volumes.append(get_volume_string(gases[i], true));
		if (duration[i]) {
			sac.mliter = gases[i].mliter / (depth_to_atm(mean[i], &displayed_dive) * duration[i] / 60);
			SACs.append(get_volume_string(sac, true).append(tr("/min")));
		}
	}
	ui->gasUsedText->setText(volumes);
	ui->oxygenHeliumText->setText(gaslist);

	int sum = displayed_dive.dc.divemode != FREEDIVE ? 30 : 0;
	ui->diveTimeText->setText(get_time_string_s(displayed_dive.duration.seconds + sum, 0, false));

	struct dive *prevd;
	process_all_dives(&displayed_dive, &prevd);

	if (prevd)
		ui->surfaceIntervalText->setText(get_time_string_s(displayed_dive.when - (prevd->when + prevd->duration.seconds), 4,
									(displayed_dive.dc.divemode == FREEDIVE)));
	else
		ui->surfaceIntervalText->clear();

	ui->sacText->setText( mean[0] ? SACs : QString());

	if (displayed_dive.surface_pressure.mbar) 	/* this is ALWAYS displayed in mbar */
		ui->airPressureText->setText(QString("%1mbar").arg(displayed_dive.surface_pressure.mbar));
	else
		ui->airPressureText->clear();

	if (displayed_dive.salinity)
		ui->salinityText->setText(QString("%1g/l").arg(displayed_dive.salinity / 10.0));
	else
		ui->salinityText->clear();

}
