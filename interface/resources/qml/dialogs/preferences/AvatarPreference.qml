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
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: dataTextField.controlHeight + bookmarkAvatarButton.height + hifi.dimensions.contentSpacing.y

        TextField {
            id: dataTextField
            placeholderText: root.placeholderText
            text: preference.value
            label: root.label
            anchors {
                left: parent.left
                right: parent.right
                bottom: bookmarkAvatarButton.top
                bottomMargin: hifi.dimensions.contentSpacing.y
            }
            colorScheme: hifi.colorSchemes.dark
        }

        QueuedButton {
            id: bookmarkAvatarButton
            text: "Bookmark Avatar"
            width: 140
            anchors {
                left: parent.left
                bottom: parent.bottom
                rightMargin: hifi.dimensions.contentSpacing.x
            }
            onClickedQueued: avatarBookmarks.addBookmark()
        }

        Button {
            id: browseAvatarsButton
            text: "Browse Avatars"
            width: 140
            anchors {
                left: bookmarkAvatarButton.right
                bottom: parent.bottom
                leftMargin: hifi.dimensions.contentSpacing.x
            }
            onClicked: {
                // Load dialog via OffscreenUi so that JavaScript EventBridge is available.
                root.browser = OffscreenUi.load("dialogs/preferences/AvatarBrowser.qml");
                root.browser.windowDestroyed.connect(function(){
                    root.browser = null;
                });
            }
        }

    }
}
