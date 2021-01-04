// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

Kirigami.Page {
	id: statisticsPagge
	objectName: "StatisticsPage"
	title: qsTr("Statistics")
	leftPadding: 0
	topPadding: 0
	rightPadding: 0
	bottomPadding: 0
	width: rootItem.colWidth
	StatsView {
		id: statisticsWidget
		anchors.fill: parent
		Component.onCompleted: {
			console.log("Statistics widget loaded")
		}
	}
}
