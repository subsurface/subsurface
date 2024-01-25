// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import org.kde.kirigami 2.4 as Kirigami
import org.subsurfacedivelog.mobile 1.0

TemplatePage {
	objectName: "Settings"
	title: qsTr("Settings")
	width: rootItem.colWidth - Kirigami.Units.gridUnit
	property alias defaultCylinderModel: defaultCylinderBox.model
	property alias defaultCylinderIndex: defaultCylinderBox.currentIndex
	property real contentColumeWidth: width - 2 * Kirigami.Units.gridUnit
	property var describe: [qsTr("Undefined"),
		qsTr("Incorrect username/password combination"),
		qsTr("Credentials need to be verified"),
		qsTr("Credentials verified"),
		qsTr("No cloud mode")]
	Column {
		width: contentColumeWidth
		TemplateSection {
			id: sectionGeneral
			title: qsTr("General settings")
			isExpanded: true
			width: parent.width

			GridLayout {
				id: cloudSetting
				visible: sectionGeneral.isExpanded
				columns: 3
				width: parent.width
				TemplateLabel {
					text: qsTr("Cloud status")
					font.pointSize: subsurfaceTheme.headingPointSize
					font.weight: Font.Light
					Layout.topMargin: Kirigami.Units.largeSpacing
					Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
					Layout.columnSpan: 3
				}
				TemplateLabel {
					text: qsTr("Email")
				}
				TemplateLabel {
					text: Backend.cloud_verification_status === Enums.CS_NOCLOUD ? qsTr("Not applicable") : PrefCloudStorage.cloud_storage_email
					Layout.fillWidth: true
				}
				TemplateButton {
					id: changeCloudSettings
					text: qsTr("Change")
					onClicked: {
						Backend.cloud_verification_status = Enums.CS_UNKNOWN
						manager.startPageText  = qsTr("Starting...");
					}
				}
				TemplateLabel {
					text: qsTr("Status")
					Layout.preferredHeight: Kirigami.Units.gridUnit * 1.5
				}
				TemplateLabel {
					text: describe[Backend.cloud_verification_status]
					Layout.preferredHeight: Kirigami.Units.gridUnit * 1.5
				}
				TemplateButton {
					id: deleteCloudAccount
					enabled: Backend.cloud_verification_status !== Enums.CS_NOCLOUD
					text: qsTr("Delete Account")
					onClicked: {
						manager.appendTextToLog("requesting account deletion");
						showPage(deleteAccount)
					}
				}
			}
			TemplateLine {
				visible: sectionGeneral.isExpanded
			}
			GridLayout {
				id: defaultCylinder
				visible: sectionGeneral.isExpanded
				columns: 2
				width: parent.width
				TemplateLabel {
					text: qsTr("Default Cylinder")
					font.pointSize: subsurfaceTheme.headingPointSize
					font.weight: Font.Light
					Layout.topMargin: Kirigami.Units.largeSpacing
					Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
					Layout.columnSpan: 2
				}
				TemplateLabel {
					text: qsTr("Cylinder:")
				}
				TemplateSlimComboBox {
					id: defaultCylinderBox
					Layout.fillWidth: true
					onActivated: {
						// the entry for 'no default cylinder' is known to be the top, but its text
						// is possibly translated so check against the index
						if (currentIndex === 0)
							PrefEquipment.default_cylinder = ""
						else
							PrefEquipment.default_cylinder = defaultCylinderBox.currentText
					}
				}
			}
			TemplateLine {
				visible: sectionGeneral.isExpanded
			}
			GridLayout {
				id: divecomputers
				visible: sectionGeneral.isExpanded
				columns: 2
				width: parent.width
				TemplateLabel {
					text: qsTr("Dive computers")
					font.pointSize: subsurfaceTheme.headingPointSize
					font.weight: Font.Light
					Layout.topMargin: Kirigami.Units.largeSpacing
					Layout.bottomMargin: Kirigami.Units.largeSpacing
					Layout.columnSpan: 2
				}
				TemplateLabel {
					text: qsTr("Forget remembered dive computers")
					Layout.fillWidth: true
				}
				TemplateButton {
					id: forgetDCButton
					text: qsTr("Forget")
					enabled: PrefDiveComputer.vendor1 !== ""
					onClicked: {
						PrefDiveComputer.vendor1 = PrefDiveComputer.product1 = PrefDiveComputer.device1 = ""
						PrefDiveComputer.vendor2 = PrefDiveComputer.product2 = PrefDiveComputer.device2 = ""
						PrefDiveComputer.vendor3 = PrefDiveComputer.product3 = PrefDiveComputer.device3 = ""
						PrefDiveComputer.vendor4 = PrefDiveComputer.product4 = PrefDiveComputer.device4 = ""
						PrefDiveComputer.vendor = PrefDiveComputer.product = PrefDiveComputer.device = ""
					}
				}
			}

		}
		TemplateSection {
			id: sectionTheme
			title: qsTr("Theme")
			width: parent.width
			GridLayout {
				visible: sectionTheme.isExpanded
				columns: 2
				width: parent.width
				TemplateLabel {
					text: qsTr("Color theme")
					font.pointSize: subsurfaceTheme.headingPointSize
					font.weight: Font.Light
					Layout.topMargin: Kirigami.Units.largeSpacing
					Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
					Layout.columnSpan: 2
				}
				TemplateSlimComboBox {
					editable: false
					Layout.columnSpan: 2
					currentIndex: (subsurfaceTheme.currentTheme === "Blue") ? 0 :
								  (subsurfaceTheme.currentTheme === "Pink") ? 1 : 2
					model: ListModel {
						   ListElement {text: qsTr("Blue")}
						   ListElement {text: qsTr("Pink")}
						   ListElement {text: qsTr("Dark")}
					}
					onActivated:  {
						subsurfaceTheme.currentTheme = currentIndex === 0 ? "Blue" : currentIndex === 1 ? "Pink" : "Dark"
					}
				}
				Rectangle {
					color: 	subsurfaceTheme.backgroundColor
					Layout.fillWidth: true
					Layout.preferredHeight: 2 * Kirigami.Units.gridUnit
					TemplateLabel {
						text: qsTr("background")
						color: subsurfaceTheme.textColor
						colorBackground: parent.color
						anchors.horizontalCenter: parent.horizontalCenter
						anchors.verticalCenter: parent.verticalCenter
					}
				}
				Rectangle {
					color: subsurfaceTheme.backgroundColor
					Layout.fillWidth: true
					Layout.preferredHeight: 2 * Kirigami.Units.gridUnit
					TemplateLabel {
						text: qsTr("text")
						color: subsurfaceTheme.textColor
						colorBackground: parent.color
						anchors.horizontalCenter: parent.horizontalCenter
						anchors.verticalCenter: parent.verticalCenter
					}
				}
				Rectangle {
					border.width: 2
					border.color: "black"
					color: 	subsurfaceTheme.primaryColor
					Layout.fillWidth: true
					Layout.preferredHeight: 2 * Kirigami.Units.gridUnit
					TemplateLabel {
						text: qsTr("primary")
						color: subsurfaceTheme.primaryTextColor
						colorBackground: parent.color
						anchors.horizontalCenter: parent.horizontalCenter
						anchors.verticalCenter: parent.verticalCenter
					}
				}
				Rectangle {
					border.width: 2
					border.color: "black"
					color: subsurfaceTheme.primaryTextColor
					Layout.fillWidth: true
					Layout.preferredHeight: 2 * Kirigami.Units.gridUnit
					TemplateLabel {
						text: qsTr("primary text")
						color: subsurfaceTheme.primaryColor
						colorBackground: parent.color
						anchors.horizontalCenter: parent.horizontalCenter
						anchors.verticalCenter: parent.verticalCenter
					}
				}
				Rectangle {
					border.width: 2
					border.color: "black"
					color: 	subsurfaceTheme.darkerPrimaryColor
					Layout.fillWidth: true
					Layout.preferredHeight: 2 * Kirigami.Units.gridUnit
					TemplateLabel {
						text: qsTr("darker primary")
						color: subsurfaceTheme.darkerPrimaryTextColor
						colorBackground: parent.color
						anchors.horizontalCenter: parent.horizontalCenter
						anchors.verticalCenter: parent.verticalCenter
					}
				}
				Rectangle {
					border.width: 2
					border.color: "black"
					color: subsurfaceTheme.darkerPrimaryTextColor
					Layout.fillWidth: true
					Layout.preferredHeight: 2 * Kirigami.Units.gridUnit
					TemplateLabel {
						text: qsTr("darker primary text")
						color: subsurfaceTheme.darkerPrimaryColor
						colorBackground: parent.color
						anchors.horizontalCenter: parent.horizontalCenter
						anchors.verticalCenter: parent.verticalCenter
					}
				}
				Rectangle {
					border.width: 2
					border.color: "black"
					color: 	subsurfaceTheme.lightPrimaryColor
					Layout.fillWidth: true
					Layout.preferredHeight: 2 * Kirigami.Units.gridUnit
					TemplateLabel {
						text: qsTr("light primary")
						color: subsurfaceTheme.lightPrimaryTextColor
						colorBackground: parent.color
						anchors.horizontalCenter: parent.horizontalCenter
						anchors.verticalCenter: parent.verticalCenter
					}
				}
				Rectangle {
					border.width: 2
					border.color: "black"
					color: subsurfaceTheme.lightPrimaryTextColor
					Layout.fillWidth: true
					Layout.preferredHeight: 2 * Kirigami.Units.gridUnit
					TemplateLabel {
						text: qsTr("light primary text")
						color: subsurfaceTheme.lightPrimaryColor
						colorBackground: parent.color
						anchors.horizontalCenter: parent.horizontalCenter
						anchors.verticalCenter: parent.verticalCenter
					}
				}
				TemplateLabel {
					text: ""
				}
				Rectangle {
					border.width: 2
					border.color: "black"
					color: subsurfaceTheme.secondaryTextColor
					Layout.fillWidth: true
					Layout.preferredHeight: 2 * Kirigami.Units.gridUnit
					TemplateLabel {
						text: qsTr("secondary text")
						color: subsurfaceTheme.primaryColor
						colorBackground: parent.color
						anchors.horizontalCenter: parent.horizontalCenter
						anchors.verticalCenter: parent.verticalCenter
					}
				}
				Rectangle {
					border.width: 2
					border.color: "black"
					color: 	subsurfaceTheme.drawerColor
					Layout.fillWidth: true
					Layout.preferredHeight: 2 * Kirigami.Units.gridUnit
					TemplateLabel {
						text: qsTr("drawer")
						color: subsurfaceTheme.textColor
						colorBackground: parent.color
						anchors.horizontalCenter: parent.horizontalCenter
						anchors.verticalCenter: parent.verticalCenter
					}
				}
				TemplateLabel {
					text: ""
				}
				TemplateLabel {
					text: qsTr("Font size")
					font.pointSize: subsurfaceTheme.headingPointSize
					font.weight: Font.Light
					Layout.topMargin: Kirigami.Units.largeSpacing
					Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
					Layout.columnSpan: 2
				}
				Flow {
					Layout.columnSpan: 2
					spacing: Kirigami.Units.largeSpacing
					TemplateButton {
						text: qsTr("very small")
						fontSize: subsurfaceTheme.regularPointSize / subsurfaceTheme.currentScale * 0.75
						enabled: subsurfaceTheme.currentScale !== 0.75
						onClicked: {
							subsurfaceTheme.currentScale = 0.75
							rootItem.setupUnits()
						}
					}
					TemplateButton {
						text: qsTr("small")
						Layout.fillWidth: true
						fontSize: subsurfaceTheme.regularPointSize / subsurfaceTheme.currentScale * 0.85
						enabled: subsurfaceTheme.currentScale !== 0.85
						onClicked: {
							subsurfaceTheme.currentScale = 0.85
							rootItem.setupUnits()
						}
					}
					TemplateButton {
						text: qsTr("regular")
						Layout.fillWidth: true
						fontSize: subsurfaceTheme.regularPointSize / subsurfaceTheme.currentScale * 0.85
						enabled: subsurfaceTheme.currentScale !== 1.0
						onClicked: {
							subsurfaceTheme.currentScale = 1.0
							rootItem.setupUnits()
						}
					}
					TemplateButton {
						text: qsTr("large")
						Layout.fillWidth: true
						fontSize: subsurfaceTheme.regularPointSize / subsurfaceTheme.currentScale * 1.15
						enabled: subsurfaceTheme.currentScale !== 1.15
						onClicked: {
							subsurfaceTheme.currentScale = 1.15
							rootItem.setupUnits()
						}
					}
					TemplateButton {
						text: qsTr("very large")
						Layout.fillWidth: true
						fontSize: subsurfaceTheme.regularPointSize / subsurfaceTheme.currentScale * 1.3
						enabled: subsurfaceTheme.currentScale !== 1.3
						onClicked: {
							subsurfaceTheme.currentScale = 1.3
							rootItem.setupUnits()
						}
					}
				}
			}
		}
		TemplateSection {
			id: sectionUnits
			title: qsTr("Units")
			width: parent.width
			RowLayout {
				visible: sectionUnits.isExpanded
				width: parent.width
				TemplateRadioButton {
					text: qsTr("Metric")
					Layout.fillWidth: true
					checked: (Backend.unit_system === Enums.METRIC)
					onClicked: {
						Backend.unit_system = Enums.METRIC
						manager.changesNeedSaving()
						manager.refreshDiveList()
					}
				}
				TemplateRadioButton {
					text: qsTr("Imperial")
					Layout.fillWidth: true
					checked:  (Backend.unit_system === Enums.IMPERIAL)
					onClicked: {
						Backend.unit_system = Enums.IMPERIAL
						manager.changesNeedSaving()
						manager.refreshDiveList()
					}
				}
				TemplateRadioButton {
					text: qsTr("Personalize")
					Layout.fillWidth: true
					checked:  (Backend.unit_system === Enums.PERSONALIZE)
					onClicked: {
						Backend.unit_system = Enums.PERSONALIZE
						manager.changesNeedSaving()
						manager.refreshDiveList()
					}
				}
			}

			GridLayout {
				visible: sectionUnits.isExpanded
				width: parent.width - 2 * Kirigami.Units.gridUnit
				enabled: (Backend.unit_system === Enums.PERSONALIZE)
				anchors.horizontalCenter: parent.horizontalCenter
				columns: 3

				ButtonGroup { id: unitDepth }
				ButtonGroup { id: unitPressure }
				ButtonGroup { id: unitVolume }
				ButtonGroup { id: unitTemperature }
				ButtonGroup { id: unitWeight }

				TemplateLabel {
					text: qsTr("Depth")
					font.bold: (Backend.unit_system === Enums.PERSONALIZE)
				}
				TemplateRadioButton {
					text: qsTr("meters")
					checked: (Backend.length === Enums.METERS)
					ButtonGroup.group: unitDepth
					onClicked: {
						Backend.length = Enums.METERS
					}
				}
				TemplateRadioButton {
					text: qsTr("feet")
					checked: (Backend.length === Enums.FEET)
					ButtonGroup.group: unitDepth
					onClicked: {
						Backend.length = Enums.FEET
					}
				}
				TemplateLabel {
					text: qsTr("Pressure")
					font.bold: (Backend.unit_system === Enums.PERSONALIZE)
				}
				TemplateRadioButton {
					text: qsTr("bar")
					checked: (Backend.pressure === Enums.BAR)
					ButtonGroup.group: unitPressure
					onClicked: {
						Backend.pressure = Enums.BAR
					}
				}
				TemplateRadioButton {
					text: qsTr("psi")
					checked: (Backend.pressure === Enums.PSI)
					ButtonGroup.group: unitPressure
					onClicked: {
						Backend.pressure = Enums.PSI
					}
				}
				TemplateLabel {
					text: qsTr("Volume")
					font.bold: (Backend.unit_system === Enums.PERSONALIZE)
				}
				TemplateRadioButton {
					text: qsTr("liter")
					checked: (Backend.volume === Enums.LITER)
					ButtonGroup.group: unitVolume
					onClicked: {
						Backend.volume = Enums.LITER
					}
				}
				TemplateRadioButton {
					text: qsTr("cuft")
					checked: (Backend.volume === Enums.CUFT)
					ButtonGroup.group: unitVolume
					onClicked: {
						Backend.volume = Enums.CUFT
					}
				}
				TemplateLabel {
					text: qsTr("Temperature")
					font.bold: (Backend.unit_system === Enums.PERSONALIZE)
				}
				TemplateRadioButton {
					text: qsTr("celsius")
					checked: (Backend.temperature === Enums.CELSIUS)
					ButtonGroup.group: unitTemperature
					onClicked: {
						Backend.temperature = Enums.CELSIUS
					}
				}
				TemplateRadioButton {
					text: qsTr("fahrenheit")
					checked: (Backend.temperature === Enums.FAHRENHEIT)
					ButtonGroup.group: unitTemperature
					onClicked: {
						Backend.temperature = Enums.FAHRENHEIT
					}
				}
				TemplateLabel {
					text: qsTr("Weight")
					font.bold: (Backend.unit_system === Enums.PERSONALIZE)
				}
				TemplateRadioButton {
					text: qsTr("kg")
					checked: (Backend.weight === Enums.KG)
					ButtonGroup.group: unitWeight
					onClicked: {
						Backend.weight = Enums.KG
					}
				}
				TemplateRadioButton {
					text: qsTr("lbs")
					checked: (Backend.weight === Enums.LBS)
					ButtonGroup.group: unitWeight
					onClicked: {
						Backend.weight = Enums.LBS
					}
				}
			}
		}

		TemplateSection {
			id: sectionAdvanced
			title: qsTr("Advanced")
			width: parent.width

			GridLayout {
				visible: sectionAdvanced.isExpanded
				width: parent.width
				columns: 2

				TemplateLine {
					visible: sectionAdvanced.isExpanded
					Layout.columnSpan: 2
				}
				TemplateLabel {
					text: qsTr("Bluetooth")
					font.pointSize: subsurfaceTheme.headingPointSize
					font.weight: Font.Light
					Layout.topMargin: Kirigami.Units.largeSpacing
					Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
					Layout.columnSpan: 2
				}
				TemplateLabel {
					text: qsTr("Temporarily show all bluetooth devices \neven if not recognized as dive computers.\nPlease report DCs that need this setting")
					Layout.fillWidth: true
				}
				SsrfSwitch {
					id: nonDCButton
					checked: manager.showNonDiveComputers
					onClicked: {
						manager.showNonDiveComputers = checked
					}
				}

				TemplateLine {
					visible: sectionAdvanced.isExpanded
					Layout.columnSpan: 2
				}
				TemplateLabel {
					text: qsTr("Display")
					font.pointSize: subsurfaceTheme.headingPointSize
					font.weight: Font.Light
					Layout.topMargin: Kirigami.Units.largeSpacing
					Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
					Layout.columnSpan: 2
				}
				TemplateLabel {
					text: qsTr("Show only one column in Portrait mode")
					Layout.fillWidth: true
				}
				SsrfSwitch {
					id: singleColumnButton
					checked: PrefDisplay.singleColumnPortrait
					onClicked: {
						PrefDisplay.singleColumnPortrait = checked
					}
				}
		TemplateLabel {
		    text: qsTr("Depth line based on ×3 intervals")
		}
		SsrfSwitch {
		    checked: PrefDisplay.three_m_based_grid
		    onClicked: {
			PrefDisplay.three_m_based_grid = checked
			rootItem.settingsChanged()
		    }
		}
				TemplateLine {
					visible: sectionAdvanced.isExpanded
					Layout.columnSpan: 2
				}
				TemplateLabel {
					text: qsTr("Profile deco ceiling")
					font.pointSize: subsurfaceTheme.headingPointSize
					font.weight: Font.Light
					Layout.topMargin: Kirigami.Units.largeSpacing
					Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
					Layout.columnSpan: 2
				}
				TemplateLabel {
					text: qsTr("Show DC reported ceiling")
				}
				SsrfSwitch {
					checked: PrefTechnicalDetails.dcceiling
					onClicked: {
						PrefTechnicalDetails.dcceiling = checked
						rootItem.settingsChanged()
					}
				}
				TemplateLabel {
					text: qsTr("Show calculated ceiling")
				}
				SsrfSwitch {
					checked: PrefTechnicalDetails.calcceiling
					onClicked: {
						PrefTechnicalDetails.calcceiling = checked
						rootItem.settingsChanged()
					}
				}
				TemplateLabel {
					text: qsTr("GFLow")
				}
				TemplateSpinBox {
					id: gfLow
					from: 10
					to: 150
					stepSize: 1
					value: PrefTechnicalDetails.gflow
					textFromValue: function (value, locale) {
						return value + "%"
					}
					onValueChanged: {
						PrefTechnicalDetails.gflow = value
						rootItem.settingsChanged()
					}
				}
				TemplateLabel {
					text: qsTr("GFHigh")
				}
				TemplateSpinBox {
					id: gfHigh
					from: 10
					to: 150
					stepSize: 1
					value: PrefTechnicalDetails.gfhigh
					textFromValue: function (value, locale) {
						return value + "%"
					}
					onValueChanged: {
						PrefTechnicalDetails.gfhigh = value
						rootItem.settingsChanged()
					}
				}
				TemplateLine {
					visible: sectionAdvanced.isExpanded
					Layout.columnSpan: 2
				}
				TemplateLabel {
					text: qsTr("Developer")
					font.pointSize: subsurfaceTheme.headingPointSize
					font.weight: Font.Light
					Layout.topMargin: Kirigami.Units.largeSpacing
					Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
					Layout.columnSpan: 2
				}
				TemplateLabel {
					text: qsTr("Display Developer menu")
					Layout.fillWidth: true
				}
				SsrfSwitch {
					id: developerButton
					checked: PrefDisplay.show_developer
					onClicked: {
						PrefDisplay.show_developer = checked
					}
				}
			}
		}
	}
}
