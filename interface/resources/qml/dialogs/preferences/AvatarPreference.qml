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

Preference {
    id: root
    property alias text: dataTextField.text
    property alias buttonText: button.text
    property alias placeholderText: dataTextField.placeholderText
    property real spacing: 0
    property var browser;
    height: Math.max(dataTextField.controlHeight, button.height) + spacing

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

    TextField {
        id: dataTextField
        placeholderText: root.placeholderText
        text: preference.value
        label: root.label
        anchors {
            left: parent.left
            right: button.left
            rightMargin: hifi.dimensions.contentSpacing.x
            bottomMargin: spacing
        }
        colorScheme: hifi.colorSchemes.dark
    }

    Component {
        id: avatarBrowserBuilder;
        AvatarBrowser { }
    }

    Button {
        id: button
        anchors { right: parent.right; verticalCenter: dataTextField.verticalCenter }
        text: "Browse"
        onClicked: {
            root.browser = avatarBrowserBuilder.createObject(desktop);
            root.browser.windowDestroyed.connect(function(){
                root.browser = null;
            })
        }
    }
}
