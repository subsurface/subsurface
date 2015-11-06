import QtQuick 2.5
import QtQuick.Layouts 1.1

ColumnLayout {
	id: themetest

	Text {
		text: "units.gridUnit is: " + units.gridUnit
	}

	Text {
		text: "units.devicePixelRatio: " + units.devicePixelRatio

	}
}
