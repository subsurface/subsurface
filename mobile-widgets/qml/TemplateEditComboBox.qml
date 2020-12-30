// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import org.kde.kirigami 2.4 as Kirigami

// this reuses our themed combo box, but makes it editable and behave consistently
// for the dive edit page - this reduces redundant code on that page
TemplateComboBox {
	id: ecb
	editable: true
	inputMethodHints: Qt.ImhNoPredictiveText
	onActivated: {
		focus = false
	}
	onAccepted: {
		focus = false
	}
	onEditTextChanged: { // this allows us to set the initial text in DiveDetails / startEditMode()
		displayText = editText
	}
}
