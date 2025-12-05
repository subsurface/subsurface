// GasCalculator.qml
// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.12
import QtQuick.Controls 2.12 as Controls
import QtQuick.Layouts 1.12
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

TemplatePage {
    id: gasCalculatorPage
    title: qsTr("Gas Utilities")

	property string pressureUnit: (Backend.pressure === Enums.BAR) ? qsTr("bar") : qsTr("psi")
    property string gasResults: ""

    ListModel { id: gasListModel }

    Component.onCompleted: {
        // Default examples
        gasListModel.append({ "mix": "0/100", "pressure": (Backend.pressure === Enums.BAR) ? "200" : "3000", "amount": "UNLIMITED", "cost": "1.00" });
        gasListModel.append({ "mix": "100/0", "pressure": (Backend.pressure === Enums.BAR) ? "200" : "3000", "amount": "UNLIMITED", "cost": "0.50" });
        gasListModel.append({ "mix": "21/0", "pressure": (Backend.pressure === Enums.BAR) ? "200" : "3000", "amount": "UNLIMITED", "cost": "0.10" });
    }

    Flickable {
        anchors.fill: parent
        contentHeight: mainLayout.implicitHeight
        clip: true

        ColumnLayout {
            id: mainLayout
            width: parent.width
            Layout.margins: Kirigami.Units.gridUnit
            spacing: Kirigami.Units.largeSpacing

            // --- MODE SELECTOR ---
            TemplateComboBox {
                id: modeSelector
                Layout.fillWidth: true
                model: [qsTr("MOD Calculator"), qsTr("Simple Blending"), qsTr("Full Blending")]
                onCurrentIndexChanged: gasResults = ""
            }

            Kirigami.Separator { Layout.fillWidth: true }

            // --- SECTION 1: MOD INPUTS ---
            GridLayout {
                id: modInputs
                columns: 2
                Layout.fillWidth: true
                visible: modeSelector.currentIndex === 0

                TemplateComboBox {
                    id: typeBox
                    visible: false
                    model: manager.cylinderListInit
                    currentIndex: model.indexOf("AL80")
                }

                TemplateLabel { text: qsTr("Oxygen %") }
                TemplateSpinBox {
                    id: o2Box
                    Layout.fillWidth: true; from: 0; to: 100; value: 21
                }

                TemplateLabel { text: qsTr("Helium %") }
                TemplateSpinBox {
                    id: heBox
                    Layout.fillWidth: true; from: 0; to: 100; value: 0
                }
            }

            // --- SECTION 2: BLENDING TARGET INPUTS ---
            GridLayout {
                id: blendTargetInputs
                columns: 2
                Layout.fillWidth: true
                visible: modeSelector.currentIndex > 0 // Visible for Simple and Full blending

                TemplateLabel { text: qsTr("Target Cylinder Size"); visible: modeSelector.currentIndex > 0 }
                TemplateComboBox { 
                    id: targetCylinderSize; 
                    Layout.fillWidth: true; 
                    model: manager.cylinderListInit; 
                    currentIndex: model.indexOf("AL80"); 
                    visible: modeSelector.currentIndex > 0 
                }

                TemplateLabel { text: qsTr("Start Mix (O₂/He)") }
                TemplateTextField { id: targetStartMix; text: "21/0"; Layout.fillWidth: true }

                TemplateLabel { text: qsTr("Start Pressure") }
                TemplateSpinBox { id: targetStartPressure; Layout.fillWidth: true; from: 0; to: 6000; value: 0 }

                TemplateLabel { text: qsTr("Target Mix (O₂/He)") }
                TemplateTextField { id: targetEndMix; text: "32/0"; Layout.fillWidth: true }

                TemplateLabel { text: qsTr("Target Pressure") }
                TemplateSpinBox { id: targetEndPressure; Layout.fillWidth: true; from: 0; to: 6000; value: (Backend.pressure === Enums.BAR) ? 200 : 3000 }
            }

            // --- SECTION 3: AVAILABLE GASES ---
            ColumnLayout {
                id: availableGasesSection
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing
                visible: modeSelector.currentIndex > 0

                RowLayout {
                    Controls.Label { text: qsTr("Available Gases"); font.bold: true; font.pixelSize: Kirigami.Units.gridUnit * 1.2 }
                    TemplateButton {
                        contentItem: Text { text: "+"; horizontalAlignment: Text.AlignHCenter; color: subsurfaceTheme.primaryTextColor }
                        font.bold: true; Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                        onClicked: {
                            // Default new gas to UNLIMITED amount
                            gasListModel.append({ "mix": "21/0", "pressure": (Backend.pressure === Enums.BAR) ? "200" : "3000", "amount": "UNLIMITED", "cost": "0.10" });
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing
                    Controls.Label { text: qsTr(" "); font.bold: true; Layout.preferredWidth: Kirigami.Units.gridUnit * 2 }
                    Controls.Label { text: qsTr("#"); font.bold: true; Layout.preferredWidth: Kirigami.Units.gridUnit * 2 }
                    Controls.Label { text: qsTr("Cylinder"); font.bold: true; Layout.preferredWidth: Kirigami.Units.gridUnit * 7 ; visible: modeSelector.currentIndex === 2 }
                    Controls.Label { text: qsTr("Mix"); font.bold: true; Layout.fillWidth: true }
                    Controls.Label { text: qsTr("Boost"); font.bold: true; Layout.preferredWidth: Kirigami.Units.gridUnit * 2; visible: modeSelector.currentIndex === 2 }
                    Controls.Label { text: qsTr("[%1]").arg(pressureUnit); font.bold: true; Layout.preferredWidth: Kirigami.Units.gridUnit * 3; visible: modeSelector.currentIndex === 2 }
                    Controls.Label { text: qsTr("Cost/vol"); font.bold: true; Layout.preferredWidth: Kirigami.Units.gridUnit * 4; visible: modeSelector.currentIndex > 0 }
                    Item { Layout.preferredWidth: Kirigami.Units.iconSizes.small }
                }

                ListView {
                    id: cylinderListView
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(contentHeight, Kirigami.Units.gridUnit * 10)
                    clip: true
                    model: gasListModel

                    move: Transition {
                        NumberAnimation { properties: "x,y"; duration: 200 }
                    }

                    delegate: DropArea {
                        id: dragDelegate
                        width: parent.width
                        height: content.height

                        property int modelIndex: index
                        
                        onEntered: {
                            var from = drag.source.modelIndex
                            var to = dragDelegate.modelIndex
                            if (from !== to) {
                                gasListModel.move(from, to, 1)
                            }
                        }

                        Item {
                            id: content
                            width: parent.width
                            height: rowLayout.implicitHeight

                            anchors.horizontalCenter: undefined
                            anchors.verticalCenter: undefined
                            
                            Drag.active: dragMouseArea.drag.active
                            Drag.source: dragDelegate
                            Drag.hotSpot.x: width / 2
                            Drag.hotSpot.y: height / 2

                            z: dragMouseArea.drag.active ? 100 : 1
                            
                            states: State {
                                when: dragMouseArea.drag.active
                                ParentChange { target: content; parent: cylinderListView } 
                                AnchorChanges { target: content; anchors.horizontalCenter: undefined; anchors.verticalCenter: undefined }
                            }

                            Rectangle {
                                anchors.fill: parent
                                color: subsurfaceTheme.backgroundColor || "white"
                                opacity: dragMouseArea.drag.active ? 0.9 : 0.0
                            }

                            RowLayout {
                                id: rowLayout
                                width: parent.width
                                spacing: Kirigami.Units.smallSpacing

                                Item {
                                    Layout.preferredWidth: Kirigami.Units.gridUnit * 1.1
                                    Layout.fillHeight: true
                                    Layout.leftMargin: Kirigami.Units.gridUnit * 0.5
                                    
                                    Text {
                                        text: "≡"
                                        font.bold: true
                                        font.pixelSize: Kirigami.Units.gridUnit * 1.2
                                        color: subsurfaceTheme.primaryTextColor
                                        anchors.centerIn: parent
                                    }

                                    MouseArea {
                                        id: dragMouseArea
                                        anchors.fill: parent
                                        cursorShape: Qt.OpenHandCursor
                                        
                                        drag.target: content
                                        drag.axis: Drag.YAxis
                                        
                                        onPressed: cursorShape = Qt.ClosedHandCursor
                                        onReleased: {
                                            cursorShape = Qt.OpenHandCursor
                                            content.x = 0 
                                            content.y = 0
                                        }
                                    }
                                }

                                property int listRowIndex: index
                                
                                Controls.Label { 
                                    text: index + 1; 
                                    Layout.preferredWidth: Kirigami.Units.gridUnit * 2; 
                                    horizontalAlignment: Text.AlignHCenter; 
                                    verticalAlignment: Text.AlignVCenter 
                                }

                                TemplateComboBox { 
                                    id: cylinderSize
                                    Layout.preferredWidth: Kirigami.Units.gridUnit * 7
                                    visible: modeSelector.currentIndex === 2
                                    
                                    model: [qsTr("UNLIMITED")].concat(manager.cylinderListInit)
                                    currentIndex: model.indexOf(amount)

                                    onActivated: {
                                        var amt = textAt(index)
                                        gasListModel.setProperty(modelIndex, "amount", amt)
                                    }
                                }

                                TemplateTextField { text: mix; onTextChanged: gasListModel.setProperty(index, "mix", text); Layout.fillWidth: true }
                                TemplateCheckBox { checked: boost; onCheckedChanged: gasListModel.setProperty(index, "boost", checked); Layout.preferredWidth: Kirigami.Units.gridUnit * 2; visible: modeSelector.currentIndex === 2 }
                                TemplateTextField { text: pressure; onTextChanged: gasListModel.setProperty(index, "pressure", text); Layout.preferredWidth: Kirigami.Units.gridUnit * 3; visible: modeSelector.currentIndex === 2 }
                                TemplateTextField { text: cost; onTextChanged: gasListModel.setProperty(index, "cost", text); Layout.preferredWidth: Kirigami.Units.gridUnit * 4; visible: modeSelector.currentIndex > 0 }
                                
                                TemplateButton {
                                    text: "X"; font.bold: true; Layout.preferredWidth: Kirigami.Units.iconSizes.small
                                    onClicked: gasListModel.remove(index)
                                }
                            }
                        }
                    }
                }
            }
            // --- CALCULATE BUTTON ---
            TemplateButton {
                Layout.alignment: Qt.AlignHCenter
                text: {
                    switch (modeSelector.currentIndex) {
                        case 0: return qsTr("Calculate MOD");
                        case 1: return qsTr("Calculate Simple Blend");
                        case 2: return qsTr("Calculate Cheapest Blend");
                    }
                }
                onClicked: {
                    gasResults = ""; // Clear previous results
                    var results;
                    switch (modeSelector.currentIndex) {
                        case 0: // MOD
                            if (heBox.value + o2Box.value > 100) heBox.value = 100 - o2Box.value;
                            results = Backend.divePlannerPointsModel.calculateGasInfo(typeBox.currentText, o2Box.value * 10, heBox.value * 10);
                            break;
                        case 1: // Simple Blending
                        case 2: // Full Blending
                            var availableGases = [];
                            for (var i = 0; i < gasListModel.count; i++) {
                                var item = gasListModel.get(i);
                                availableGases.push({
                                    "mix": item.mix,
                                    "boost": item.boost,
                                    "pressure": item.pressure,
                                    "amount": item.amount.toString(),
                                    "cost": item.cost
                                });
                            }
                            if (modeSelector.currentIndex === 1) {
                                results = Backend.divePlannerPointsModel.calculateCheapestBlend(targetCylinderSize.currentText,targetStartMix.text, targetStartPressure.value, targetEndMix.text, targetEndPressure.value, availableGases, true);
                            } else {
                                results = Backend.divePlannerPointsModel.calculateCheapestBlend(targetCylinderSize.currentText, targetStartMix.text, targetStartPressure.value, targetEndMix.text, targetEndPressure.value, availableGases, false);
                            }
                            break;
                    }

                    if (results) {
                        gasResults = results;
                    }
                }
            }

            // --- RESULTS DISPLAY ---
            Item {
                Layout.fillWidth: true
                // Height is bound to the measurement text, with a slightly larger buffer to prevent clipping.
                Layout.preferredHeight: Math.min(resultTextMeasurement.contentHeight + (Kirigami.Units.gridUnit * 1.5), Kirigami.Units.gridUnit * 25)
                visible: gasResults !== ""
                Layout.topMargin: Kirigami.Units.gridUnit

                // This invisible Text element reliably measures the required height for the rich text content.
                Text {
                    id: resultTextMeasurement
                    width: parent.width // Must match the width of the visible component
                    text: gasResults
                    textFormat: Text.RichText
                    wrapMode: Text.Wrap // Ensure wrapping is calculated
                    visible: false // For measurement only
                }

                Controls.TextArea {
                    anchors.fill: parent // Fills the Item container
                    readOnly: true
                    wrapMode: Text.Wrap
                    text: gasResults
                    textFormat: Text.RichText
                }
            }
        }
    }

    actions.left: Kirigami.Action {
        icon { name: ":/icons/undo.svg"; color: subsurfaceTheme.primaryColor }
        text: "Return"
        onTriggered: pageStack.pop()
    }
}