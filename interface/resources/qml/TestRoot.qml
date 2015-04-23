import Hifi 1.0
import QtQuick 2.3
import QtQuick.Controls 1.3
// Import local folder last so that our own control customizations override 
// the built in ones
import "controls"

Root {
    id: root
    anchors.fill: parent

    onWidthChanged: {
		console.log("Root width: " + width)
	}
	onHeightChanged: {
		console.log("Root height: " + height)
	}
    
    Component.onCompleted: {
    	console.log("Completed root")
    	root.forceActiveFocus()
    }
		
    Button {
        id: messageBox
        anchors.right: createDialog.left
        anchors.rightMargin: 24
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        text: "Message"
        onClicked: {
            console.log("Foo")
            root.information("a")
            console.log("Bar")
        }
    }

    Button {
        id: createDialog
        anchors.right: parent.right
        anchors.rightMargin: 24
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        text: "Create"
        onClicked: {
            root.loadChild("MenuTest.qml");
        }
    }

    Keys.onPressed: {
    	console.log(event.key);
    }
}

