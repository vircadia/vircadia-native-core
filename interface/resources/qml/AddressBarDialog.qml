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
import QtQuick 2.3
import QtQuick.Controls 1.2
import "controls"
import "styles"

Item {
    id: root
    HifiConstants { id: hifi }

    objectName: "AddressBarDialog"

    property int animationDuration: hifi.effects.fadeInDuration
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

            TextInput {
                id: addressLine

                anchors {
                    fill: parent
                    leftMargin: parent.height + hifi.layout.spacing * 2
                    rightMargin: hifi.layout.spacing * 2
                    topMargin: parent.inputAreaStep + hifi.layout.spacing
                    bottomMargin: parent.inputAreaStep + hifi.layout.spacing

                }

                font.pixelSize: hifi.fonts.pixelSize * root.scale

                helperText: "Go to: place, @user, /path, network address"

                onAccepted: {
                    event.accepted = true  // Generates erroneous error in program log, "ReferenceError: event is not defined".
                    addressBarDialog.loadAddress(addressLine.text)
                }
            }

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
        }
    }

    // The UI enables an object, rather than manipulating its visibility, so that we can do animations in both directions. 
    // Because visibility and enabled are booleans, they cannot be animated. So when enabled is changed, we modify a property 
    // that can be animated, like scale or opacity, and then when the target animation value is reached, we can modify the 
    // visibility.
    enabled: false
    opacity: 0.0

    onEnabledChanged: {
        opacity = enabled ? 1.0 : 0.0
        if (enabled) {
            addressLine.forceActiveFocus();
        }
    }

    Behavior on opacity {
        // Animate opacity.
        NumberAnimation {
            duration: animationDuration
            easing.type: Easing.OutCubic
        }
    }

    onOpacityChanged: {
        // Once we're transparent, disable the dialog's visibility.
        visible = (opacity != 0.0)
    }

    onVisibleChanged: {
        if (!visible) {
            reset()

            // Some dialogs should be destroyed when they become invisible.
            if (destroyOnInvisible) {
                destroy()
            }
        }

    }

    function reset() {
        addressLine.text = ""
    }

    function toggleOrGo() {
        if (addressLine.text == "") {
            enabled = false
        } else {
            addressBarDialog.loadAddress(addressLine.text)
        }
    }

    Keys.onEscapePressed: {
        enabled = false
    }

    Keys.onPressed: {
        switch(event.key) {
            case Qt.Key_W:
                if (event.modifiers == Qt.ControlModifier) {
                    event.accepted = true
                    enabled = false
                }
                break
        }
    }

    Keys.onReturnPressed: toggleOrGo()
    Keys.onEnterPressed: toggleOrGo()
}
