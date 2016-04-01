import QtQuick 2.5
import QtQuick.Controls 1.4

Rectangle {
    id: root
    width: parent ? parent.width : 100
    height: parent ? parent.height : 100
            
    signal sendToScript(var message);

    Text {
        id: label
        text: "GPU Texture Usage: "
    }
    Text {
        id: usage
        anchors.left: label.right
        anchors.leftMargin: 8
        text: "N/A"
        Timer {
            repeat: true
            running: true
            interval: 500
            onTriggered: {
                usage.text = Render.getConfig("Stats")["textureGPUMemoryUsage"];
            }
        }
    }

    Column {

        
        anchors { left: parent.left; right: parent.right; top: label.bottom; topMargin: 8; bottom: parent.bottom }
        spacing: 8

        Button {
            text: "Add 1"
            onClicked: root.sendToScript(["create", 1]);
        }
        Button {
            text: "Add 10"
            onClicked: root.sendToScript(["create", 10]);
        }
        Button {
            text: "Add 100"
            onClicked: root.sendToScript(["create", 100]);
        }
        /*
        Button {
            text: "Delete 1"
            onClicked: root.sendToScript(["delete", 1]);
        }
        Button {
            text: "Delete 10"
            onClicked: root.sendToScript(["delete", 10]);
        }
        Button {
            text: "Delete 100"
            onClicked: root.sendToScript(["delete", 100]);
        }
        */
        Button {
            text: "Delete All"
            onClicked: root.sendToScript(["delete", 0]);
        }
    }
}


