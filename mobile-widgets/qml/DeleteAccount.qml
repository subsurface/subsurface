// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.4 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Kirigami.ScrollablePage {
	id: deleteAccountPage
	property int pageWidth: deleteAccountPage.width - deleteAccountPage.leftPadding - deleteAccountPage.rightPadding
	title: qsTr("Delete Subsurface Cloud Account")
	background: Rectangle { color: subsurfaceTheme.backgroundColor }

	ColumnLayout {
		spacing: Kirigami.Units.largeSpacing
		width: deleteAccountPage.width
		Layout.margins: Kirigami.Units.gridUnit / 2

		Kirigami.Heading {
			text: qsTr("Delete Subsurface Cloud Account")
			color: subsurfaceTheme.textColor
			Layout.topMargin: Kirigami.Units.gridUnit
			Layout.alignment: Qt.AlignHCenter
			Layout.maximumWidth: pageWidth
			wrapMode: TextEdit.NoWrap
			fontSizeMode: Text.Fit
		}

		Kirigami.Heading {
			text: qsTr("Deleting your Subsurface Cloud account is permanent.\n") +
				qsTr("There is no way to undo this action.")
			level: 4
			color: subsurfaceTheme.textColor
			Layout.alignment: Qt.AlignHCenter
			Layout.topMargin: Kirigami.Units.largeSpacing * 3
			Layout.maximumWidth: pageWidth
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			anchors.horizontalCenter: parent.Center
			horizontalAlignment: Text.AlignHCenter
		}

		Kirigami.Heading {
			text: PrefCloudStorage.cloud_storage_email
			level: 4
			color: subsurfaceTheme.textColor
			Layout.alignment: Qt.AlignHCenter
			Layout.topMargin: Kirigami.Units.largeSpacing * 3
			Layout.maximumWidth: pageWidth
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			anchors.horizontalCenter: parent.Center
			horizontalAlignment: Text.AlignHCenter
		}


		TemplateButton {
			id: deleteCloudAccount
			Layout.alignment: Qt.AlignHCenter
			text: qsTr("delete cloud account")
			onClicked: {
				manager.appendTextToLog("request to delete account confirmed")
				manager.deleteAccount()
				rootItem.returnTopPage()
				}
		}

		TemplateButton {
			id: dontDeleteCloudAccount
			Layout.alignment: Qt.AlignHCenter
			text: qsTr("never mind")
			onClicked: {
				manager.appendTextToLog("request to delete account cancelled")
				rootItem.returnTopPage()
				}
		}
	}
}
