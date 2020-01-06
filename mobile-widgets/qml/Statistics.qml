// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.12
import QtQuick.Controls 2.12 as Controls
//import QtQuick.Controls 1.4
//import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.3
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

Kirigami.ScrollablePage {
	title: qsTr("Annual statistics")
	objectName: "statistics" // we mainly use this in debug messages to the log
	property QtObject statisticsModel: mobileStatisticsModel
	Component {
		id: yearDelegate
		Kirigami.AbstractListItem {
			leftPadding: Kirigami.Units.largeSpacing
			rightPadding: Kirigami.Units.largeSpacing
			backgroundColor: subsurfaceTheme.backgroundColor
			width: parent.width
			Column {
				Row {
					spacing: 2 * Kirigami.Units.gridUnit
					TemplateLabel { text: year }
					TemplateLabel { text: dives + " " + qsTr("dive(s)") }
					TemplateLabel { text: qsTr("Total dive time:") + " " + totalTime }
				}
				GridLayout {
					columns: 4
					width: parent.width
					// Row 0 -- this should create consistent width, but doesn't
					Rectangle {
						width: yearDelegate.width / 4
						height: 1
					}
					Rectangle {
						width: yearDelegate.width / 4
						height: 1
					}
					Rectangle {
						width: yearDelegate.width / 4
						height: 1
					}
					Rectangle {
						width: yearDelegate.width / 4
						height: 1
					}

					// Row 1
					TemplateLabel { }
					TemplateLabel { text: qsTr("AVG") }
					TemplateLabel { text: qsTr("MIN") }
					TemplateLabel { text: qsTr("MAX") }
					// Row 2
					TemplateLabel { text: qsTr("time") }
					TemplateLabel { text: averageTime }
					TemplateLabel { text: shortestTime }
					TemplateLabel { text: longestTime }
					// Row 3
					TemplateLabel { text: qsTr("depth") }
					TemplateLabel { text: averageDepth }
					TemplateLabel { text: minDepth }
					TemplateLabel { text: maxDepth }
					// Row 4
					TemplateLabel { text: qsTr("SAC") }
					TemplateLabel { text: averageSac }
					TemplateLabel { text: minSac }
					TemplateLabel { text: maxSac }
				}
			}
		}
	}

	ListView {
		id: statisticsListView
		anchors.fill: parent
		model: statisticsModel
		delegate: yearDelegate
		bottomMargin: Kirigami.Units.iconSizes.medium + Kirigami.Units.gridUnit
	}
}
