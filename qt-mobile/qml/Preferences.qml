import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents
import org.subsurfacedivelog.mobile 1.0


GridLayout {

	signal accept

	columns: 2
	width: parent.width - MobileComponents.Units.gridUnit
	anchors {
		fill: parent
		margins: MobileComponents.Units.gridUnit / 2
	}

	MobileComponents.Heading {
		text: "Preferences"
		Layout.bottomMargin: MobileComponents.Units.largeSpacing / 2
		Layout.columnSpan: 2
	}

	MobileComponents.Heading {
		text: "Subsurface GPS data webservice"
		level: 3
		Layout.topMargin: MobileComponents.Units.largeSpacing
		Layout.bottomMargin: MobileComponents.Units.largeSpacing / 2
		Layout.columnSpan: 2
	}

	MobileComponents.Label {
		text: "Distance threshold (meters)"
		Layout.alignment: Qt.AlignRight
	}

	TextField {
		id: distanceThreshold
		text: manager.distanceThreshold
		Layout.fillWidth: true
	}

	MobileComponents.Label {
		text: "Time threshold (minutes)"
		Layout.alignment: Qt.AlignRight
	}

	TextField {
		id: timeThreshold
		text: manager.timeThreshold
		Layout.fillWidth: true
	}

	Item { width: MobileComponents.Units.gridUnit; height: width }
	Item {
		height: saveButton.height
		width: saveButton.width
		Button {
			id: saveButton
			text: "Save"
			anchors.centerIn: parent
			onClicked: {
				manager.distanceThreshold = distanceThreshold.text
				manager.timeThreshold = timeThreshold.text
				manager.savePreferences()
				stackView.pop()
			}
		}
	}

	Item {
		Layout.fillHeight: true
	}
}
