import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.3
import "controls"

VrDialog {
    title: "Test Dialog"
    id: testDialog
    objectName: "TestDialog"
    width: 512
    height: 512
    animationDuration: 200

    onEnabledChanged: {
        if (enabled) {
            edit.forceActiveFocus();
        }
    }
    
    Item {
        id: clientArea
        // The client area
        anchors.fill: parent
        anchors.margins: parent.margins
        anchors.topMargin: parent.topMargin
    
        Rectangle {
            property int d: 100
            id: square
            objectName: "testRect"
            width: d
            height: d
            anchors.centerIn: parent
            color: "red"
            NumberAnimation on rotation { from: 0; to: 360; duration: 2000; loops: Animation.Infinite; }
        }
    
    
        TextEdit {
            id: edit
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.right: parent.right
            anchors.rightMargin: 12
            clip: true
            text: "test edit"
            anchors.top: parent.top
            anchors.topMargin: 12
        }

        Button {
            x: 128
            y: 192
            text: "Test"
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            anchors.right: parent.right
            anchors.rightMargin: 12
            onClicked: {
                console.log("Click");
                
                if (square.visible) {
                    square.visible = false
                } else {
                    square.visible = true
                }
            }
        }
    
        Button {
            id: customButton2
            y: 192
            text: "Move"
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            onClicked: {
                onClicked: testDialog.x == 0 ? testDialog.x = 200 : testDialog.x = 0
            }
        }
        
        Keys.onPressed: {
            console.log("Key " + event.key);
            switch (event.key) {
            case Qt.Key_Q:
                if (Qt.ControlModifier == event.modifiers) {
                    event.accepted = true;
                    break;
                }
            }
        }
    }
}
