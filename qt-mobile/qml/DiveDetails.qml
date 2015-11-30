import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.subsurfacedivelog.mobile 1.0
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

MobileComponents.Page {
	id: diveDetailsWindow
	width: parent.width
	objectName: "DiveDetails"
	flickable: flick

	property string location
	property string dive_id
	property string airtemp
	property string watertemp
	property string suit
	property string buddy
	property string divemaster;
	property string notes;
	property string date
	property string number

	onDive_idChanged: {
		qmlProfile.diveId = dive_id
		qmlProfile.update()
	}

	ScrollView {
		anchors.fill: parent
		Flickable {
			id: flick
			anchors.fill: parent
			contentHeight: content.height
			clip: true
			Item {
				id: content
				width: flick.width
				height: childrenRect.height + MobileComponents.Units.smallSpacing * 2

				ColumnLayout {
					anchors {
						left: parent.left
						right: parent.right
						top: parent.top
						margins: MobileComponents.Units.smallSpacing
					}
					spacing: MobileComponents.Units.smallSpacing


					GridLayout {
						id: editorDetails
						width: parent.width
						columns: 2

						MobileComponents.Heading {
							Layout.columnSpan: 2
							text: "Dive " + number + " (" + date + ")"
						}

						Item {
							Layout.columnSpan: 2
							Layout.fillWidth: true
							Layout.preferredHeight: qmlProfile.visible ? qmlProfile.height : profileHideButton.height
							QMLProfile {
								id: qmlProfile
								height: MobileComponents.Units.gridUnit * 25
								anchors {
									top: parent.top
									left: parent.left
									right: parent.right
								}
								//Rectangle { color: "green"; opacity: 0.4; anchors.fill: parent } // used for debugging the dive profile sizing, will be removed later
							}
							Button {
								id: profileHideButton
								anchors {
									right: parent.right
									top: parent.top
								}
								text: "Hide Dive Profile"
								onClicked: {
									qmlProfile.visible = !qmlProfile.visible
									if (qmlProfile.visible) {
										text = "Hide Dive Profile"
									} else {
										text = "Show Dive Profile"
									}
								}
							}
						}
						MobileComponents.Label {
							Layout.alignment: Qt.AlignRight
							text: "Location:"
						}
						TextField {
							id: txtLocation; text: location;
							Layout.fillWidth: true
						}

						MobileComponents.Label {
							Layout.alignment: Qt.AlignRight
							text: "Air Temp:"
						}
						TextField {
							id: txtAirTemp
							text: airtemp
							Layout.fillWidth: true
						}

						MobileComponents.Label {
							Layout.alignment: Qt.AlignRight
							text: "Water Temp:"
						}
						TextField {
							id: txtWaterTemp
							text: watertemp
							Layout.fillWidth: true
						}

						MobileComponents.Label {
							Layout.alignment: Qt.AlignRight
							text: "Suit:"

						}
						TextField {
							id: txtSuit
							text: suit
							Layout.fillWidth: true
						}

						MobileComponents.Label {
							Layout.alignment: Qt.AlignRight
							text: "Buddy:"
						}
						TextField {
							id: txtBuddy
							text: buddy
							Layout.fillWidth: true
						}

						MobileComponents.Label {
							Layout.alignment: Qt.AlignRight
							text: "Dive Master:"
						}
						TextField {
							id: txtDiveMaster
							text: divemaster
							Layout.fillWidth: true
						}

						MobileComponents.Label {
							Layout.alignment: Qt.AlignRight
							text: "Notes:"
						}
						TextArea {
							id: txtNotes
							text: notes
							focus: true
							Layout.fillWidth: true
							Layout.fillHeight: true
							selectByMouse: true
							wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
						}
					}
					Item {
						height: MobileComponents.Units.gridUnit * 3
						width: height // just to make sure the spacer doesn't produce scrollbars, but also isn't null
					}
				}
			}
		}
	}
}
