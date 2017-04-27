// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.3
import QtQuick.Controls 2.0
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.0 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Kirigami.Page {

	title: qsTr("Preferences")
	actions {
		main: Kirigami.Action {
			text: qsTr("Save")
			iconName: "document-save"
			onTriggered: {
				manager.distanceThreshold = distanceThreshold.text
				manager.timeThreshold = timeThreshold.text
				manager.savePreferences()
				stackView.pop()
			}
		}
	}

	GridLayout {

		signal accept

		columns: 2
		width: parent.width - Kirigami.Units.gridUnit
		anchors {
			fill: parent
			margins: Kirigami.Units.gridUnit / 2
		}

		Kirigami.Heading {
			text: qsTr("Preferences")
			Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
			Layout.columnSpan: 2
		}

		Kirigami.Heading {
			text: qsTr("Subsurface GPS data webservice")
			level: 3
			Layout.topMargin: Kirigami.Units.largeSpacing
			Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
			Layout.columnSpan: 2
		}

		Kirigami.Label {
			text: qsTr("Distance threshold (meters)")
			Layout.alignment: Qt.AlignRight
		}

		TextField {
			id: distanceThreshold
			text: manager.distanceThreshold
			Layout.fillWidth: true
		}

		Kirigami.Label {
			text: qsTr("Time threshold (minutes)")
			Layout.alignment: Qt.AlignRight
		}

		TextField {
			id: timeThreshold
			text: manager.timeThreshold
			Layout.fillWidth: true
		}

		Item {
			Layout.fillHeight: true
		}
	}
}
