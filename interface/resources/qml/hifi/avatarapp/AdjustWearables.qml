import Hifi 1.0 as Hifi
import QtQuick 2.5
import "../../styles-uit"
import "../../controls-uit" as HifiControlsUit
import "../../controls" as HifiControls

Rectangle {
    id: root;
    visible: false;
    width: 480
    height: 706
    color: 'lightgray'

    property bool modified: false;
    Component.onCompleted: {
        modified = false;
    }

    onModifiedChanged: {
        console.debug('modified: ', modified)
    }

    property var onButton2Clicked;
    property var onButton1Clicked;

    function open() {
        visible = true;
    }

    function close() {
        visible = false;
    }

    HifiConstants { id: hifi }

    // This object is always used in a popup.
    // This MouseArea is used to prevent a user from being
    //     able to click on a button/mouseArea underneath the popup.
    MouseArea {
        anchors.fill: parent;
        propagateComposedEvents: false;
        hoverEnabled: true;
    }

    Column {
        anchors.top: parent.top
        anchors.topMargin: 15
        anchors.horizontalCenter: parent.horizontalCenter

        spacing: 20
        width: parent.width - 30 * 2

        TextStyle5 {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Adjust Wearables"
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            height: 2
            color: 'gray'
        }

        HifiControlsUit.ComboBox {
            anchors.left: parent.left
            anchors.right: parent.right

            model: [
                'Fedora.fbx [HeadTop_End]',
                'Fedora1.fbx [HeadTop_End]',
                'Fedora2.fbx [HeadTop_End]'
            ]
        }

        Column {
            width: parent.width
            spacing: 5

            Row {
                spacing: 20

                TextStyle5 {
                    text: "Position"
                }

                TextStyle7 {
                    text: "m"
                }
            }

            Vector3 {
                id: position
                onXvalueChanged: modified = true;
                onYvalueChanged: modified = true;
                onZvalueChanged: modified = true;
            }
        }

        Column {
            width: parent.width
            spacing: 5

            Row {
                spacing: 20

                TextStyle5 {
                    text: "Rotation"
                }

                TextStyle7 {
                    text: "deg"
                }
            }

            Vector3 {
                id: rotation
                onXvalueChanged: modified = true;
                onYvalueChanged: modified = true;
                onZvalueChanged: modified = true;
            }
        }

        Column {
            width: parent.width
            spacing: 5

            TextStyle5 {
                text: "Scale"
            }

            Item {
                width: parent.width
                height: childrenRect.height

                HifiControlsUit.SpinBox {
                    id: scalespinner
                    value: 0
                    backgroundColor: "darkgray"
                    width: position.spinboxWidth
                    colorScheme: hifi.colorSchemes.light
                    onValueChanged: modified = true;
                }

                HifiControlsUit.Button {
                    anchors.right: parent.right
                    color: hifi.buttons.red;
                    colorScheme: hifi.colorSchemes.dark;
                    text: "TAKE IT OFF"
                }
            }

        }
    }

    DialogButtons {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30
        anchors.left: parent.left
        anchors.leftMargin: 30
        anchors.right: parent.right
        anchors.rightMargin: 30

        yesText: "SAVE"
        noText: "CANCEL"

        onYesClicked: function() {
            if(onButton2Clicked) {
                onButton2Clicked();
            } else {
                root.close();
            }
        }

        onNoClicked: function() {
            if(onButton1Clicked) {
                onButton1Clicked();
            } else {
                root.close();
            }
        }
    }
}
