import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

TextField {
	property bool fixed: false
	readOnly: fixed
	style: TextFieldStyle {
		background: Rectangle {
			color: fixed ? "transparent" : "white"
			radius: 4
			border.width: 0.5
			border.color: fixed ? "transparent" : "#c0c0c0"
		}
	}

}
