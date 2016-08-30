import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 1.0 as Kirigami

Item {
	id: detailsEdit
	property int dive_id
	property int number
	property alias dateText: txtDate.text
	property alias locationText: txtLocation.text
	property alias gpsText: txtGps.text
	property alias airtempText: txtAirTemp.text
	property alias watertempText: txtWaterTemp.text
	property alias suitIndex: suitBox.currentIndex
	property alias buddyIndex: buddyBox.currentIndex
	property alias divemasterIndex: divemasterBox.currentIndex
	property alias cylinderIndex: cylinderBox.currentIndex
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

	function saveData() {
		// apply the changes to the dive_table
		manager.commitChanges(dive_id, detailsEdit.dateText, detailsEdit.locationText, detailsEdit.gpsText, detailsEdit.durationText,
				      detailsEdit.depthText, detailsEdit.airtempText, detailsEdit.watertempText, suitBox.editText, buddyBox.editText,
				      divemasterBox.editText, detailsEdit.weightText, detailsEdit.notesText, detailsEdit.startpressureText,
				      detailsEdit.endpressureText, detailsEdit.gasmixText, cylinderBox.editText)
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
		diveDetailsPage.state = "view"
		Qt.inputMethod.hide()
		// now make sure we directly show the saved dive (this may be a new dive, or it may have moved)
		showDiveIndex(newIdx)
	}

	height: editArea.height
	width: diveDetailsPage.width - diveDetailsPage.leftPadding - diveDetailsPage.rightPadding
	ColumnLayout {
		id: editArea
		spacing: Kirigami.Units.smallSpacing
		width: parent.width - 2 * Kirigami.Units.gridUnit

		GridLayout {
			id: editorDetails
			width: parent.width
			columns: 2

			Kirigami.Heading {
				Layout.columnSpan: 2
				text: qsTr("Dive %1").arg(number)
			}
			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Date:")
			}
			StyledTextField {
				id: txtDate;
				Layout.fillWidth: true
			}
			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Location:")
			}
			StyledTextField {
				id: txtLocation;
				Layout.fillWidth: true
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Coordinates:")
			}
			StyledTextField {
				id: txtGps
				Layout.fillWidth: true
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Use current\nGPS location:")
				visible: manager.locationServiceAvailable
			}
			CheckBox {
				id: checkboxGPS
				visible: manager.locationServiceAvailable
				onCheckedChanged: {
					if (checked)
						gpsText = manager.getCurrentPosition()
				}
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Depth:")
			}
			StyledTextField {
				id: txtDepth
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /[^-]*/ }
			}
			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Duration:")
			}
			StyledTextField {
				id: txtDuration
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /[^-]*/ }
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Air Temp:")
			}
			StyledTextField {
				id: txtAirTemp
				Layout.fillWidth: true
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Water Temp:")
			}
			StyledTextField {
				id: txtWaterTemp
				Layout.fillWidth: true
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Suit:")
			}
			ComboBox {
				id: suitBox
				editable: true
				model: diveDetailsListView.currentItem.modelData.dive.suitList
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
				style: ComboBoxStyle {
					dropDownButtonWidth: 0
				}
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Buddy:")
			}
			ComboBox {
				id: buddyBox
				editable: true
				model: diveDetailsListView.currentItem.modelData.dive.buddyList
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
				style: ComboBoxStyle {
					dropDownButtonWidth: 0
				}
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Dive Master:")
			}
			ComboBox {
				id: divemasterBox
				editable: true
				model: diveDetailsListView.currentItem.modelData.dive.divemasterList
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
				style: ComboBoxStyle {
					dropDownButtonWidth: 0
				}
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Weight:")
			}
			StyledTextField {
				id: txtWeight
				fixed: text === "cannot edit multiple weight systems"
				Layout.fillWidth: true
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder:")
			}
			ComboBox {
				id: cylinderBox
				editable: true
				model: diveDetailsListView.currentItem.modelData.dive.cylinderList
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
				style: ComboBoxStyle {
					dropDownButtonWidth: 0
				}
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Gas mix:")
			}
			StyledTextField {
				id: txtGasMix
				fixed: (text == "cannot edit multiple gases" ? true : false)
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Start Pressure:")
			}
			StyledTextField {
				id: txtStartPressure
				fixed: (text == "cannot edit multiple cylinders" ? true : false)
				Layout.fillWidth: true
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("End Pressure:")
			}
			StyledTextField {
				id: txtEndPressure
				readOnly: (text == "cannot edit multiple cylinders" ? true : false)
				Layout.fillWidth: true
			}

			Kirigami.Label {
				Layout.columnSpan: 2
				Layout.alignment: Qt.AlignLeft
				text: qsTr("Notes:")
			}
			TextArea {
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
