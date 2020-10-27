// SPDX-License-Identifier: GPL-2.0
#include "tab_divestatistics.h"
#include "ui_tab_divestatistics.h"

#include "core/qthelper.h"
#include "core/selection.h"
#include "core/statistics.h"
#include <QQuickView>
#include <QtQuick>


TabDiveStatistics::TabDiveStatistics(QWidget *parent) : TabBase(parent), ui(new Ui::TabDiveStatistics())
{
	ui->setupUi(this);
	const QUrl urlStatsWidget = QUrl(QStringLiteral("qrc:/qml/stats-widget.qml"));


	QString CSSColourLabel = "QLabel { color: mediumblue; }";

	ui->yVarLabel->setStyleSheet(CSSColourLabel);
	ui->xVarLabel->setStyleSheet(CSSColourLabel);
	ui->cCodedLabel->setStyleSheet(CSSColourLabel);
	ui->graphTypeLabel->setStyleSheet(CSSColourLabel);
	
	QStringList graphTypes = {" - ", "Scatterplot", "Bar graph"};
	QStringList varys = {"No. dives", "Depth", "SAC", "Temp", "SAC"};
	QStringList varxs = {"Depth", "Duration", "Temp", "SAC"};
	QStringList cCode = {" - ", "Temperature", "Tag", "Suit", "DiveMode", "Depth", "Duration"};
	for (int i = 0; i<2; i++)
		ui->graphType->addItem(graphTypes[i]);
	for (int i = 0; i<4; i++)
		ui->yVarName->addItem(varys[i]);
	for (int i = 0; i<4; i++)
		ui->xVarName->addItem(varxs[i]);
	for (int i = 0; i<6; i++)
		ui->cCodedName->addItem(cCode[i]);
}

TabDiveStatistics::~TabDiveStatistics()
{
	delete ui;
}

void TabDiveStatistics::clear()
{
}

// This function gets called if a field gets updated by an undo command.
// Refresh the corresponding UI field.
/*
void TabDiveStatistics::divesChanged(const QVector<dive *> &dives, DiveField field)
{
	// If none of the changed dives is selected, do nothing
	if (std::none_of(dives.begin(), dives.end(), [] (const dive *d) { return d->selected; }))
		return;

	// TODO: make this more fine grained. Currently, the core can only calculate *all* statistics.
	if (field.duration || field.depth || field.mode || field.air_temp || field.water_temp)
		updateData();
}

void TabDiveStatistics::cylinderChanged(dive *d)
{
	// If the changed dive is not selected, do nothing
	if (!d->selected)
		return;
	updateData();
}
*/

void TabDiveStatistics::updateData()
{

}
	
void TabDiveStatistics::on_createGraphButton_clicked() 
{

	ui->graphImage->setSource(QUrl(urlStatsWidget));
	ui->graphImage->show();

}


