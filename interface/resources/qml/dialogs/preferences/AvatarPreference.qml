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

import "../../controls-uit"
import "../../hifi/tablet/tabletWindows/preferences"

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
            label: root.label
            placeholderText: root.placeholderText
            text: preference.value
            colorScheme: dataTextField.acceptableInput ? hifi.colorSchemes.dark : hifi.colorSchemes.light
            validator: RegExpValidator {
                regExp: /.*\.(?:fst).*\?*/ig
            }
            anchors {
                left: parent.left
                right: parent.right
                bottom: bookmarkAvatarButton.top
                bottomMargin: hifi.dimensions.contentSpacing.y
            }
        }

        QueuedButton {
            id: bookmarkAvatarButton
            text: "Bookmark Avatar"
            width: 140
            visible: dataTextField.acceptableInput
            anchors {
                left: parent.left
                bottom: parent.bottom
                rightMargin: hifi.dimensions.contentSpacing.x
            }
            onClickedQueued: ApplicationInterface.loadAddAvatarBookmarkDialog()
        }

        Button {
            id: browseAvatarsButton
            text: "Browse Avatars"
            width: 140
            anchors {
                left: dataTextField.acceptableInput ? bookmarkAvatarButton.right : parent.left
                bottom: parent.bottom
                leftMargin: dataTextField.acceptableInput ? hifi.dimensions.contentSpacing.x : 0
            }
            onClicked: {
                if (typeof desktop !== "undefined") {
                    // Load dialog via OffscreenUi so that JavaScript EventBridge is available.
                    root.browser = OffscreenUi.load("dialogs/preferences/AvatarBrowser.qml");
                    root.browser.windowDestroyed.connect(function(){
                        root.browser = null;
                    });
                } else {
                    root.browser = tabletAvatarBrowserBuilder.createObject(tabletRoot);

                    // Make dialog modal.
                    tabletRoot.openModal = root.browser;
                }
            }
        }

        Component {
            id: tabletAvatarBrowserBuilder;
            TabletAvatarBrowser { }
        }

    }
}
