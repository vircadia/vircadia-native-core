import Hifi 1.0
import QtQuick 2.3
import "controls"
import "styles"

Dialog {
    title: "Go to..."
    objectName: "AddressBarDialog"
    height: 128
    width: 512
    destroyOnCloseButton: false

    onVisibleChanged: {
        if (!visible) {
            reset();
        } 
    }

    onEnabledChanged: {
        if (enabled) {
            addressLine.forceActiveFocus();
        }
    }

    function reset() {
        addressLine.text = ""
        goButton.source = "../images/address-bar-submit.svg"
    }

    AddressBarDialog {
        id: addressBarDialog

        // The client area
        anchors.fill: parent
        anchors.margins: parent.margins
        anchors.topMargin: parent.topMargin

        Border {
            height: 64
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.right: goButton.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            TextInput {
                id: addressLine
                anchors.fill: parent
                helperText: "domain, location, @user, /x,y,z"
                anchors.margins: 8
                onAccepted: {
                    addressBarDialog.loadAddress(addressLine.text)
                }
            }
        }

        Image {
            id: goButton
            width: 32
            height: 32
            anchors.right: parent.right
            anchors.rightMargin: 8
            source: "../images/address-bar-submit.svg"
            anchors.verticalCenter: parent.verticalCenter

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    parent.source = "../images/address-bar-submit-active.svg"
                    addressBarDialog.loadAddress(addressLine.text)
                }
            }
        }
    }
}

