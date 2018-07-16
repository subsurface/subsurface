// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2 as Controls
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.2 as Kirigami

Item {
	id: detailsEdit
	property int dive_id
	property int number
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
	property alias startpressureText0: txtStartPressure0.text
	property alias endpressureText0: txtEndPressure0.text
	property alias gasmixText0: txtGasMix0.text
	property alias gpsCheckbox: checkboxGPS.checked
	property alias suitModel: suitBox.model
	property alias divemasterModel: divemasterBox.model
	property alias buddyModel: buddyBox.model
	property alias cylinderModel: cylinderBox0.model
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
		diveDetailsPage.state = "view" // run the transition
		// join cylinder info from separate string into a list.
		if (usedCyl[0] != null) {
			usedCyl[0] = cylinderBox0.currentText
		}
		if (usedCyl[1] != null) {
			usedCyl[1] = cylinderBox1.currentText
		}
		if (usedCyl[2] != null) {
			usedCyl[2] = cylinderBox2.currentText
		}
		if (usedCyl[3] != null) {
			usedCyl[3] = cylinderBox3.currentText
		}
		if (usedCyl[4] != null) {
			usedCyl[4] = cylinderBox4.currentText
		}

		// apply the changes to the dive_table
		manager.commitChanges(dive_id, detailsEdit.dateText, locationBox.editText, detailsEdit.gpsText, detailsEdit.durationText,
				      detailsEdit.depthText, detailsEdit.airtempText, detailsEdit.watertempText,
				      suitBox.currentText != "" ? suitBox.currentText : suitBox.editText, buddyBox.editText,
				      divemasterBox.currentText != "" ? divemasterBox.currentText : divemasterBox.editText,
				      detailsEdit.weightText, detailsEdit.notesText, detailsEdit.startpressureText,
				      detailsEdit.endpressureText, detailsEdit.gasmixText, usedCyl ,
				      detailsEdit.rating,
				      detailsEdit.visibility)
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

			Kirigami.Heading {
				Layout.columnSpan: 2
				text: qsTr("Dive %1").arg(number)
			}
			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Date:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				id: txtDate;
				Layout.fillWidth: true
				onEditingFinished: {
					focus = false
				}
			}
			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Location:")
				font.pointSize: subsurfaceTheme.smallPointSize
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
			}
			Controls.TextField {
				id: txtGps
				Layout.fillWidth: true
				onEditingFinished: {
					focus = false
				}
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Use current\nGPS location:")
				visible: manager.locationServiceAvailable
				font.pointSize: subsurfaceTheme.smallPointSize
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
			}
			Controls.TextField {
				id: txtDepth
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /[^-]*/ }
				onEditingFinished: {
					focus = false
				}
			}
			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Duration:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				id: txtDuration
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /[^-]*/ }
				onEditingFinished: {
					focus = false
				}
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Air Temp:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				id: txtAirTemp
				Layout.fillWidth: true
				onEditingFinished: {
					focus = false
				}
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Water Temp:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				id: txtWaterTemp
				Layout.fillWidth: true
				onEditingFinished: {
					focus = false
				}
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Suit:")
				font.pointSize: subsurfaceTheme.smallPointSize
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
			}
			Controls.TextField {
				id: txtWeight
				readOnly: text === "cannot edit multiple weight systems"
				Layout.fillWidth: true
				onEditingFinished: {
					focus = false
				}
			}
// all cylinder info should be able to become dynamic instead of this blob of code.
// first cylinder
			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder1:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.ComboBox {
				id: cylinderBox0
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					diveDetailsListView.currentItem.modelData.dive.cylinderList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Gas mix:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				id: txtGasMix0
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
				onEditingFinished: {
					focus = false
				}
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Start Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				id: txtStartPressure0
				Layout.fillWidth: true
				onEditingFinished: {
					focus = false
				}
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("End Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				id: txtEndPressure0
				Layout.fillWidth: true
				onEditingFinished: {
					focus = false
				}
			}
//second cylinder
			Controls.Label {
				visible: usedCyl[1] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder2:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.ComboBox {
				visible: usedCyl[1] != null ? true : false
				id: cylinderBox1
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					diveDetailsListView.currentItem.modelData.dive.cylinderList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
			}

			Controls.Label {
				visible: usedCyl[1] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Gas mix:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				visible: usedCyl[1] != null ? true : false
				id: txtGasMix1
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
				onEditingFinished: {
					focus = false
				}
			}

			Controls.Label {
				visible: usedCyl[1] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Start Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				visible: usedCyl[1] != null ? true : false
				id: txtStartPressure1
				Layout.fillWidth: true
				onEditingFinished: {
					focus = false
				}
			}

			Controls.Label {
				visible: usedCyl[1] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("End Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				visible: usedCyl[1] != null ? true : false
				id: txtEndPressure1
				Layout.fillWidth: true
				onEditingFinished: {
					focus = false
				}
			}
// third cylinder
			Controls.Label {
				visible: usedCyl[2] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder3:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.ComboBox {
				visible: usedCyl[2] != null ? true : false
				id: cylinderBox2
				currentIndex: find(usedCyl[2])
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					diveDetailsListView.currentItem.modelData.dive.cylinderList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
			}

			Controls.Label {
				visible: usedCyl[2] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Gas mix:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				visible: usedCyl[2] != null ? true : false
				id: txtGasMix2
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
				onEditingFinished: {
					focus = false
				}
			}

			Controls.Label {
				visible: usedCyl[2] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Start Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				visible: usedCyl[2] != null ? true : false
				id: txtStartPressure2
				Layout.fillWidth: true
				onEditingFinished: {
					focus = false
				}
			}

			Controls.Label {
				visible: usedCyl[2] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("End Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				visible: usedCyl[2] != null ? true : false
				id: txtEndPressure2
				Layout.fillWidth: true
				onEditingFinished: {
					focus = false
				}
			}
// fourth cylinder
			Controls.Label {
				visible: usedCyl[3] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder4:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.ComboBox {
				visible: usedCyl[3] != null ? true : false
				id: cylinderBox3
				currentIndex: find(usedCyl[3])
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					diveDetailsListView.currentItem.modelData.dive.cylinderList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
			}

			Controls.Label {
				visible: usedCyl[3] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Gas mix:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				visible: usedCyl[3] != null ? true : false
				id: txtGasMix3
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
				onEditingFinished: {
					focus = false
				}
			}

			Controls.Label {
				visible: usedCyl[3] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Start Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				visible: usedCyl[3] != null ? true : false
				id: txtStartPressure3
				Layout.fillWidth: true
				onEditingFinished: {
					focus = false
				}
			}

			Controls.Label {
				visible: usedCyl[3] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("End Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				visible: usedCyl[3] != null ? true : false
				id: txtEndPressure3
				Layout.fillWidth: true
				onEditingFinished: {
					focus = false
				}
			}
// fifth cylinder
			Controls.Label {
				visible: usedCyl[4] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder5:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.ComboBox {
				visible: usedCyl[4] != null ? true : false
				id: cylinderBox4
				currentIndex: find(usedCyl[4])
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					diveDetailsListView.currentItem.modelData.dive.cylinderList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
			}

			Controls.Label {
				visible: usedCyl[4] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Gas mix:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				visible: usedCyl[4] != null ? true : false
				id: txtGasMix4
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
				onEditingFinished: {
					focus = false
				}
			}

			Controls.Label {
				visible: usedCyl[4] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("Start Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				visible: usedCyl[4] != null ? true : false
				id: txtStartPressure4
				Layout.fillWidth: true
				onEditingFinished: {
					focus = false
				}
			}

			Controls.Label {
				visible: usedCyl[4] != null ? true : false
				Layout.alignment: Qt.AlignRight
				text: qsTr("End Pressure:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.TextField {
				visible: usedCyl[4] != null ? true : false
				id: txtEndPressure4
				Layout.fillWidth: true
				onEditingFinished: {
					focus = false
				}
			}

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Rating:")
				font.pointSize: subsurfaceTheme.smallPointSize
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
			}
		}
		Item {
			height: Kirigami.Units.gridUnit * 3
			width: height // just to make sure the spacer doesn't produce scrollbars, but also isn't null
		}
	}
}
