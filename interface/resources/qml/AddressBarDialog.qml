import Hifi 1.0
import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Controls.Styles 1.3

AddressBarDialog {
    id: addressBarDialog
    objectName: "AddressBarDialog"
    SystemPalette { id: myPalette; colorGroup: SystemPalette.Active }
    height: 128
    width: 512

    onVisibleChanged: {
        if (!visible) {
            reset();
        } else {
            addressLine.focus = true
            addressLine.forceActiveFocus()
        }
    }

    Component.onCompleted: {
        addressLine.focus = true
        addressLine.forceActiveFocus()
    }

    function reset() {
        addressLine.text = ""
        goButton.source = "../images/address-bar-submit.svg"
    }

    CustomDialog {
        id: dialog
        anchors.fill: parent
        title: "Go to..."

        // The client area
        Item {
            id: item1
            anchors.fill: parent
            anchors.margins: parent.margins
            anchors.topMargin: parent.topMargin

            CustomBorder {
                height: 64
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.right: goButton.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                CustomTextInput {
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
}

