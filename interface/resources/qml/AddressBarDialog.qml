//
//  AddressBarDialog.qml
//
//  Created by Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import "controls"
import "styles"
import "windows"

Window {
    id: root
    HifiConstants { id: hifi }
    anchors.centerIn: parent
    objectName: "AddressBarDialog"
    frame: HiddenFrame {}

    visible: false
    destroyOnInvisible: false
    resizable: false
    scale: 1.25  // Make this dialog a little larger than normal

    width: addressBarDialog.implicitWidth
    height: addressBarDialog.implicitHeight

    AddressBarDialog {
        id: addressBarDialog
        implicitWidth: backgroundImage.width
        implicitHeight: backgroundImage.height

        Image {
            id: backgroundImage
            source: "../images/address-bar.svg"
            width: 576 * root.scale
            height: 80 * root.scale
            property int inputAreaHeight: 56.0 * root.scale  // Height of the background's input area
            property int inputAreaStep: (height - inputAreaHeight) / 2

            Image {
                id: backArrow
                source: addressBarDialog.backEnabled ? "../images/left-arrow.svg" : "../images/left-arrow-disabled.svg"
                anchors {
                    fill: parent
                    leftMargin: parent.height + hifi.layout.spacing + 6
                    rightMargin: parent.height + hifi.layout.spacing * 60
                    topMargin: parent.inputAreaStep + parent.inputAreaStep + hifi.layout.spacing
                    bottomMargin: parent.inputAreaStep + parent.inputAreaStep + hifi.layout.spacing
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        addressBarDialog.loadBack()
                    }
                }
            }

            Image {
                id: forwardArrow
                source: addressBarDialog.forwardEnabled ? "../images/right-arrow.svg" : "../images/right-arrow-disabled.svg"
                anchors {
                    fill: parent
                    leftMargin: parent.height + hifi.layout.spacing * 9
                    rightMargin: parent.height + hifi.layout.spacing * 53
                    topMargin: parent.inputAreaStep + parent.inputAreaStep + hifi.layout.spacing
                    bottomMargin: parent.inputAreaStep + parent.inputAreaStep + hifi.layout.spacing
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        addressBarDialog.loadForward()
                    }
                }
            }

            // FIXME replace with TextField
            TextInput {
                id: addressLine
                focus: true
                anchors {
                    fill: parent
                    leftMargin: parent.height + parent.height + hifi.layout.spacing * 5
                    rightMargin: hifi.layout.spacing * 2
                    topMargin: parent.inputAreaStep + hifi.layout.spacing
                    bottomMargin: parent.inputAreaStep + hifi.layout.spacing
                }
                font.pixelSize: hifi.fonts.pixelSize * root.scale * 0.75
                helperText: "Go to: place, @user, /path, network address"
            }

        }
    }

    onVisibleChanged: {
        if (visible) {
            addressLine.forceActiveFocus()
        } else {
            addressLine.text = ""
        }
    }

    function toggleOrGo() {
        if (addressLine.text !== "") {
            addressBarDialog.loadAddress(addressLine.text)
        }
        root.visible = false;
    }

    Keys.onPressed: {
        switch (event.key) {
            case Qt.Key_Escape:
            case Qt.Key_Back:
                root.visible = false
                event.accepted = true
                break
            case Qt.Key_Enter:
            case Qt.Key_Return:
                toggleOrGo()
                event.accepted = true
                break
        }
    }
}
