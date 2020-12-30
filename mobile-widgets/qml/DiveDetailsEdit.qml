// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2 as Controls
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

Item {
	id: detailsEdit
	property int dive_id
	property alias number: txtNumber.text
	property alias dateText: txtDate.text
	property alias locationText: locationBox.editText
	property alias locationIndex: locationBox.currentIndex
	property alias gpsText: txtGps.text
	property alias airtempText: txtAirTemp.text
	property alias watertempText: txtWaterTemp.text
	property alias suitIndex: suitBox.currentIndex
	property alias suitText: suitBox.editText
	property alias buddyIndex: buddyBox.currentIndex
	property alias buddyText: buddyBox.editText
	property alias divemasterIndex: divemasterBox.currentIndex
	property alias divemasterText: divemasterBox.editText
	property alias cylinderIndex0: cylinderBox0.currentIndex
	property alias cylinderIndex1: cylinderBox1.currentIndex
	property alias cylinderIndex2: cylinderBox2.currentIndex
	property alias cylinderIndex3: cylinderBox3.currentIndex
	property alias cylinderIndex4: cylinderBox4.currentIndex
	property alias notesText: txtNotes.text
	property alias durationText: txtDuration.text
	property alias depthText: txtDepth.text
	property alias weightText: txtWeight.text
	property var usedGas: []
	property var endpressure: []
	property var startpressure: []
	property alias gpsCheckbox: checkboxGPS.checked
	property alias suitModel: suitBox.model
	property alias divemasterModel: divemasterBox.model
	property alias buddyModel: buddyBox.model
	property alias cylinderModel0: cylinderBox0.model
	property alias cylinderModel1: cylinderBox1.model
	property alias cylinderModel2: cylinderBox2.model
	property alias cylinderModel3: cylinderBox3.model
	property alias cylinderModel4: cylinderBox4.model
	property alias locationModel: locationBox.model
	property int rating
	property int visibility
	property var usedCyl: []

	function focusReset() {
		// set the focus explicitlt (to steal from any other field), then unset
		editArea.focus = true
		editArea.focus = false
		Qt.inputMethod.hide()
	}

	function clearDetailsEdit() {
		detailsEdit.dive_id = 0
		detailsEdit.number = 0
		detailsEdit.dateText = ""
		detailsEdit.locationText = ""
		detailsEdit.durationText = ""
		detailsEdit.depthText = ""
		detailsEdit.airtempText = ""
		detailsEdit.watertempText = ""
		detailsEdit.divemasterText = ""
		detailsEdit.buddyText = ""
		suitBox.currentIndex = -1
		buddyBox.currentIndex = -1
		divemasterBox.currentIndex = -1
		cylinderBox0.currentIndex = -1
		cylinderBox1.currentIndex = -1
		cylinderBox2.currentIndex = -1
		cylinderBox3.currentIndex = -1
		cylinderBox4.currentIndex = -1
		detailsEdit.notesText = ""
		detailsEdit.rating = 0
		detailsEdit.visibility = 0
	}

	function saveData() {
		var state =  diveDetailsPage.state
		diveDetailsPage.state = "view" // run the transition
		// join cylinder info from separate string into a list.
		if (usedCyl[0] != null) {
			usedCyl[0] = cylinderBox0.currentText
			usedGas[0] = txtGasMix0.text
			startpressure[0] = txtStartPressure0.text
			endpressure[0] = txtEndPressure0.text
		}
		if (usedCyl[1] != null) {
			usedCyl[1] = cylinderBox1.currentText
			usedGas[1] = txtGasMix1.text
			startpressure[1] = txtStartPressure1.text
			endpressure[1] = txtEndPressure1.text
		}
		if (usedCyl[2] != null) {
			usedCyl[2] = cylinderBox2.currentText
			usedGas[2] = txtGasMix2.text
			startpressure[2] = txtStartPressure2.text
			endpressure[2] = txtEndPressure2.text
		}
		if (usedCyl[3] != null) {
			usedCyl[3] = cylinderBox3.currentText
			usedGas[3] = txtGasMix3.text
			startpressure[3] = txtStartPressure3.text
			endpressure[3] = txtEndPressure3.text
		}
		if (usedCyl[4] != null) {
			usedCyl[4] = cylinderBox4.currentText
			usedGas[4] = txtGasMix4.text
			startpressure[4] = txtStartPressure4.text
			endpressure[4] = txtEndPressure4.text
		}

		// apply the changes to the dive_table
		manager.commitChanges(dive_id, detailsEdit.number, detailsEdit.dateText, locationBox.editText, detailsEdit.gpsText, detailsEdit.durationText,
				      detailsEdit.depthText, detailsEdit.airtempText, detailsEdit.watertempText,
				      suitBox.editText, buddyBox.editText, divemasterBox.editText,
				      detailsEdit.weightText, detailsEdit.notesText, startpressure,
				      endpressure, usedGas, usedCyl,
				      detailsEdit.rating,
				      detailsEdit.visibility, state)
		Qt.inputMethod.hide()
		// now make sure we directly show the saved dive (this may be a new dive, or it may have moved)
		clearDetailsEdit()
	}

	height: editArea.height
	width: diveDetailsPage.width - diveDetailsPage.leftPadding - diveDetailsPage.rightPadding - Kirigami.Units.smallSpacing * 2
	ColumnLayout {
		id: editArea
		spacing: Kirigami.Units.smallSpacing
		width: parent.width

		GridLayout {
			id: editorDetails
			width: parent.width
			columns: 2
			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Dive number:")
			}
			SsrfTextField {
				id: txtNumber;
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Date:")
			}
			SsrfTextField {
				id: txtDate;
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}
			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Location:")
			}
			TemplateEditComboBox {
				id: locationBox
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					       manager.locationList : null
				onAccepted: {
					focus = false
					gpsText = manager.getGpsFromSiteName(editText)
				}
				onActivated: {
					focus = false
					gpsText = manager.getGpsFromSiteName(editText)
				}
			}

			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Coordinates:")
			}
			SsrfTextField {
				id: txtGps
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Use current\nGPS location:")
				visible: manager.locationServiceAvailable
			}
			SsrfCheckBox {
				id: checkboxGPS
				visible: manager.locationServiceAvailable
				onCheckedChanged: {
					if (checked)
						gpsText = manager.getCurrentPosition()
				}
			}
			Connections {
				target: manager
				onWaitingForPositionChanged: {
					gpsText = manager.getCurrentPosition()
				}
			}

			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Depth:")
			}
			SsrfTextField {
				id: txtDepth
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /[^-]*/ }
				flickable: detailsEditFlickable
			}
			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Duration:")
			}
			SsrfTextField {
				id: txtDuration
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /[^-]*/ }
				flickable: detailsEditFlickable
			}

			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Air Temp:")
			}
			SsrfTextField {
				id: txtAirTemp
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Water Temp:")
			}
			SsrfTextField {
				id: txtWaterTemp
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Suit:")
			}
			TemplateEditComboBox  {
				id: suitBox
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					       manager.suitList : null
			}

			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Buddy:")
			}
			TemplateEditComboBox {
				id: buddyBox
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					       manager.buddyList : null
			}

			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Divemaster:")
			}
			TemplateEditComboBox {
				id: divemasterBox
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					       manager.divemasterList : null
			}

			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Weight:")
			}
			SsrfTextField {
				id: txtWeight
				readOnly: text === "cannot edit multiple weight systems"
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}
// all cylinder info should be able to become dynamic instead of this blob of code.
// first cylinder
			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder1:")
			}
			TemplateComboBox {
				id: cylinderBox0
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					diveDetailsListView.currentItem.modelData.cylinderList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
			}

			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Gas mix:")
			}
			SsrfTextField {
				id: txtGasMix0
				text: usedGas[0] != null ? usedGas[0] : null
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
				flickable: detailsEditFlickable
			}

			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Start Pressure:")
			}
			SsrfTextField {
				id: txtStartPressure0
				text: startpressure[0] != null ? startpressure[0] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("End Pressure:")
			}
			SsrfTextField {
				id: txtEndPressure0
				text: endpressure[0] != null ? endpressure[0] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}
//second cylinder
			TemplateLabelSmall {
				visible: usedCyl[1] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder2:")
			}
			TemplateComboBox {
				visible: usedCyl[1] != null ? true : false
				id: cylinderBox1
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					diveDetailsListView.currentItem.modelData.cylinderList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
			}

			TemplateLabelSmall {
				visible: usedCyl[1] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Gas mix:")
			}
			SsrfTextField {
				visible: usedCyl[1] != null ? true : false
				id: txtGasMix1
				text: usedGas[1] != null ? usedGas[1] : null
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
				flickable: detailsEditFlickable
			}

			TemplateLabelSmall {
				visible: usedCyl[1] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Start Pressure:")
			}
			SsrfTextField {
				visible: usedCyl[1] != null ? true : false
				id: txtStartPressure1
				text: startpressure[1] != null ? startpressure[1] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			TemplateLabelSmall {
				visible: usedCyl[1] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("End Pressure:")
			}
			SsrfTextField {
				visible: usedCyl[1] != null ? true : false
				id: txtEndPressure1
				text: endpressure[1] != null ? endpressure[1] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}
// third cylinder
			TemplateLabelSmall {
				visible: usedCyl[2] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder3:")
			}
			TemplateComboBox {
				visible: usedCyl[2] != null ? true : false
				id: cylinderBox2
				currentIndex: find(usedCyl[2])
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					diveDetailsListView.currentItem.modelData.cylinderList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
			}

			TemplateLabelSmall {
				visible: usedCyl[2] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Gas mix:")
			}
			SsrfTextField {
				visible: usedCyl[2] != null ? true : false
				id: txtGasMix2
				text: usedGas[2] != null ? usedGas[2] : null
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
			}

			TemplateLabelSmall {
				visible: usedCyl[2] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Start Pressure:")
			}
			SsrfTextField {
				visible: usedCyl[2] != null ? true : false
				id: txtStartPressure2
				text: startpressure[2] != null ? startpressure[2] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			TemplateLabelSmall {
				visible: usedCyl[2] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("End Pressure:")
			}
			SsrfTextField {
				visible: usedCyl[2] != null ? true : false
				id: txtEndPressure2
				text: endpressure[2] != null ? endpressure[2] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}
// fourth cylinder
			TemplateLabelSmall {
				visible: usedCyl[3] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder4:")
			}
			TemplateComboBox {
				visible: usedCyl[3] != null ? true : false
				id: cylinderBox3
				currentIndex: find(usedCyl[3])
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					diveDetailsListView.currentItem.modelData.cylinderList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
			}

			TemplateLabelSmall {
				visible: usedCyl[3] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Gas mix:")
			}
			SsrfTextField {
				visible: usedCyl[3] != null ? true : false
				id: txtGasMix3
				text: usedGas[3] != null ? usedGas[3] : null
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
				flickable: detailsEditFlickable
			}

			TemplateLabelSmall {
				visible: usedCyl[3] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Start Pressure:")
			}
			SsrfTextField {
				visible: usedCyl[3] != null ? true : false
				id: txtStartPressure3
				text: startpressure[3] != null ? startpressure[3] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			TemplateLabelSmall {
				visible: usedCyl[3] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("End Pressure:")
			}
			SsrfTextField {
				visible: usedCyl[3] != null ? true : false
				id: txtEndPressure3
				text: endpressure[3] != null ? endpressure[3] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}
// fifth cylinder
			TemplateLabelSmall {
				visible: usedCyl[4] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder5:")
			}
			TemplateComboBox {
				visible: usedCyl[4] != null ? true : false
				id: cylinderBox4
				currentIndex: find(usedCyl[4])
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					diveDetailsListView.currentItem.modelData.cylinderList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
			}

			TemplateLabelSmall {
				visible: usedCyl[4] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Gas mix:")
			}
			SsrfTextField {
				visible: usedCyl[4] != null ? true : false
				id: txtGasMix4
				text: usedGas[4] != null ? usedGas[4] : null
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
				flickable: detailsEditFlickable
			}

			TemplateLabelSmall {
				visible: usedCyl[4] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Start Pressure:")
			}
			SsrfTextField {
				visible: usedCyl[4] != null ? true : false
				id: txtStartPressure4
				text: startpressure[4] != null ? startpressure[4] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			TemplateLabelSmall {
				visible: usedCyl[4] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("End Pressure:")
			}
			SsrfTextField {
				visible: usedCyl[4] != null ? true : false
				id: txtEndPressure4
				text: endpressure[4] != null ? endpressure[4] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Rating:")
			}
			TemplateSpinBox {
				id: ratingPicker
				from: 0
				to: 5
				value: rating
				onValueChanged: rating = value
			}

			TemplateLabelSmall {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Visibility:")
			}
			TemplateSpinBox {
				id: visibilityPicker
				from: 0
				to: 5
				value: visibility
				onValueChanged: visibility = value
			}

			TemplateLabelSmall {
				Layout.columnSpan: 2
				Layout.alignment: Qt.AlignLeft
				text: qsTr("Notes:")
			}
			Controls.TextArea {
				Layout.columnSpan: 2
				width: parent.width
				id: txtNotes
				textFormat: TextEdit.RichText
				focus: true
				color: subsurfaceTheme.textColor
				Layout.fillWidth: true
				Layout.fillHeight: true
				Layout.minimumHeight: Kirigami.Units.gridUnit * 6
				selectByMouse: true
				wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
				property bool firstTime: true
				property int visibleTop: detailsEditFlickable.contentY
				property int visibleBottom: visibleTop + detailsEditFlickable.height - 4 * Kirigami.Units.gridUnit
				onPressed: waitForKeyboard.start()
				onCursorRectangleChanged: {
					ensureVisible(y + cursorRectangle.y)
				}
				onContentHeightChanged: {
					console.log("content height changed")
				}

				// ensure that the y coordinate is inside the visible part of the detailsEditFlickable (our flickable)
				function ensureVisible(yDest) {
					if (yDest > visibleBottom)
						detailsEditFlickable.contentY += yDest - visibleBottom
					if (yDest < visibleTop)
						detailsEditFlickable.contentY -= visibleTop - yDest
				}

				// give the OS enough time to actually resize the flickable
				Timer {
					id: waitForKeyboard
					interval: 300 // 300ms seems like FOREVER
					onTriggered: {
						if (!Qt.inputMethod.visible) {
							if (txtNotes.firstTime) {
								txtNotes.firstTime = false
								restart()
							}
							return
						}
						// make sure at least half the Notes field is visible
						txtNotes.ensureVisible(txtNotes.y + txtNotes.cursorRectangle.y)
					}
				}
			}
		}
		Item {
			height: Kirigami.Units.gridUnit * 3
			width: height // just to make sure the spacer doesn't produce scrollbars, but also isn't null
		}
	}
}
