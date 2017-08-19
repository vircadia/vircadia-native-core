//
//  WebBrowser.qml
//
//
//  Created by Vlad Stelmahovsky on 06/22/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.5 as QQControls
import QtQuick.Layouts 1.3
import QtQuick.Controls.Styles 1.4

import QtWebEngine 1.2
import QtWebChannel 1.0

import "../styles-uit"
import "../controls-uit" as HifiControls
import "../windows"
import "../controls"

Rectangle {
    id: root;

    HifiConstants { id: hifi; }

    property string title: "";
    signal sendToScript(var message);
    property bool keyboardEnabled: true  // FIXME - Keyboard HMD only: Default to false
    property bool keyboardRaised: false
    property bool punctuationMode: false


    color: hifi.colors.baseGray;

    // only show the title if loaded through a "loader"

    Column {
        spacing: 2
        width: parent.width;

        RowLayout {
            width: parent.width;
            height: 48

            HifiControls.WebGlyphButton {
                enabled: webEngineView.canGoBack
                glyph: hifi.glyphs.backward;
                anchors.verticalCenter: parent.verticalCenter;
                size: 38;
                onClicked: {
                    webEngineView.goBack()
                }
            }

            HifiControls.WebGlyphButton {
                enabled: webEngineView.canGoForward
                glyph: hifi.glyphs.forward;
                anchors.verticalCenter: parent.verticalCenter;
                size: 38;
                onClicked: {
                    webEngineView.goForward()
                }
            }

            QQControls.TextField {
                id: addressBar

                Image {
                    anchors.verticalCenter: addressBar.verticalCenter;
                    x: 5
                    z: 2
                    id: faviconImage
                    width: 16; height: 16
                    sourceSize: Qt.size(width, height)
                    source: webEngineView.icon
                }

                HifiControls.WebGlyphButton {
                    glyph: webEngineView.loading ? hifi.glyphs.closeSmall : hifi.glyphs.reloadSmall;
                    anchors.verticalCenter: parent.verticalCenter;
                    width: hifi.dimensions.controlLineHeight
                    z: 2
                    x: addressBar.width - 28
                    onClicked: {
                        if (webEngineView.loading) {
                            webEngineView.stop()
                        } else {
                            reloadTimer.start()
                        }
                    }
                }

                style: TextFieldStyle {
                    padding {
                        left: 26;
                        right: 26
                    }
                }
                focus: true
                Layout.fillWidth: true
                text: webEngineView.url
                onAccepted: webEngineView.url = text
            }
            HifiControls.WebGlyphButton {
                checkable: true
                //only QtWebEngine 1.3
                //checked: webEngineView.audioMuted
                glyph: checked ? hifi.glyphs.unmuted : hifi.glyphs.muted
                anchors.verticalCenter: parent.verticalCenter;
                width: hifi.dimensions.controlLineHeight
                onClicked: {
                    webEngineView.triggerWebAction(WebEngineView.ToggleMediaMute)
                }
            }
        }

        QQControls.ProgressBar {
            id: loadProgressBar
            style: ProgressBarStyle {
                background: Rectangle {
                    color: "#6A6A6A"
                }
                progress: Rectangle{
                    color: "#00B4EF"
                }
            }

            width: parent.width;
            minimumValue: 0
            maximumValue: 100
            value: webEngineView.loadProgress
            height: 2
        }

        HifiControls.BaseWebView {
            id: webEngineView
            focus: true
            objectName: "tabletWebEngineView"

            url: "http://www.highfidelity.com"
            property real webViewHeight: root.height - loadProgressBar.height - 48 - 4

            width: parent.width;
            height: keyboardEnabled && keyboardRaised ? webViewHeight - keyboard.height : webViewHeight

            profile: HFWebEngineProfile;

            property string userScriptUrl: ""

            // creates a global EventBridge object.
            WebEngineScript {
                id: createGlobalEventBridge
                sourceCode: eventBridgeJavaScriptToInject
                injectionPoint: WebEngineScript.DocumentCreation
                worldId: WebEngineScript.MainWorld
            }

            // detects when to raise and lower virtual keyboard
            WebEngineScript {
                id: raiseAndLowerKeyboard
                injectionPoint: WebEngineScript.Deferred
                sourceUrl: resourceDirectoryUrl + "/html/raiseAndLowerKeyboard.js"
                worldId: WebEngineScript.MainWorld
            }

            // User script.
            WebEngineScript {
                id: userScript
                sourceUrl: webEngineView.userScriptUrl
                injectionPoint: WebEngineScript.DocumentReady  // DOM ready but page load may not be finished.
                worldId: WebEngineScript.MainWorld
            }

            userScripts: [ createGlobalEventBridge, raiseAndLowerKeyboard, userScript ]

            settings.autoLoadImages: true
            settings.javascriptEnabled: true
            settings.errorPageEnabled: true
            settings.pluginsEnabled: true
            settings.fullScreenSupportEnabled: false
            //from WebEngine 1.3
            //            settings.autoLoadIconsForPage: false
            //            settings.touchIconsEnabled: false

            onCertificateError: {
                error.defer();
            }

            Component.onCompleted: {
                webChannel.registerObject("eventBridge", eventBridge);
                webChannel.registerObject("eventBridgeWrapper", eventBridgeWrapper);
                webEngineView.profile.httpUserAgent = "Mozilla/5.0 Chrome (HighFidelityInterface)";
            }

            onFeaturePermissionRequested: {
                grantFeaturePermission(securityOrigin, feature, true);
            }

            onNewViewRequested: {
                if (!request.userInitiated) {
                    print("Warning: Blocked a popup window.");
                }
            }

            onRenderProcessTerminated: {
                var status = "";
                switch (terminationStatus) {
                case WebEngineView.NormalTerminationStatus:
                    status = "(normal exit)";
                    break;
                case WebEngineView.AbnormalTerminationStatus:
                    status = "(abnormal exit)";
                    break;
                case WebEngineView.CrashedTerminationStatus:
                    status = "(crashed)";
                    break;
                case WebEngineView.KilledTerminationStatus:
                    status = "(killed)";
                    break;
                }

                print("Render process exited with code " + exitCode + " " + status);
                reloadTimer.running = true;
            }

            onWindowCloseRequested: {
            }

            Timer {
                id: reloadTimer
                interval: 0
                running: false
                repeat: false
                onTriggered: webEngineView.reload()
            }
        }
    }

    HifiControls.Keyboard {
        id: keyboard
        raised: parent.keyboardEnabled && parent.keyboardRaised
        numeric: parent.punctuationMode
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }
}
