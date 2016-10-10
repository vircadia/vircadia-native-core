//
//  AvatarBrowser.qml
//
//  Created by Bradley Austin Davis on 30 Aug 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtWebChannel 1.0
import QtWebEngine 1.2

import "../../windows"
import "../../controls-uit"
import "../../styles-uit"

Window {
    id: root
    HifiConstants { id: hifi }
    width: 900; height: 700
    resizable: true
    modality: Qt.ApplicationModal

    property alias eventBridge: eventBridgeWrapper.eventBridge

    Item {
        anchors.fill: parent

        property bool keyboardRaised: true
        property bool punctuationMode: false

        BaseWebView {
            id: webview
            url: "https://metaverse.highfidelity.com/marketplace?category=avatars"
            focus: true

            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                bottom: keyboard.top
            }

            property alias eventBridgeWrapper: eventBridgeWrapper

            QtObject {
                id: eventBridgeWrapper
                WebChannel.id: "eventBridgeWrapper"
                property var eventBridge;
            }

            webChannel.registeredObjects: [eventBridgeWrapper]

            // Create a global EventBridge object for raiseAndLowerKeyboard.
            WebEngineScript {
                id: createGlobalEventBridge
                sourceCode: eventBridgeJavaScriptToInject
                injectionPoint: WebEngineScript.DocumentCreation
                worldId: WebEngineScript.MainWorld
            }

            // Detect when may want to raise and lower keyboard.
            WebEngineScript {
                id: raiseAndLowerKeyboard
                injectionPoint: WebEngineScript.Deferred
                sourceUrl: resourceDirectoryUrl + "html/raiseAndLowerKeyboard.js"
                worldId: WebEngineScript.MainWorld
            }

            userScripts: [ createGlobalEventBridge, raiseAndLowerKeyboard ]

        }

        Keyboard {
            id: keyboard
            raised: parent.keyboardRaised
            numeric: parent.punctuationMode
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
        }
    }
}
