import QtQuick 2.3

Item {
	property int gridUnit: unitsM.paintedHeight
	property int spacing: gridUnit / 3

	Text {
		id: unitsM
		text: "M"
	}
}