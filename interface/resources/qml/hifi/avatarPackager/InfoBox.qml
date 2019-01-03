import Hifi 1.0 as Hifi
import QtQuick 2.5
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../../controls" as HifiControls

Rectangle {
    id: root
    visible: false
    color: Qt.rgba(.34, .34, .34, 0.6)
    z: 999;

    anchors.fill: parent

    property alias title: titleText.text
    property alias content: loader.sourceComponent

    property bool closeOnClickOutside: false;

    property alias boxWidth: mainContainer.width
    property alias boxHeight: mainContainer.height

    function open() {
        visible = true;
    }

    function close() {
        visible = false;
    }

    HifiConstants {
        id: hifi
    }

    // This object is always used in a popup.
    // This MouseArea is used to prevent a user from being
    //     able to click on a button/mouseArea underneath the popup.
    MouseArea {
        anchors.fill: parent;
        propagateComposedEvents: false;
        hoverEnabled: true;
        onClicked: {
            if (closeOnClickOutside) {
                root.close()
            }
        }
    }

    Rectangle {
        id: mainContainer

        width: Math.max(parent.width * 0.8, 400)
        height: parent.height * 0.6

        MouseArea {
            anchors.fill: parent
            propagateComposedEvents: false
            hoverEnabled: true
            onClicked: function(ev) {
                ev.accepted = true;
            }
        }

        anchors.centerIn: parent

        color: "#252525"

        // TextStyle1
        RalewaySemiBold {
            id: titleText
            size: 24
            color: "white"

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 30

            text: "Title not defined"
        }

        Item {
            anchors.topMargin: 10
            anchors.top: titleText.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: button.top

            Loader {
                id: loader
                anchors.fill: parent
            }
        }

        Item {
            id: button

            height: 40
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12

            HifiControlsUit.Button {
                anchors.centerIn: parent

                text: "CLOSE"
                onClicked: close()

                color: hifi.buttons.noneBorderlessWhite;
                colorScheme: hifi.colorSchemes.dark;
            }
        }
    }
}
