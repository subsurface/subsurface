// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2 as Controls
import QtQuick.Layouts 1.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

Kirigami.Page {
	id: tripEditPage
	objectName: "TripDetails"
	property string tripId
	property string tripLocation
	property string tripNotes

	title: "" !== tripLocation ? tripLocation : qsTr("Trip details")
	state: "view"
	padding: Kirigami.Units.largeSpacing
	background: Rectangle { color: subsurfaceTheme.backgroundColor }

	// we want to use our own colors for Kirigami, so let's define our colorset
	Kirigami.Theme.inherit: false
	Kirigami.Theme.colorSet: Kirigami.Theme.Button
	Kirigami.Theme.backgroundColor: subsurfaceTheme.backgroundColor
	Kirigami.Theme.textColor: subsurfaceTheme.textColor

	actions.main: saveAction
	actions.right: cancelAction
	onVisibleChanged: {
		resetState()
	}
	onTripIdChanged: {
		resetState()
	}

	function resetState() {
		// make sure we have the right width and reset focus / state if there aren't any unsaved changes
		if (parent)
			width = parent.width
		if (tripLocation === tripLocationField.text && tripNotes === tripNotesField.text) {
			tripLocationField.focus = false
			tripNotesField.focus = false
			state = "view"
		}

	}

	states: [
		State {
			name: "view"
			PropertyChanges { target: saveAction; enabled: false }
		},
		State {
			name: "edit"
			PropertyChanges { target: saveAction; enabled: true }
		}
	]

	property QtObject saveAction: Kirigami.Action {
		icon {
			name: ":/icons/document-save.svg"
			color: enabled ? subsurfaceTheme.primaryColor : subsurfaceTheme.backgroundColor
		}
		text: enabled ? qsTr("Save edits") : ""
		onTriggered: {
			manager.appendTextToLog("Save trip details triggered")
			manager.updateTripDetails(tripId, tripLocationField.text, tripNotesField.text)
			Qt.inputMethod.hide()
			pageStack.pop()
		}
	}
	property QtObject cancelAction: Kirigami.Action {
		text: qsTr("Cancel edit")
		icon {
			name: ":/icons/dialog-cancel.svg"
		}
		onTriggered: {
			manager.appendTextToLog("Cancel trip details edit")
			state = "view"
			pageStack.pop()
		}
	}

	Flickable {
		id: tripEditFlickable
		anchors.fill: parent
		GridLayout {
			columns: 2
			width: tripEditFlickable.width
			TemplateLabel {
				Layout.columnSpan: 2
				id: title
				text: qsTr("Edit trip details")
				font.pointSize: subsurfaceTheme.titlePointSize
				font.bold: true
			}
			Rectangle {
				id: spacer
				Layout.columnSpan: 2
				color: subsurfaceTheme.backgroundColor
				height: Kirigami.Units.gridUnit
				width: 1
			}

			TemplateLabel {
				text: qsTr("Trip location:")
				opacity: 0.6
			}
			SsrfTextField {
				id: tripLocationField
				Layout.fillWidth: true
				text: tripLocation
				flickable: tripEditFlickable
				onFocusChanged: {
					tripEditPage.state = "edit"
				}
			}
			TemplateLabel {
				Layout.columnSpan: 2
				text: qsTr("Trip notes")
				opacity: 0.6
			}
			Controls.TextArea {
				id: tripNotesField
				text: tripNotes
				textFormat: TextEdit.PlainText
				color: subsurfaceTheme.textColor
				Layout.columnSpan: 2
				Layout.fillWidth: true
				Layout.fillHeight: true
				Layout.minimumHeight: Kirigami.Units.gridUnit * 6
				selectByMouse: true
				wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
				onActiveFocusChanged: {
					tripEditPage.state = "edit"
				}
			}
		}
	}
}
