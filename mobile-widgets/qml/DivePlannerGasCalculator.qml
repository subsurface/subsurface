// GasCalculator.qml
// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami as Kirigami

TemplatePage {
    id: gasCalculatorPage
    title: qsTr("Gas Utilities")

    property string pressureUnit: (Backend.pressure === Enums.BAR) ? qsTr("bar") : qsTr("psi")
    property string gasResults: ""

    ListModel { id: gasListModel }

    Component.onCompleted: {
        // Default examples
        gasListModel.append({ "mix": "0/100", "pressure": (Backend.pressure === Enums.BAR) ? "200" : "3000", "amount": "UNLIMITED", "cost": "3.00", "boost": false});
        gasListModel.append({ "mix": "100/0", "pressure": (Backend.pressure === Enums.BAR) ? "200" : "3000", "amount": "UNLIMITED", "cost": "0.80", "boost": false });
        gasListModel.append({ "mix": "21/0", "pressure": (Backend.pressure === Enums.BAR) ? "200" : "3000", "amount": "UNLIMITED", "cost": "0.00", "boost": false });
    }

    Flickable {
        anchors.fill: parent
        contentHeight: mainLayout.implicitHeight + Kirigami.Units.gridUnit * 5
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
                    Layout.rightMargin: Kirigami.Units.gridUnit
                }

                TemplateLabel { text: qsTr("Helium %") }
                TemplateSpinBox {
                    id: heBox
                    Layout.fillWidth: true; from: 0; to: 100; value: 0
                    Layout.rightMargin: Kirigami.Units.gridUnit
                }
            }

            // --- SECTION 2: BLENDING TARGET INPUTS ---
            GridLayout {
                id: blendTargetInputs
                columns: 2
                Layout.fillWidth: true
                visible: modeSelector.currentIndex > 0

                TemplateLabel { text: qsTr("Target Cylinder Size") }
                TemplateComboBox { 
                    id: targetCylinderSize; 
                    Layout.fillWidth: true; 
                    model: manager.cylinderListInit; 
                    currentIndex: model.indexOf("AL80")
                    Layout.rightMargin: Kirigami.Units.gridUnit
                }

                TemplateLabel { text: qsTr("Start Mix (O₂/He)") }
                SsrfTextField { id: targetStartMix; text: "21/0"; Layout.fillWidth: true; Layout.rightMargin: Kirigami.Units.gridUnit }

                TemplateLabel { text: qsTr("Start Pressure") }
                TemplateSpinBox { 
                    id: targetStartPressure; Layout.fillWidth: true; from: 0; to: 6000; value: 0 
                    Layout.rightMargin: Kirigami.Units.gridUnit
                }

                TemplateLabel { text: qsTr("Target Mix (O₂/He)") }
                SsrfTextField { id: targetEndMix; text: "32/0"; Layout.fillWidth: true; Layout.rightMargin: Kirigami.Units.gridUnit }

                TemplateLabel { text: qsTr("Target Pressure") }
                TemplateSpinBox { 
                    id: targetEndPressure; Layout.fillWidth: true; from: 0; to: 6000; value: (Backend.pressure === Enums.BAR) ? 200 : 3000 
                    Layout.rightMargin: Kirigami.Units.gridUnit
                }
            }

            // --- SECTION 3: AVAILABLE GASES ---
            ColumnLayout {
                id: availableGasesSection
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing
                visible: modeSelector.currentIndex > 0

                RowLayout {
                    TemplateLabel {
                        text: qsTr("Available Gases")
                        font.bold: true
                        font.pixelSize: Kirigami.Units.gridUnit * 1.2
                    }
                    TemplateButton {
                        text: "+"
                        font.bold: true
                        Layout.preferredWidth: Kirigami.Units.gridUnit
                        onClicked: {
                            gasListModel.append({ "mix": "21/0", "pressure": (Backend.pressure === Enums.BAR) ? "200" : "3000", "amount": "UNLIMITED", "cost": "0.10", "boost": false });
                        }
                    }
                }

                ListView {
                    id: cylinderListView
                    Layout.fillWidth: true
                    Layout.preferredHeight: contentHeight
                    model: gasListModel

                    // Animates the items that are bumped out of the way
                    moveDisplaced: Transition {
                        NumberAnimation { properties: "y"; duration: 150; easing.type: Easing.InOutQuad }
                    }

                    header: RowLayout {
                        width: cylinderListView.width
                        spacing: Kirigami.Units.smallSpacing

                        Item { Layout.preferredWidth: Kirigami.Units.gridUnit * 1.5 } // Drag handle spacer
                        TemplateLabel { 
                            text: qsTr("#"); font.bold: true; 
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 1.0; 
                            horizontalAlignment: Text.AlignHCenter 
                        }
                        TemplateLabel { 
                            text: qsTr("Cyl"); font.bold: true; 
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 5.5
                            visible: modeSelector.currentIndex === 2 
                        }
                        TemplateLabel { 
                            text: qsTr("Mix"); font.bold: true; Layout.fillWidth: true; 
                            Layout.leftMargin: Kirigami.Units.smallSpacing // Nudges the header to align with inner textfield text
                        }
                        TemplateLabel { 
                            text: qsTr("Bst"); font.bold: true; 
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 1.5; 
                            visible: modeSelector.currentIndex === 2 
                        }
                        TemplateLabel { 
                            text: "[%1]".arg(pressureUnit); font.bold: true; 
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 2.5; 
                            visible: modeSelector.currentIndex === 2 
                        }
                        TemplateLabel { 
                            text: qsTr("Cost"); font.bold: true; 
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 2.5 
                        }
                        Item { Layout.preferredWidth: Kirigami.Units.gridUnit * 3 } // Spacer for wider X button + rightMargin
                    }

                    // --- REORDER DELEGATE ---
                    delegate: Item {
                        id: delegateContainer
                        width: cylinderListView.width
                        height: dragWrapper.height

                        property int listIndex: index
                        
                        // Keep the dragged item on top
                        z: dragArea.drag.active ? 100 : 1 

                        // Invisible wrapper that actually moves
                        Item {
                            id: dragWrapper
                            width: parent.width
                            height: rowLayout.implicitHeight

                            // Mathematical Reorder Logic
                            onYChanged: {
                                if (dragArea.drag.active) {
                                    // Trigger swap when dragged 60% into the next item's space
                                    var swapThreshold = delegateContainer.height * 0.6;
                                    
                                    // Moving Down
                                    if (y > swapThreshold && index < gasListModel.count - 1) {
                                        gasListModel.move(index, index + 1, 1);
                                        y -= delegateContainer.height; // Reset visual offset instantly
                                    } 
                                    // Moving Up
                                    else if (y < -swapThreshold && index > 0) {
                                        gasListModel.move(index, index - 1, 1);
                                        y += delegateContainer.height; // Reset visual offset instantly
                                    }
                                }
                            }

                            RowLayout {
                                id: rowLayout
                                width: parent.width
                                spacing: Kirigami.Units.smallSpacing

                                Item {
                                    Layout.preferredWidth: Kirigami.Units.gridUnit * 1.5
                                    Layout.fillHeight: true
                                    Controls.Label { 
                                        text: "≡"
                                        anchors.centerIn: parent 
                                        color: subsurfaceTheme.textColor // Match native text color perfectly
                                    }
                                    MouseArea { 
                                        id: dragArea
                                        anchors.fill: parent
                                        drag.target: dragWrapper // Targets the invisible wrapper
                                        drag.axis: Drag.YAxis

                                        onPressed: cylinderListView.interactive = false // Stop scrolling
                                        onReleased: {
                                            cylinderListView.interactive = true // Restore scrolling
                                            dragWrapper.y = 0 // Snap firmly into place
                                        }
                                    }
                                }

                                TemplateLabel { 
                                    text: index + 1; 
                                    Layout.preferredWidth: Kirigami.Units.gridUnit * 1.0; 
                                    horizontalAlignment: Text.AlignHCenter 
                                }

                                TemplateComboBox { 
                                    id: cylinderSize
                                    Layout.preferredWidth: Kirigami.Units.gridUnit * 5.5
                                    visible: modeSelector.currentIndex === 2
                                    model: [qsTr("UNLIMITED")].concat(manager.cylinderListInit)
                                    currentIndex: model.indexOf(amount)
                                    onActivated: gasListModel.setProperty(listIndex, "amount", textAt(index))
                                }

                                SsrfTextField { 
                                    text: mix; 
                                    Layout.fillWidth: true; 
                                    onTextChanged: if(text !== mix) gasListModel.setProperty(index, "mix", text) 
                                }

                                TemplateCheckBox { 
                                    checked: boost; 
                                    Layout.preferredWidth: Kirigami.Units.gridUnit * 1.5; 
                                    visible: modeSelector.currentIndex === 2;
                                    onCheckedChanged: gasListModel.setProperty(index, "boost", checked)
                                }

                                SsrfTextField { 
                                    text: pressure; 
                                    Layout.preferredWidth: Kirigami.Units.gridUnit * 2.5; 
                                    visible: modeSelector.currentIndex === 2;
                                    onTextChanged: if(text !== pressure) gasListModel.setProperty(index, "pressure", text)
                                }

                                SsrfTextField { 
                                    text: cost; 
                                    Layout.preferredWidth: Kirigami.Units.gridUnit * 2.5; 
                                    onTextChanged: if(text !== cost) gasListModel.setProperty(index, "cost", text)
                                }
                                
                                TemplateButton {
                                    Layout.preferredWidth: Kirigami.Units.gridUnit 
                                    Layout.maximumWidth: Kirigami.Units.gridUnit 
                                    Layout.rightMargin: Kirigami.Units.gridUnit
                                    
                                    // Strip all internal button padding
                                    padding: 0
                                    leftPadding: 0
                                    rightPadding: 0

                                    contentItem: Text {
                                        text: "X"
                                        font.bold: true
                                        color: subsurfaceTheme.textColor // Back to Subsurface's native theme
                                        
                                        // This is the magic line that forces the text into the exact center
                                        anchors.centerIn: parent 
                                        
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

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
            TemplateLabel {
                text: qsTr("Results")
                font.bold: true
                font.pixelSize: Kirigami.Units.gridUnit * 1.2
                visible: gasResults !== ""
            }

            Controls.TextArea {
                Layout.fillWidth: true
                Layout.rightMargin: Kirigami.Units.gridUnit
                readOnly: true
                wrapMode: Text.Wrap
                text: gasResults
                textFormat: Text.RichText
                color: subsurfaceTheme.textColor
                visible: gasResults !== ""
            }
        }
    }

    Item {
        parent: gasCalculatorPage
        z: 999
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: Kirigami.Units.gridUnit * 3 + Kirigami.Units.smallSpacing * 2
        
        Row {
            anchors.centerIn: parent
            SsrfToolButton {
                iconSource: "qrc:/icons/undo.svg"
                highlighted: true
                onClicked: pageStack.pop()
            }
        }
    }
}