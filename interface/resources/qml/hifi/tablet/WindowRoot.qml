//
//  WindowRoot.qml
//
//  Created by Anthony Thibault on 14 Feb 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  This qml is used when tablet content is shown on the 2d overlay ui
//  TODO: FIXME: this is practically identical to TabletRoot.qml

import "../../windows" as Windows
import QtQuick 2.0
import Hifi 1.0

import Qt.labs.settings 1.0

Windows.ScrollingWindow {
    id: tabletRoot
    objectName: "tabletRoot"
    property string username: "Unknown user"

    property var rootMenu;
    property string subMenu: ""

    shown: false
    resizable: false

    Settings {
        id: settings
        category: "WindowRoot.Windows"
        property real width: 480
        property real height: 706
    }

    onResizableChanged: {
        if (!resizable) {
            // restore default size
            settings.width = tabletRoot.width
            settings.height = tabletRoot.height
            tabletRoot.width = 480
            tabletRoot.height = 706
        } else {
            tabletRoot.width = settings.width
            tabletRoot.height = settings.height
        }
    }

    signal showDesktop();

    function setResizable(value) {
        tabletRoot.resizable = value;
    }

    function setMenuProperties(rootMenu, subMenu) {
        tabletRoot.rootMenu = rootMenu;
        tabletRoot.subMenu = subMenu;
    }

    function loadSource(url) {
        loader.source = "";  // make sure we load the qml fresh each time.
        loader.source = url;
    }

    function loadWebBase() {
        loader.source = "";
        loader.source = "WindowWebView.qml";
    }

    function loadTabletWebBase() {
        loader.source = "";
        loader.source = "./BlocksWebView.qml";
    }

    function loadWebUrl(url, injectedJavaScriptUrl) {
        loader.item.url = url;
        loader.item.scriptURL = injectedJavaScriptUrl;
        if (loader.item.hasOwnProperty("closeButtonVisible")) {
            loader.item.closeButtonVisible = false;
        }
    }

    // used to send a message from qml to interface script.
    signal sendToScript(var message);

    // used to receive messages from interface script
    function fromScript(message) {
        if (loader.item !== null) {
            if (loader.item.hasOwnProperty("fromScript")) {
                loader.item.fromScript(message);
            }
        }
    }

    SoundEffect {
        id: buttonClickSound
        volume: 0.1
        source: "../../../sounds/Gamemaster-Audio-button-click.wav"
    }

    function playButtonClickSound() {
        // Because of the asynchronous nature of initalization, it is possible for this function to be
        // called before the C++ has set the globalPosition context variable.
        if (typeof globalPosition !== 'undefined') {
            buttonClickSound.play(globalPosition);
        }
    }

    function setUsername(newUsername) {
        username = newUsername;
    }

    Loader {
        id: loader
        objectName: "loader"
        asynchronous: false

        height: pane.scrollHeight
        width: pane.contentWidth
        anchors.left: parent.left
        anchors.top: parent.top

        // Hook up callback for clara.io download from the marketplace.
        Connections {
            id: eventBridgeConnection
            target: eventBridge
            onWebEventReceived: {
                if (message.slice(0, 17) === "CLARA.IO DOWNLOAD") {
                    ApplicationInterface.addAssetToWorldFromURL(message.slice(18));
                }
            }
        }

        onLoaded: {
            if (loader.item.hasOwnProperty("sendToScript")) {
                loader.item.sendToScript.connect(tabletRoot.sendToScript);
            }
            if (loader.item.hasOwnProperty("setRootMenu")) {
                loader.item.setRootMenu(tabletRoot.rootMenu, tabletRoot.subMenu);
            }
            loader.item.forceActiveFocus();
        }
    }

    implicitWidth: 480
    implicitHeight: 706
}
