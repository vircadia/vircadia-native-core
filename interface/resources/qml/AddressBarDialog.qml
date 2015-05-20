import Hifi 1.0
import QtQuick 2.3
import "controls"
import "styles"

Dialog {
    id: root
    HifiConstants { id: hifi }
    
    title: "Go to..."
    objectName: "AddressBarDialog"
    contentImplicitWidth: addressBarDialog.implicitWidth
    contentImplicitHeight: addressBarDialog.implicitHeight
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
    onParentChanged: {
        if (enabled && visible) {
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
        x: root.clientX
        y: root.clientY
        implicitWidth: 512
        implicitHeight: border.height + hifi.layout.spacing * 4


        Border {
            id: border
            height: 64
            anchors.left: parent.left
            anchors.leftMargin: hifi.layout.spacing * 2
            anchors.right: goButton.left
            anchors.rightMargin: hifi.layout.spacing
            anchors.verticalCenter: parent.verticalCenter
            TextInput {
                id: addressLine
                anchors.fill: parent
                helperText: "domain, location, @user, /x,y,z"
                anchors.margins: hifi.layout.spacing
                onAccepted: {
                    event.accepted
                    addressBarDialog.loadAddress(addressLine.text)
                }
            }
        }

        Image {
            id: goButton
            width: 32
            height: 32
            anchors.right: parent.right
            anchors.rightMargin: hifi.layout.spacing * 2
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
    
    Keys.onEscapePressed: {
        enabled = false;
    }

    function toggleOrGo() {
        if (addressLine.text == "") {
            enabled = false
        } else {
            addressBarDialog.loadAddress(addressLine.text)
        }
    }
    
    Keys.onReturnPressed: toggleOrGo()
    Keys.onEnterPressed: toggleOrGo()
}

