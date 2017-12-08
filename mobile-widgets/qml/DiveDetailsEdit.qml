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
	property alias locationText: txtLocation.editText
	property alias gpsText: txtGps.text
	property alias airtempText: txtAirTemp.text
	property alias watertempText: txtWaterTemp.text
	property alias suitIndex: suitBox.currentIndex
	property alias suitText: suitBox.editText
	property alias buddyIndex: buddyBox.currentIndex
	property alias buddyText: buddyBox.editText
	property alias divemasterIndex: divemasterBox.currentIndex
	property alias divemasterText: divemasterBox.editText
	property alias cylinderIndex: cylinderBox.currentIndex
	property alias cylinderText: cylinderBox.editText
	property alias notesText: txtNotes.text
	property alias durationText: txtDuration.text
	property alias depthText: txtDepth.text
	property alias weightText: txtWeight.text
	property alias startpressureText: txtStartPressure.text
	property alias endpressureText: txtEndPressure.text
	property alias gasmixText: txtGasMix.text
	property alias gpsCheckbox: checkboxGPS.checked
	property alias suitModel: suitBox.model
	property alias divemasterModel: divemasterBox.model
	property alias buddyModel: buddyBox.model
	property alias cylinderModel: cylinderBox.model
	property alias locationModel: txtLocation.model
	property int rating
	property int visibility

	function saveData() {
		diveDetailsPage.state = "view" // run the transition
		// apply the changes to the dive_table
		manager.commitChanges(dive_id, detailsEdit.dateText, detailsEdit.locationText, detailsEdit.gpsText, detailsEdit.durationText,
				      detailsEdit.depthText, detailsEdit.airtempText, detailsEdit.watertempText,
				      suitBox.currentText != "" ? suitBox.currentText : suitBox.editText,
				      buddyBox.currentText != "" ? buddyBox.currentText : buddyBox.editText,
				      divemasterBox.currentText != "" ? divemasterBox.currentText : divemasterBox.editText,
				      detailsEdit.weightText, detailsEdit.notesText, detailsEdit.startpressureText,
				      detailsEdit.endpressureText, detailsEdit.gasmixText,
				      cylinderBox.currentText != "" ? cylinderBox.currentText : cylinderBox.editText,
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
		diveDetailsListView.currentItem.modelData.location = detailsEdit.locationText
		diveDetailsListView.currentItem.modelData.duration = detailsEdit.durationText
		diveDetailsListView.currentItem.modelData.depth = detailsEdit.depthText
		diveDetailsListView.currentItem.modelData.airtemp = detailsEdit.airtempText
		diveDetailsListView.currentItem.modelData.watertemp = detailsEdit.watertempText
		diveDetailsListView.currentItem.modelData.suit = suitBox.currentText
		diveDetailsListView.currentItem.modelData.buddy = buddyBox.currentText
		diveDetailsListView.currentItem.modelData.divemaster = divemasterBox.currentText
		diveDetailsListView.currentItem.modelData.cylinder = cylinderBox.currentText
		diveDetailsListView.currentItem.modelData.notes = detailsEdit.notesText
		diveDetailsListView.currentItem.modelData.rating = detailsEdit.rating
		diveDetailsListView.currentItem.modelData.visibility = detailsEdit.visibility
		Qt.inputMethod.hide()
		// now make sure we directly show the saved dive (this may be a new dive, or it may have moved)
		showDiveIndex(newIdx)
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
				id: txtLocation
				editable: true
				flat: true
				model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
					diveDetailsListView.currentItem.modelData.dive.locationList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
				onAccepted: {
					gpsText = manager.getGpsFromSiteName(text)
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
					diveDetailsListView.currentItem.modelData.dive.suitList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
				onActivated: {
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
					diveDetailsListView.currentItem.modelData.dive.buddyList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
				onActivated: {
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
					diveDetailsListView.currentItem.modelData.dive.divemasterList : null
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
				onActivated: {
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

			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.ComboBox {
				id: cylinderBox
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
				id: txtGasMix
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
				id: txtStartPressure
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
				id: txtEndPressure
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
