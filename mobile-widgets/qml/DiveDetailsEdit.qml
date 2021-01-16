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
		focusReset()
		var state =  diveDetailsPage.state
		diveDetailsPage.state = "view" // run the transition
		// join cylinder info from separate string into a list.
		if (usedCyl[0] !== undefined) {
			usedCyl[0] = cylinderBox0.currentText
			usedGas[0] = txtGasMix0.text
			startpressure[0] = txtStartPressure0.text
			endpressure[0] = txtEndPressure0.text
		}
		if (usedCyl[1] !== undefined) {
			usedCyl[1] = cylinderBox1.currentText
			usedGas[1] = txtGasMix1.text
			startpressure[1] = txtStartPressure1.text
			endpressure[1] = txtEndPressure1.text
		}
		if (usedCyl[2] !== undefined) {
			usedCyl[2] = cylinderBox2.currentText
			usedGas[2] = txtGasMix2.text
			startpressure[2] = txtStartPressure2.text
			endpressure[2] = txtEndPressure2.text
		}
		if (usedCyl[3] !== undefined) {
			usedCyl[3] = cylinderBox3.currentText
			usedGas[3] = txtGasMix3.text
			startpressure[3] = txtStartPressure3.text
			endpressure[3] = txtEndPressure3.text
		}
		if (usedCyl[4] !== undefined) {
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

	height: editArea.height + Kirigami.Units.gridUnit * 3
	width: diveDetailsPage.width - diveDetailsPage.leftPadding - diveDetailsPage.rightPadding - Kirigami.Units.smallSpacing * 2
	Item {
		// there is a maximum width above which this becomes less pleasant to use. 42 gridUnits
		// allows for two of the large drop downs or four of the text fields or all of a cylinder
		// to be in one row. More just doesn't look good
		width: Math.min(parent.width - Kirigami.Units.smallSpacing, Kirigami.Units.gridUnit * 42)
		// weird way to create a little space from the left edge - but I can't do a margin here
		x: Kirigami.Units.smallSpacing
		Flow {
			id: editArea
			// with larger fonts we need more space, or things look too crowded
			spacing: subsurfaceTheme.currentScale > 1.0 ? 1.5 * Kirigami.Units.largeSpacing : Kirigami.Units.largeSpacing
			width: parent.width
			flow: GridLayout.LeftToRight
			RowLayout {
				TemplateLabelSmall {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 4
					horizontalAlignment: Text.AlignRight
					text: qsTr("Date/Time:")
				}
				SsrfTextField {
					id: txtDate;
					Layout.preferredWidth: Kirigami.Units.gridUnit * 10
					flickable: detailsEditFlickable
				}
			}
			RowLayout {
				TemplateLabelSmall {
					horizontalAlignment: Text.AlignRight
					Layout.preferredWidth: Kirigami.Units.gridUnit * 4
					text: qsTr("Dive number:")
				}
				SsrfTextField {
					id: txtNumber;
					Layout.preferredWidth: Kirigami.Units.gridUnit * 3
					flickable: detailsEditFlickable
				}
				Item {
					// if date and dive number are on the same line, don't have the Depth behind them
					// to ensure that we add an element that fills enough of the line that the flow
					// will not pull the next element up
					visible: editArea.width > Kirigami.Units.gridUnit * 27
					Layout.preferredWidth: editArea.width - Kirigami.Units.gridUnit * 26
				}
			}
			RowLayout {
				TemplateLabelSmall {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 4
					horizontalAlignment: Text.AlignRight
					text: qsTr("Depth:")
				}
				SsrfTextField {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 3
					id: txtDepth
					validator: RegExpValidator { regExp: /[^-]*/ }
					flickable: detailsEditFlickable
				}
			}
			RowLayout {
				TemplateLabelSmall {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 4
					horizontalAlignment: Text.AlignRight
					text: qsTr("Duration:")
				}
				SsrfTextField {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 3
					id: txtDuration
					validator: RegExpValidator { regExp: /[^-]*/ }
					flickable: detailsEditFlickable
				}

			}
			RowLayout {
				TemplateLabelSmall {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 4
					horizontalAlignment: Text.AlignRight
					text: qsTr("Air Temp:")
				}
				SsrfTextField {
					id: txtAirTemp
					Layout.preferredWidth: Kirigami.Units.gridUnit * 3
					flickable: detailsEditFlickable
				}

			}
			RowLayout {
				TemplateLabelSmall {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 4
					horizontalAlignment: Text.AlignRight
					text: qsTr("Water Temp:")
				}
				SsrfTextField {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 3
					id: txtWaterTemp
					flickable: detailsEditFlickable
				}
			}
			RowLayout {
				width: Kirigami.Units.gridUnit * 20
				TemplateLabelSmall {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 4
					horizontalAlignment: Text.AlignRight
					text: qsTr("Location:")
				}
				TemplateEditComboBox {
					// this one needs more space
					id: locationBox
					flickable: detailsEditFlickable
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
			}
			RowLayout {
				spacing: Kirigami.Units.smallSpacing
				Layout.preferredWidth: Kirigami.Units.gridUnit * 20
				TemplateLabelSmall {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 4
					horizontalAlignment: Text.AlignRight
					text: qsTr("Coordinates:")
				}
				SsrfTextField {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 16
					id: txtGps
					flickable: detailsEditFlickable
				}
			}
			RowLayout {
				width: manager.locationServiceAvailable ? Kirigami.Units.gridUnit * 12 : 0
				TemplateLabelSmall {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 6
					horizontalAlignment: Text.AlignRight
					text: qsTr("Use current\nGPS location:")
					visible: manager.locationServiceAvailable
				}
				TemplateCheckBox {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 6
					id: checkboxGPS
					visible: manager.locationServiceAvailable
					onCheckedChanged: {
						if (checked)
							gpsText = manager.getCurrentPosition()
					}
				}
			}

			Connections {
				target: manager
				onWaitingForPositionChanged: {
					gpsText = manager.getCurrentPosition()
					manager.appendTextToLog("received updated position info " + gpsText)
				}
			}
			RowLayout {
				width: Kirigami.Units.gridUnit * 20
				TemplateLabelSmall {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 4
					horizontalAlignment: Text.AlignRight
					text: qsTr("Suit:")
				}
				TemplateEditComboBox  {
					id: suitBox
					flickable: detailsEditFlickable
					model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
						       manager.suitList : null
				}
			}
			RowLayout {
				width: Kirigami.Units.gridUnit * 20
				TemplateLabelSmall {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 4
					horizontalAlignment: Text.AlignRight
					text: qsTr("Buddy:")
				}
				TemplateEditComboBox {
					id: buddyBox
					flickable: detailsEditFlickable
					model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
						       manager.buddyList : null
				}
			}
			RowLayout {
				width: Kirigami.Units.gridUnit * 20
				TemplateLabelSmall {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 4
					horizontalAlignment: Text.AlignRight
					text: qsTr("Divemaster:")
				}
				TemplateEditComboBox {
					id: divemasterBox
					flickable: detailsEditFlickable
					model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
						       manager.divemasterList : null
				}
			}


			RowLayout {
				width: Kirigami.Units.gridUnit * 16
				TemplateLabelSmall {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 4
					horizontalAlignment: Text.AlignRight
					text: qsTr("Weight:")
				}
				SsrfTextField {
					id: txtWeight
					Layout.preferredWidth: Kirigami.Units.gridUnit * 12
					readOnly: text === "cannot edit multiple weight systems"
					flickable: detailsEditFlickable
				}
			}

			// all cylinder info should be able to become dynamic instead of this blob of code.
			// first cylinder
			Flow {
				width: parent.width
				RowLayout {
					width: Kirigami.Units.gridUnit * 12
					id: cb1
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 4
						horizontalAlignment: Text.AlignRight
						text: qsTr("Cylinder1:")
					}
					TemplateComboBox {
						id: cylinderBox0
						flickable: detailsEditFlickable
						flat: true
						model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
							       diveDetailsListView.currentItem.modelData.cylinderList : null
						inputMethodHints: Qt.ImhNoPredictiveText
					}
				}
				RowLayout {
					height: cb1.height
					width: Kirigami.Units.gridUnit * 8
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 4
						horizontalAlignment: Text.AlignRight
						text: qsTr("Gas mix:")
					}
					SsrfTextField {
						id: txtGasMix0
						text: usedGas[0] !== undefined ? usedGas[0] : null
						Layout.fillWidth: true
						validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
						flickable: detailsEditFlickable
					}
				}
				RowLayout {
					height: cb1.height
					width: Kirigami.Units.gridUnit * 10
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 6
						horizontalAlignment: Text.AlignRight
						text: qsTr("Start Pressure:")
					}
					SsrfTextField {
						id: txtStartPressure0
						text: startpressure[0] !== undefined ? startpressure[0] : null
						Layout.fillWidth: true
						flickable: detailsEditFlickable
					}
				}
				RowLayout {
					height: cb1.height
					width: Kirigami.Units.gridUnit * 10
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 6
						horizontalAlignment: Text.AlignRight
						text: qsTr("End Pressure:")
					}
					SsrfTextField {
						id: txtEndPressure0
						text: endpressure[0] !== undefined ? endpressure[0] : null
						Layout.fillWidth: true
						flickable: detailsEditFlickable
					}
				}
			}
			//second cylinder
			Flow {
				width: parent.width
				visible: usedCyl[1] !== undefined ? true : false
				RowLayout {
					id: cb2
					width: Kirigami.Units.gridUnit * 12
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 4
						horizontalAlignment: Text.AlignRight
						text: qsTr("Cylinder2:")
					}
					TemplateComboBox {
						id: cylinderBox1
						flickable: detailsEditFlickable
						flat: true
						model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
							       diveDetailsListView.currentItem.modelData.cylinderList : null
						inputMethodHints: Qt.ImhNoPredictiveText
					}
				}
				RowLayout {
					width: Kirigami.Units.gridUnit * 8
					height: cb2.height
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 4
						horizontalAlignment: Text.AlignRight
						text: qsTr("Gas mix:")
					}
					SsrfTextField {
						id: txtGasMix1
						text: usedGas[1] !== undefined ? usedGas[1] : null
						Layout.fillWidth: true
						validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
						flickable: detailsEditFlickable
					}
				}
				RowLayout {
					width: Kirigami.Units.gridUnit * 10
					height: cb2.height
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 6
						horizontalAlignment: Text.AlignRight
						text: qsTr("Start Pressure:")
					}
					SsrfTextField {
						id: txtStartPressure1
						text: startpressure[1] !== undefined ? startpressure[1] : null
						Layout.fillWidth: true
						flickable: detailsEditFlickable
					}
				}
				RowLayout {
					width: Kirigami.Units.gridUnit * 10
					height: cb2.height
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 6
						horizontalAlignment: Text.AlignRight
						text: qsTr("End Pressure:")
					}
					SsrfTextField {
						id: txtEndPressure1
						text: endpressure[1] !== undefined ? endpressure[1] : null
						Layout.fillWidth: true
						flickable: detailsEditFlickable
					}
				}
			}
			// third cylinder
			Flow {
				width: parent.width
				visible: usedCyl[2] !== undefined ? true : false
				RowLayout {
					id: cb3
					width: Kirigami.Units.gridUnit * 12
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 4
						horizontalAlignment: Text.AlignRight
						text: qsTr("Cylinder3:")
					}
					TemplateComboBox {
						id: cylinderBox2
						flickable: detailsEditFlickable
						currentIndex: find(usedCyl[2])
						flat: true
						model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
							       diveDetailsListView.currentItem.modelData.cylinderList : null
						inputMethodHints: Qt.ImhNoPredictiveText
					}
				}
				RowLayout {
					width: Kirigami.Units.gridUnit * 8
					height: cb3.height
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 4
						horizontalAlignment: Text.AlignRight
						text: qsTr("Gas mix:")
					}
					SsrfTextField {
						id: txtGasMix2
						text: usedGas[2] !== undefined ? usedGas[2] : null
						Layout.fillWidth: true
						validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
						flickable: detailsEditFlickable
					}
				}
				RowLayout {
					width: Kirigami.Units.gridUnit * 10
					height: cb3.height
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 6
						horizontalAlignment: Text.AlignRight
						text: qsTr("Start Pressure:")
					}
					SsrfTextField {
						id: txtStartPressure2
						text: startpressure[2] !== undefined ? startpressure[2] : null
						Layout.fillWidth: true
						flickable: detailsEditFlickable
					}
				}
				RowLayout {
					width: Kirigami.Units.gridUnit * 10
					height: cb3.height
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 6
						horizontalAlignment: Text.AlignRight
						text: qsTr("End Pressure:")
					}
					SsrfTextField {
						id: txtEndPressure2
						text: endpressure[2] !== undefined ? endpressure[2] : null
						Layout.fillWidth: true
						flickable: detailsEditFlickable
					}
				}
			}
			// fourth cylinder
			Flow {
				width: parent.width
				visible: usedCyl[3] !== undefined ? true : false
				RowLayout {
					id: cb4
					width: Kirigami.Units.gridUnit * 12
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 4
						horizontalAlignment: Text.AlignRight
						text: qsTr("Cylinder4:")
					}
					TemplateComboBox {
						id: cylinderBox3
						flickable: detailsEditFlickable
						currentIndex: find(usedCyl[3])
						flat: true
						model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
							       diveDetailsListView.currentItem.modelData.cylinderList : null
						inputMethodHints: Qt.ImhNoPredictiveText
					}

				}
				RowLayout {
					width: Kirigami.Units.gridUnit * 8
					height: cb4.height
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 4
						horizontalAlignment: Text.AlignRight
						text: qsTr("Gas mix:")
					}
					SsrfTextField {
						id: txtGasMix3
						text: usedGas[3] !== undefined ? usedGas[3] : null
						Layout.fillWidth: true
						validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
						flickable: detailsEditFlickable
					}

				}
				RowLayout {
					width: Kirigami.Units.gridUnit * 10
					height: cb4.height
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 6
						horizontalAlignment: Text.AlignRight
						text: qsTr("Start Pressure:")
					}
					SsrfTextField {
						id: txtStartPressure3
						text: startpressure[3] !== undefined ? startpressure[3] : null
						Layout.fillWidth: true
						flickable: detailsEditFlickable
					}

				}
				RowLayout {
					width: Kirigami.Units.gridUnit * 10
					height: cb4.height
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 6
						horizontalAlignment: Text.AlignRight
						text: qsTr("End Pressure:")
					}
					SsrfTextField {
						id: txtEndPressure3
						text: endpressure[3] !== undefined ? endpressure[3] : null
						Layout.fillWidth: true
						flickable: detailsEditFlickable
					}
				}
			}
			// fifth cylinder
			Flow {
				width: parent.width
				visible: usedCyl[4] !== undefined ? true : false
				RowLayout {
					id: cb5
					width: Kirigami.Units.gridUnit * 12
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 4
						horizontalAlignment: Text.AlignRight
						text: qsTr("Cylinder5:")
					}
					TemplateComboBox {
						id: cylinderBox4
						flickable: detailsEditFlickable
						currentIndex: find(usedCyl[4])
						flat: true
						model: diveDetailsListView.currentItem && diveDetailsListView.currentItem.modelData !== null ?
							       diveDetailsListView.currentItem.modelData.cylinderList : null
						inputMethodHints: Qt.ImhNoPredictiveText
						Layout.fillWidth: true
					}

				}
				RowLayout {
					width: Kirigami.Units.gridUnit * 8
					height: cb5.height
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 4
						horizontalAlignment: Text.AlignRight
						text: qsTr("Gas mix:")
					}
					SsrfTextField {
						id: txtGasMix4
						text: usedGas[4] !== undefined ? usedGas[4] : null
						Layout.fillWidth: true
						validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
						flickable: detailsEditFlickable
					}

				}
				RowLayout {
					width: Kirigami.Units.gridUnit * 10
					height: cb5.height
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 6
						horizontalAlignment: Text.AlignRight
						text: qsTr("Start Pressure:")
					}
					SsrfTextField {
						id: txtStartPressure4
						text: startpressure[4] !== undefined ? startpressure[4] : null
						Layout.fillWidth: true
						flickable: detailsEditFlickable
					}

				}
				RowLayout {
					width: Kirigami.Units.gridUnit * 10
					height: cb5.height
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 6
						horizontalAlignment: Text.AlignRight
						text: qsTr("End Pressure:")
					}
					SsrfTextField {
						id: txtEndPressure4
						text: endpressure[4] !== undefined ? endpressure[4] : null
						Layout.fillWidth: true
						flickable: detailsEditFlickable
					}
				}
			}
			// rating / visibility
			RowLayout {
				width: parent.width

				RowLayout {
					width: Kirigami.Units.gridUnit * 10
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 4
						horizontalAlignment: Text.AlignRight
						text: qsTr("Rating:")
					}
					TemplateSpinBox {
						id: ratingPicker
						Layout.preferredWidth: Kirigami.Units.gridUnit * 6
						from: 0
						to: 5
						value: rating
						onValueChanged: rating = value
					}

				}
				RowLayout {
					width: Kirigami.Units.gridUnit * 10
					TemplateLabelSmall {
						Layout.preferredWidth: Kirigami.Units.gridUnit * 4
						horizontalAlignment: Text.AlignRight
						text: qsTr("Visibility:")
					}
					TemplateSpinBox {
						id: visibilityPicker
						Layout.preferredWidth: Kirigami.Units.gridUnit * 6
						from: 0
						to: 5
						value: visibility
						onValueChanged: visibility = value
					}
					Item { Layout.fillWidth: true }
				}
			}
			ColumnLayout {
				width: parent.width
				TemplateLabelSmall {
					Layout.preferredWidth: parent.width
					text: qsTr("Notes:")
				}
				Controls.TextArea {
					Layout.preferredWidth: parent.width
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
					onPressed: waitForKeyboard.start()
					onCursorRectangleChanged: {
						ensureVisible()
					}
					// ensure that the y coordinate is inside the visible part of the detailsEditFlickable (our flickable)
					function ensureVisible() {
						// make sure there's enough space for the TextArea above the keyboard and action button
						// and that it's not too far up, either
						var flickable = detailsEditFlickable
						var positionInFlickable = txtNotes.mapToItem(flickable.contentItem, 0, 0)
						var taY = positionInFlickable.y + cursorRectangle.y
						if (manager.verboseEnabled)
							manager.appendTextToLog("position check: lower edge of view is " +
										    (0 + flickable.contentY + flickable.height) +
										    " and text area is at " + taY)
						if (taY > flickable.contentY + flickable.height - 4 * Kirigami.Units.gridUnit)
							flickable.contentY = Math.max(0, 4 * Kirigami.Units.gridUnit + taY - flickable.height)
						while (taY < flickable.contentY)
							flickable.contentY -= 2 * Kirigami.Units.gridUnit
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
							txtNotes.ensureVisible()
						}
					}
				}
			}
		}
		Item {
			anchors.top: editArea.bottom
			height: Kirigami.Units.gridUnit * 3
			width: height // just to make sure the spacer doesn't produce scrollbars, but also isn't null
		}
	}
}
