import Hifi 1.0
import QtQuick 2.3

Root {
    id: root
    width: 1280
    height: 720

    CustomButton {
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

    CustomButton {
        id: createDialog
        anchors.right: parent.right
        anchors.rightMargin: 24
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        text: "Create"
        onClicked: {
            root.loadChild("TestDialog.qml");
        }
    }
}

