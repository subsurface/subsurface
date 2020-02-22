// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2 as Controls
import QtQuick.Layouts 1.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

Kirigami.Page {
	id: tripEditPage
	objectName: "TripDetails"
	property string tripLocation
	property string tripNotes

	title: "" !== tripLocation ? tripLocation : qsTr("Trip details")
	state: "view"
	padding: Kirigami.largeSpacing
	background: Rectangle { color: subsurfaceTheme.backgroundColor }
	width: rootItem.colWidth
	Flickable {
		id: tripEditFlickable
		anchors.fill: parent
		GridLayout {
			columns: 2
			width: tripEditFlickable.width
			TemplateLabel {
				Layout.columnSpan: 2
				id: title
			text: qsTr("Edit trip details")
			font.pointSize: subsurfaceTheme.titlePointSize
			font.bold: true
			}
			Rectangle {
				id: spacer
				Layout.columnSpan: 2
				color: subsurfaceTheme.backgroundColor
				height: Kirigami.Units.gridUnit
				width: 1
			}

			TemplateLabel {
				text: qsTr("Trip location:")
				opacity: 0.6
			}
			SsrfTextField {
				Layout.fillWidth: true
				text: tripLocation
				flickable: tripEditFlickable
			}
			TemplateLabel {
				Layout.columnSpan: 2
				text: qsTr("Trip notes")
				opacity: 0.6
			}
			Controls.TextArea {
				text: tripNotes
				textFormat: TextEdit.RichText
				Layout.columnSpan: 2
				Layout.fillWidth: true
				Layout.fillHeight: true
				Layout.minimumHeight: Kirigami.Units.gridUnit * 6
				selectByMouse: true
				wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			}
		}
	}
}
