import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Layouts 1.1
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents
import org.subsurfacedivelog.mobile 1.0

Item {
	ColumnLayout {
		id: startpage
		anchors.fill: parent
		anchors.margins: MobileComponents.Units.gridUnit / 2

		property int buttonWidth: width * 0.9

		MobileComponents.Heading {
			Layout.bottomMargin: MobileComponents.Units.largeSpacing
			text: "Subsurface Divelog"
		}

		MobileComponents.Label {
			id: welcomeText
			Layout.fillWidth: true
			Layout.bottomMargin: MobileComponents.Units.largeSpacing
			text: manager.startPageText
			wrapMode: Text.WordWrap
			Layout.columnSpan: 2
		}
		SubsurfaceButton {
			id: cloudstorageButton
			Layout.bottomMargin: MobileComponents.Units.largeSpacing
			Layout.preferredWidth: startpage.buttonWidth
			anchors.horizontalCenter: parent.horizontalCenter
			text: "Connect to CloudStorage..."
			onClicked: {
				stackView.push(cloudCredWindow)
			}
		}
		SubsurfaceButton {
			id: computerButton
			Layout.preferredWidth: startpage.buttonWidth
			Layout.bottomMargin: MobileComponents.Units.largeSpacing
			anchors.horizontalCenter: parent.horizontalCenter
			text: "Transfer from dive computer..."
			onClicked: {
				stackView.push(downloadDivesWindow)
			}
		}
		SubsurfaceButton {
			id: manualButton
			Layout.preferredWidth: startpage.buttonWidth
			Layout.bottomMargin: MobileComponents.Units.largeSpacing
			anchors.horizontalCenter: parent.horizontalCenter
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
