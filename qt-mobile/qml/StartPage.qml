import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

Item {
	ColumnLayout {
		id: startpage
		anchors.fill: parent
		anchors.margins: units.gridUnit * 4

		property int buttonWidth: welcomeText.width * 0.66

		Label {
			Layout.bottomMargin: units.largeSpacing
			text: "Subsurface Divelog"
			font.pointSize: welcomeText.font.pointSize * 2
		}

		Label {
			id: welcomeText
			Layout.fillWidth: true
			Layout.bottomMargin: units.largeSpacing
			text: "No recorded dives found. You can download your dives to this device from the Subsurface cloud storage service, from your dive computer, or add them manually."
			wrapMode: Text.WordWrap
			Layout.columnSpan: 2
		}
		Button {
			id: cloudstorageButton
			Layout.bottomMargin: units.largeSpacing
			Layout.preferredWidth: startpage.buttonWidth
			text: "Connect to CloudStorage..."
			onClicked: {
				stackView.push(prefsWindow)
			}
		}
		Button {
			id: computerButton
			Layout.preferredWidth: startpage.buttonWidth
			Layout.bottomMargin: units.largeSpacing
			text: "Transfer from dive computer..."
			onClicked: {
				stackView.push(downloadDivesWindow)
			}
		}
		Button {
			id: manualButton
			Layout.preferredWidth: startpage.buttonWidth
			Layout.bottomMargin: units.largeSpacing
			text: "Add dive manually..."
			onClicked: {
				manager.addDive();
				stackView.push(detailsWindow)
			}
		}
		Item {
			width: parent.width
			Layout.fillHeight: true
		}
	}
}