import QtQuick 2.4
import QtQuick.Controls 1.3

Action {
	property string name
	objectName: name + "HifiAction"
	text: qsTr(name)
	
	signal triggeredByName(string name);
	signal toggledByName(string name);
	
	onTriggered: {
		triggeredByName(name);
	}
	
	onToggled: {
		toggledByName(name, checked);
	}
}

