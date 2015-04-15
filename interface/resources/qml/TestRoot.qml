import QtQuick 2.3
import "componentCreation.js" as Creator


Item {
    id: root
    width: 1280
    height: 720

    function loadChild(url) {
        Creator.createObject(root, url)
    }
    

    CustomButton {
        anchors.right: parent.right
        anchors.rightMargin: 24
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        text: "Test"
        onClicked: {
            loadChild("TestDialog.qml");
        }
    }
}

