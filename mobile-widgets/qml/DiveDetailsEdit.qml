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
			usedGas[1] = txtGasMix0.text
			startpressure[1] = txtStartPressure1.text
			endpressure[1] = txtEndPressure1.text
		}
		if (usedCyl[2] != null) {
			usedCyl[2] = cylinderBox2.currentText
			usedGas[2] = txtGasMix0.text
			startpressure[2] = txtStartPressure2.text
			endpressure[2] = txtEndPressure2.text
		}
		if (usedCyl[3] != null) {
			usedCyl[3] = cylinderBox3.currentText
			usedGas[3] = txtGasMix0.text
			startpressure[3] = txtStartPressure3.text
			endpressure[3] = txtEndPressure3.text
		}
		if (usedCyl[4] != null) {
			usedCyl[4] = cylinderBox4.currentText
			usedGas[4] = txtGasMix0.text
			startpressure[4] = txtStartPressure4.text
			endpressure[4] = txtEndPressure4.text
		}

		// apply the changes to the dive_table
		manager.commitChanges(dive_id, detailsEdit.number, detailsEdit.dateText, locationBox.editText, detailsEdit.gpsText, detailsEdit.durationText,
				      detailsEdit.depthText, detailsEdit.airtempText, detailsEdit.watertempText,
				      suitBox.editText, buddyBox.editText, divemasterBox.editText,
				      detailsEdit.weightText, detailsEdit.notesText, startpressure,
				      endpressure, usedGas, usedCyl ,
				      detailsEdit.rating,
				      detailsEdit.visibility, state)
		// trigger the profile to be redrawn
		QMLProfile.diveId = dive_id

		// apply the changes to the dive detail view - since the edit could have changed the order
		// first make sure that we are looking at the correct dive - our model allows us to look
		// up the index based on the unique dive_id
		var newIdx = diveModel.getIdxForId(dive_id)
		diveDetailsListView.currentIndex = newIdx
		diveDetailsListView.currentItem.modelData.date = detailsEdit.dateText
		diveDetailsListView.currentItem.modelData.location = locationBox.currentText
		diveDetailsListView.currentItem.modelData.duration = detailsEdit.durationText
		diveDetailsListView.currentItem.modelData.depth = detailsEdit.depthText
		diveDetailsListView.currentItem.modelData.airtemp = detailsEdit.airtempText
		diveDetailsListView.currentItem.modelData.watertemp = detailsEdit.watertempText
		diveDetailsListView.currentItem.modelData.suit = suitBox.currentText
		diveDetailsListView.currentItem.modelData.buddy = buddyBox.currentText
		diveDetailsListView.currentItem.modelData.divemaster = divemasterBox.currentText
		diveDetailsListView.currentItem.modelData.notes = detailsEdit.notesText
		diveDetailsListView.currentItem.modelData.rating = detailsEdit.rating
		diveDetailsListView.currentItem.modelData.visibility = detailsEdit.visibility
		Qt.inputMethod.hide()
		// now make sure we directly show the saved dive (this may be a new dive, or it may have moved)
		showDiveIndex(newIdx)
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
			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Dive number:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				id: txtNumber;
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Date:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				id: txtDate;
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}
			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Location:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			Controls.ComboBox {
				id: locationBox
				editable: true
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					manager.locationList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
				onAccepted: {
					focus = false
					gpsText = manager.getGpsFromSiteName(editText)
				}
				onActivated: {
					focus = false
					gpsText = manager.getGpsFromSiteName(editText)
				}
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Coordinates:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				id: txtGps
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Use current\nGPS location:")
				visible: manager.locationServiceAvailable
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
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

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Depth:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				id: txtDepth
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /[^-]*/ }
				flickable: detailsEditFlickable
			}
			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Duration:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				id: txtDuration
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /[^-]*/ }
				flickable: detailsEditFlickable
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Air Temp:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				id: txtAirTemp
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Water Temp:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				id: txtWaterTemp
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Suit:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			Controls.ComboBox {
				id: suitBox
				editable: true
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					manager.suitList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
				onActivated: {
					focus = false
				}
				onAccepted: {
					focus = false
				}
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Buddy:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			Controls.ComboBox {
				id: buddyBox
				editable: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					manager.buddyList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
				onActivated: {
					focus = false
				}
				onAccepted: {
					focus = false
				}
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Divemaster:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			Controls.ComboBox {
				id: divemasterBox
				editable: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					manager.divemasterList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
				onActivated: {
					focus = false
				}
				onAccepted: {
					focus = false
				}
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Weight:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				id: txtWeight
				readOnly: text === "cannot edit multiple weight systems"
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}
// all cylinder info should be able to become dynamic instead of this blob of code.
// first cylinder
			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder1:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			Controls.ComboBox {
				id: cylinderBox0
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					diveDetailsListView.currentItem.modelData.cylinderList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Gas mix:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				id: txtGasMix0
				text: usedGas[0] != null ? usedGas[0] : null
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
				flickable: detailsEditFlickable
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Start Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				id: txtStartPressure0
				text: startpressure[0] != null ? startpressure[0] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("End Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				id: txtEndPressure0
				text: endpressure[0] != null ? endpressure[0] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}
//second cylinder
			Controls.Label {
				visible: usedCyl[1] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder2:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			Controls.ComboBox {
				visible: usedCyl[1] != null ? true : false
				id: cylinderBox1
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					diveDetailsListView.currentItem.modelData.cylinderList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
			}

			Controls.Label {
				visible: usedCyl[1] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Gas mix:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				visible: usedCyl[1] != null ? true : false
				id: txtGasMix1
				text: usedGas[1] != null ? usedGas[1] : null
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
				flickable: detailsEditFlickable
			}

			Controls.Label {
				visible: usedCyl[1] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Start Pressure:")
				color: subsurfaceTheme.textColor
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			SsrfTextField {
				visible: usedCyl[1] != null ? true : false
				id: txtStartPressure1
				text: startpressure[1] != null ? startpressure[1] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			Controls.Label {
				visible: usedCyl[1] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("End Pressure:")
				color: subsurfaceTheme.textColor
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			SsrfTextField {
				visible: usedCyl[1] != null ? true : false
				id: txtEndPressure1
				text: endpressure[1] != null ? endpressure[1] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}
// third cylinder
			Controls.Label {
				visible: usedCyl[2] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder3:")
				color: subsurfaceTheme.textColor
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.ComboBox {
				visible: usedCyl[2] != null ? true : false
				id: cylinderBox2
				currentIndex: find(usedCyl[2])
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					diveDetailsListView.currentItem.modelData.cylinderList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
			}

			Controls.Label {
				visible: usedCyl[2] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Gas mix:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				visible: usedCyl[2] != null ? true : false
				id: txtGasMix2
				text: usedGas[2] != null ? usedGas[2] : null
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
			}

			Controls.Label {
				visible: usedCyl[2] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Start Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				visible: usedCyl[2] != null ? true : false
				id: txtStartPressure2
				text: startpressure[2] != null ? startpressure[2] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			Controls.Label {
				visible: usedCyl[2] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("End Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				visible: usedCyl[2] != null ? true : false
				id: txtEndPressure2
				text: endpressure[2] != null ? endpressure[2] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}
// fourth cylinder
			Controls.Label {
				visible: usedCyl[3] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder4:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			Controls.ComboBox {
				visible: usedCyl[3] != null ? true : false
				id: cylinderBox3
				currentIndex: find(usedCyl[3])
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					diveDetailsListView.currentItem.modelData.cylinderList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
			}

			Controls.Label {
				visible: usedCyl[3] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Gas mix:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				visible: usedCyl[3] != null ? true : false
				id: txtGasMix3
				text: usedGas[3] != null ? usedGas[3] : null
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
				flickable: detailsEditFlickable
			}

			Controls.Label {
				visible: usedCyl[3] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Start Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				visible: usedCyl[3] != null ? true : false
				id: txtStartPressure3
				text: startpressure[3] != null ? startpressure[3] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			Controls.Label {
				visible: usedCyl[3] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("End Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				visible: usedCyl[3] != null ? true : false
				id: txtEndPressure3
				text: endpressure[3] != null ? endpressure[3] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}
// fifth cylinder
			Controls.Label {
				visible: usedCyl[4] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder5:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			Controls.ComboBox {
				visible: usedCyl[4] != null ? true : false
				id: cylinderBox4
				currentIndex: find(usedCyl[4])
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					diveDetailsListView.currentItem.modelData.cylinderList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
			}

			Controls.Label {
				visible: usedCyl[4] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Gas mix:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				visible: usedCyl[4] != null ? true : false
				id: txtGasMix4
				text: usedGas[4] != null ? usedGas[4] : null
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
				flickable: detailsEditFlickable
			}

			Controls.Label {
				visible: usedCyl[4] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Start Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				visible: usedCyl[4] != null ? true : false
				id: txtStartPressure4
				text: startpressure[4] != null ? startpressure[4] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			Controls.Label {
				visible: usedCyl[4] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("End Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			SsrfTextField {
				visible: usedCyl[4] != null ? true : false
				id: txtEndPressure4
				text: endpressure[4] != null ? endpressure[4] : null
				Layout.fillWidth: true
				flickable: detailsEditFlickable
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Rating:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			Controls.SpinBox {
				id: ratingPicker
				from: 0
				to: 5
				value: rating
				onValueChanged: rating = value
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Visibility:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			Controls.SpinBox {
				id: visibilityPicker
				from: 0
				to: 5
				value: visibility
				onValueChanged: visibility = value
			}

			Controls.Label {
				Layout.columnSpan: 2
				Layout.alignment: Qt.AlignLeft
				text: qsTr("Notes:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			Controls.TextArea {
				Layout.columnSpan: 2
				width: parent.width
				id: txtNotes
				textFormat: TextEdit.RichText
				focus: true
				Layout.fillWidth: true
				Layout.fillHeight: true
				Layout.minimumHeight: Kirigami.Units.gridUnit * 6
				selectByMouse: true
				wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
				property bool firstTime: true
				onPressed: waitForKeyboard.start()

				// we repeat the Timer / Function from the SsrfTextField here (no point really in creating a matching customized TextArea)
				function ensureVisible(yDest) {
					detailsEditFlickable.contentY = yDest
				}

				// give the OS enough time to actually resize the flickable
				Timer {
					id: waitForKeyboard
					interval: 300 // 300ms seems like FOREVER
					onTriggered: {
						if (!Qt.inputMethod.visible) {
							if (firstTime) {
								firstTime = false
								restart()
							}
							return
						}
						// make sure at least half the Notes field is visible
						if (txtNotes.y + txtNotes.height / 2 > detailsEditFlickable.contentY + detailsEditFlickable.height - 3 * Kirigami.Units.gridUnit || txtNotes.y < detailsEditFlickable.contentY)
							txtNotes.ensureVisible(Math.max(0, 3 * Kirigami.Units.gridUnit + txtNotes.y + txtNotes.height / 2 - detailsEditFlickable.height))
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
