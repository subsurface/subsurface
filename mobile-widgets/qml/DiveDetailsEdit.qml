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
	property string gpsText
	property alias airtempText: txtAirTemp.text
	property alias watertempText: txtWaterTemp.text
	property alias suitText: txtSuit.text
	property alias buddyText: txtBuddy.text
	property alias divemasterText: txtDiveMaster.text
	property alias notesText: txtNotes.text
	property alias durationText: txtDuration.text
	property alias depthText: txtDepth.text
	property alias weightText: txtWeight.text
	property alias startpressureText: txtStartPressure.text
	property alias endpressureText: txtEndPressure.text
	property alias gasmixText: txtGasMix.text

	function saveData() {
		// apply the changes to the dive_table
		manager.commitChanges(dive_id, detailsEdit.dateText, detailsEdit.locationText, detailsEdit.gpsText, detailsEdit.durationText,
				      detailsEdit.depthText, detailsEdit.airtempText, detailsEdit.watertempText, detailsEdit.suitText,
				      detailsEdit.buddyText, detailsEdit.divemasterText, detailsEdit.weightText, detailsEdit.notesText,
				      detailsEdit.startpressureText, detailsEdit.endpressureText, detailsEdit.gasmixText)
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
		diveDetailsListView.currentItem.modelData.suit = detailsEdit.suitText
		diveDetailsListView.currentItem.modelData.buddy = detailsEdit.buddyText
		diveDetailsListView.currentItem.modelData.divemaster = detailsEdit.divemasterText
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
				text: "Dive " + number
			}
			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: "Date:"
			}
			StyledTextField {
				id: txtDate;
				Layout.fillWidth: true
			}
			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: "Location:"
			}
			StyledTextField {
				id: txtLocation;
				Layout.fillWidth: true
			}

			// we should add a checkbox here that allows the user
			// to add the current location as the dive location
			// (think of someone adding a dive while on the boat or
			//  at the dive site)
			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: "Use current\nGPS location:"
			}
			CheckBox {
				id: checkboxGPS
				onCheckedChanged: {
					if (checked)
						gpsText = manager.getCurrentPosition()
				}
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: "Depth:"
			}
			StyledTextField {
				id: txtDepth
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /[^-]*/ }
			}
			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: "Duration:"
			}
			StyledTextField {
				id: txtDuration
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /[^-]*/ }
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: "Air Temp:"
			}
			StyledTextField {
				id: txtAirTemp
				Layout.fillWidth: true
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: "Water Temp:"
			}
			StyledTextField {
				id: txtWaterTemp
				Layout.fillWidth: true
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: "Suit:"
			}
			StyledTextField {
				id: txtSuit
				Layout.fillWidth: true
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: "Buddy:"
			}
			StyledTextField {
				id: txtBuddy
				Layout.fillWidth: true
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: "Dive Master:"
			}
			StyledTextField {
				id: txtDiveMaster
				Layout.fillWidth: true
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: "Weight:"
			}
			StyledTextField {
				id: txtWeight
				fixed: text === "cannot edit multiple weight systems"
				Layout.fillWidth: true
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: "Gas mix:"
			}
			StyledTextField {
				id: txtGasMix
				fixed: (text == "cannot edit multiple gases" ? true : false)
				Layout.fillWidth: true
				validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: "Start Pressure:"
			}
			StyledTextField {
				id: txtStartPressure
				fixed: (text == "cannot edit multiple cylinders" ? true : false)
				Layout.fillWidth: true
			}

			Kirigami.Label {
				Layout.alignment: Qt.AlignRight
				text: "End Pressure:"
			}
			StyledTextField {
				id: txtEndPressure
				readOnly: (text == "cannot edit multiple cylinders" ? true : false)
				Layout.fillWidth: true
			}

			Kirigami.Label {
				Layout.columnSpan: 2
				Layout.alignment: Qt.AlignLeft
				text: "Notes:"
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
