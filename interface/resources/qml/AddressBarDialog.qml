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

DialogContainer {
    id: root
    HifiConstants { id: hifi }
    z: 1000

    objectName: "AddressBarDialog"

    property bool destroyOnInvisible: false
    property real scale: 1.25  // Make this dialog a little larger than normal

    implicitWidth: addressBarDialog.implicitWidth
    implicitHeight: addressBarDialog.implicitHeight

    x: parent ? parent.width / 2 - width / 2 : 0
    y: parent ? parent.height / 2 - height / 2 : 0
    property int maximumX: parent ? parent.width - width : 0
    property int maximumY: parent ? parent.height - height : 0


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

            MouseArea {
                // Drag the icon
                width: parent.height
                height: parent.height
                x: 0
                y: 0
                drag {
                    target: root
                    minimumX: -parent.inputAreaStep
                    minimumY: -parent.inputAreaStep
                    maximumX: root.parent ? root.maximumX : 0
                    maximumY: root.parent ? root.maximumY + parent.inputAreaStep : 0
                }
            }

            MouseArea {
                // Drag the input rectangle
                width: parent.width - parent.height
                height: parent.inputAreaHeight
                x: parent.height
                y: parent.inputAreaStep
                drag {
                    target: root
                    minimumX: -parent.inputAreaStep
                    minimumY: -parent.inputAreaStep
                    maximumX: root.parent ? root.maximumX : 0
                    maximumY: root.parent ? root.maximumY + parent.inputAreaStep : 0
                }
            }

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

            TextInput {
                id: addressLine

                anchors {
                    fill: parent
                    leftMargin: parent.height + parent.height + hifi.layout.spacing * 5
                    rightMargin: hifi.layout.spacing * 2
                    topMargin: parent.inputAreaStep + hifi.layout.spacing
                    bottomMargin: parent.inputAreaStep + hifi.layout.spacing

                }

                font.pixelSize: hifi.fonts.pixelSize * root.scale * 0.75

                helperText: "Go to: place, @user, /path, network address"

                onAccepted: {
                    event.accepted = true  // Generates erroneous error in program log, "ReferenceError: event is not defined".
                    addressBarDialog.loadAddress(addressLine.text)
                }
            }

        }
    }

    onEnabledChanged: {
        if (enabled) {
            addressLine.forceActiveFocus()
        }
    }
    
    Timer {
        running: root.enabled
        interval: 500
        repeat: true
        onTriggered: {
            if (root.enabled && !addressLine.activeFocus) {
                addressLine.forceActiveFocus()
            }
        }
    }

    onVisibleChanged: {
        if (!visible) {
            addressLine.text = ""
        }
    }

    function toggleOrGo() {
        if (addressLine.text == "") {
            enabled = false
        } else {
            addressBarDialog.loadAddress(addressLine.text)
        }
    }

    Keys.onPressed: {
        switch (event.key) {
            case Qt.Key_Escape:
            case Qt.Key_Back:
                if (enabled) {
                    enabled = false
                    event.accepted = true
                }
                break
            case Qt.Key_Enter:
            case Qt.Key_Return:
                toggleOrGo()
                event.accepted = true
                break
        }
    }
}
