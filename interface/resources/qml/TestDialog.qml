import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Controls.Styles 1.3

Item {
    objectName: "TestDialog"
    id: testDialog
    width: 384
    height: 384
    scale: 0.0

    onEnabledChanged: {
        scale = enabled ? 1.0 : 0.0
    }
    onScaleChanged: {
        visible = (scale != 0.0);
    }
    Component.onCompleted: {
        scale = 1.0
    }
    Behavior on scale {
        NumberAnimation {
            //This specifies how long the animation takes
            duration: 400
            //This selects an easing curve to interpolate with, the default is Easing.Linear
            easing.type: Easing.InOutBounce
        }
    }

    CustomDialog {
        title: "Test Dlg"
        anchors.fill: parent
    
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
    
    
        CustomTextEdit {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.right: parent.right
            anchors.rightMargin: 12
            clip: true
            text: "test edit"
            anchors.top: parent.top
            anchors.topMargin: parent.titleSize + 12
        }
    
        CustomButton {
            x: 128
            y: 192
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
    
        CustomButton {
            id: customButton2
            y: 192
            text: "Close"
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


/*

// This is the behavior, and it applies a NumberAnimation to any attempt to set the x property

MouseArea {
    anchors.fill: parent
}
*/
