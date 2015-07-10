import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.subsurfacedivelog.mobile 1.0

ApplicationWindow {
	title: qsTr("Subsurface")
	width: 500;

	QMLManager {
		id: manager
	}

	Preferences {
		id: prefsWindow
	}

	ColumnLayout {
		id: layout
		anchors.fill: parent
		spacing: 4

		Rectangle {
			id: topPart
			height: 35
			Layout.fillWidth: true
			Layout.maximumHeight: 35

			RowLayout {
				Button {
					id: prefsButton
					text: "Preferences"
					onClicked: {
						prefsWindow.show()
					}
				}

				Button {
					id: loadDivesButton
					text: "Load Dives"
					onClicked: {
						manager.loadDives();
					}
				}
			}

		}

		Rectangle {
			id: detailsPage
			Layout.fillHeight: true
			Layout.fillWidth: true
			Layout.minimumWidth: 100
			Layout.preferredHeight: 400
			Layout.preferredWidth: 200

			DiveList {
				anchors.fill: detailsPage
				id: diveDetails
			}
		}

	}
}
