// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami as Kirigami

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

	Item {
		parent: tripEditPage
		z: 999
		anchors.bottom: parent.bottom
		anchors.left: parent.left
		anchors.right: parent.right
		height: Kirigami.Units.gridUnit * 3 + Kirigami.Units.smallSpacing * 2
		Row {
			anchors.centerIn: parent
			spacing: Kirigami.Units.gridUnit
			SsrfToolButton {
				iconSource: "qrc:/icons/document-save.svg"
				highlighted: true
				onClicked: {
					manager.updateTripDetails(tripId, tripLocationField.text, tripNotesField.text)
					Qt.inputMethod.hide()
					pageStack.pop()
				}
			}
			SsrfToolButton {
				iconSource: "qrc:/icons/dialog-cancel.svg"
				onClicked: {
					state = "view"
					pageStack.pop()
				}
			}
		}
	}
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

	property Kirigami.Action saveAction: Kirigami.Action {
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
	property Kirigami.Action cancelAction: Kirigami.Action {
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
		bottomMargin: Kirigami.Units.gridUnit * 4
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
				onPressed: waitForKeyboard.start()
				onCursorRectangleChanged: ensureVisible()
				// ensure the cursor stays above the keyboard when editing trip notes
				function ensureVisible() {
					var flickable = tripEditFlickable
					var positionInFlickable = tripNotesField.mapToItem(flickable.contentItem, 0, 0)
					var taY = positionInFlickable.y + cursorRectangle.y
					// On Android 16+ the window no longer shrinks when the keyboard appears
					// (edge-to-edge opt-in killed adjustResize), so use the keyboard height
					// directly to determine how much of the flickable is actually visible.
					var keyboardH = Qt.inputMethod.visible ? Qt.inputMethod.keyboardRectangle.height : 0
					var visibleHeight = flickable.height - keyboardH
					if (taY > flickable.contentY + visibleHeight - 4 * Kirigami.Units.gridUnit)
						flickable.contentY = Math.max(0, 4 * Kirigami.Units.gridUnit + taY - visibleHeight)
					while (taY < flickable.contentY)
						flickable.contentY -= 2 * Kirigami.Units.gridUnit
				}
				// give the OS enough time to actually resize the flickable
				Timer {
					id: waitForKeyboard
					interval: 300
					onTriggered: {
						if (!Qt.inputMethod.visible) {
							restart()
							return
						}
						tripNotesField.ensureVisible()
					}
				}
			}
		}
	}
}
