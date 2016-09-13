//
//  AvatarPreference.qml
//
//  Created by Bradley Austin Davis on 22 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

import "../../dialogs"
import "../../controls-uit"
import "../../controls" as Controls

Preference {
    id: root
    property alias text: dataTextField.text
    property alias buttonText: button.text
    property alias placeholderText: dataTextField.placeholderText
    property var browser;
    height: control.height + hifi.dimensions.controlInterlineHeight

    Component.onCompleted: {
        dataTextField.text = preference.value;
        console.log("MyAvatar modelName " + MyAvatar.getFullAvatarModelName())
        console.log("Application : " + ApplicationInterface)
        ApplicationInterface.fullAvatarURLChanged.connect(processNewAvatar);
    }

    Component.onDestruction: {
        ApplicationInterface.fullAvatarURLChanged.disconnect(processNewAvatar);
    }

    function processNewAvatar(url, modelName) {
        if (browser) {
            browser.destroy();
            browser = null
        }

        dataTextField.text = url;
    }

    function save() {
        preference.value = dataTextField.text;
        preference.save();
    }

    // Restores the original avatar URL
    function restore() {
        preference.save();
    }

    Item {
        id: control

        property bool keyboardRaised: false
        property bool punctuationMode: false

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: Math.max(dataTextField.controlHeight, button.height) + (keyboardRaised ? 200 : 0)

        TextField {
            id: dataTextField
            placeholderText: root.placeholderText
            text: preference.value
            label: root.label
            anchors {
                left: parent.left
                right: button.left
                rightMargin: hifi.dimensions.contentSpacing.x
                bottom: keyboard1.top
            }
            colorScheme: hifi.colorSchemes.dark
        }

        Component {
            id: avatarBrowserBuilder;
            AvatarBrowser { }
        }

        Button {
            id: button
            text: "Browse"
            anchors {
                right: parent.right
                verticalCenter: dataTextField.verticalCenter
            }
            onClicked: {
                root.browser = avatarBrowserBuilder.createObject(desktop);
                root.browser.windowDestroyed.connect(function(){
                    root.browser = null;
                })
            }
        }

        // virtual keyboard, letters
        Controls.Keyboard {
            id: keyboard1
            // y: parent.keyboardRaised ? parent.height : 0
            height: parent.keyboardRaised ? 200 : 0
            visible: parent.keyboardRaised && !parent.punctuationMode
            enabled: parent.keyboardRaised && !parent.punctuationMode
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            // anchors.bottomMargin: 2 * hifi.dimensions.contentSpacing.y
        }

        Controls.KeyboardPunctuation {
            id: keyboard2
            // y: parent.keyboardRaised ? parent.height : 0
            height: parent.keyboardRaised ? 200 : 0
            visible: parent.keyboardRaised && parent.punctuationMode
            enabled: parent.keyboardRaised && parent.punctuationMode
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            // anchors.bottomMargin: 2 * hifi.dimensions.contentSpacing.y
        }
    }
}
