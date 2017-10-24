
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

import QtQuick 2.7
import QtQuick.Controls 2.2 as QQControls
import QtQuick.Layouts 1.3

import QtWebEngine 1.5
import QtWebChannel 1.0

import "../styles-uit"
import "../controls-uit" as HifiControls
import "../windows"
import "../controls"

import HifiWeb 1.0

Rectangle {
    id: root;

    HifiConstants { id: hifi; }

    property string title: "";
    signal sendToScript(var message);
    property bool keyboardEnabled: true  // FIXME - Keyboard HMD only: Default to false
    property bool keyboardRaised: false
    property bool punctuationMode: false
    property var suggestionsList: []

    OpenSearchEngine {
        id: searchEngine
        name: "Google";
        searchUrlTemplate: "https://www.google.com/search?client=hifibrowser&q={searchTerms}";
        suggestionsUrlTemplate: "https://suggestqueries.google.com/complete/search?output=firefox&q=%s";
        suggestionsUrl: "https://suggestqueries.google.com/complete/search?output=firefox&q=%s";

        onSuggestions: {
            if (suggestions.length > 0) {
                suggestionsList = []
                suggestionsList.push(addressBarInput.text) //do not overwrite edit text
                for(var i = 0; i < suggestions.length; i++) {
                    suggestionsList.push(suggestions[i])
                }
                addressBar.model = suggestionsList
                if (!addressBar.popup.visible) {
                    addressBar.popup.open()
                }
            }
        }
    }

    Timer {
        id: suggestionRequestTimer
        interval: 200
        repeat: false
        onTriggered: {
            if (addressBar.editText !== "") {
                searchEngine.requestSuggestions(addressBarInput.text)
            }
        }
    }

    color: hifi.colors.baseGray;

    function goTo(url) {
        //must be valid attempt to open an site with dot

        if (url.indexOf(".") > 0) {
            if (url.indexOf("http") < 0) {
                url = "http://" + url;
            }
        } else {
            url = searchEngine.searchUrl(url)
        }

        addressBar.model = []
        webEngineView.url = url
        suggestionRequestTimer.stop()
        addressBar.popup.close()
    }

    Column {
        spacing: 2
        width: parent.width;

        RowLayout {
            id: addressBarRow
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

            QQControls.ComboBox {
                id: addressBar

                //selectByMouse: true
                focus: true

                editable: true
                //flat: true
                indicator: Item {}
                background: Item {}
                onActivated: {
                    goTo(textAt(index))
                }

                popup.height: webEngineView.height

                onFocusChanged: {
                    if (focus) {
                        addressBarInput.selectAll()
                    }
                }

                contentItem: QQControls.TextField {
                    id: addressBarInput
                    leftPadding: 26
                    rightPadding: hifi.dimensions.controlLineHeight
                    text: addressBar.editText
                    placeholderText: qsTr("Enter URL")
                    font: addressBar.font
                    selectByMouse: true
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    onFocusChanged: {
                        if (focus) {
                            selectAll()
                        }
                    }

                    Keys.onDeletePressed: {
                        addressBarInput.text = ""
                    }

                    Keys.onPressed: {
                        if (event.key === Qt.Key_Return) {
                            goTo(addressBarInput.text)
                            event.accepted = true;
                        }
                    }

                    Image {
                        anchors.verticalCenter: parent.verticalCenter;
                        x: 5
                        z: 2
                        id: faviconImage
                        width: 16; height: 16
                        sourceSize: Qt.size(width, height)
                        source: webEngineView.icon
                        onSourceChanged: console.log("web icon", source)
                    }

                    HifiControls.WebGlyphButton {
                        glyph: webEngineView.loading ? hifi.glyphs.closeSmall : hifi.glyphs.reloadSmall;
                        anchors.verticalCenter: parent.verticalCenter;
                        width: hifi.dimensions.controlLineHeight
                        z: 2
                        x: addressBarInput.width - implicitWidth
                        onClicked: {
                            if (webEngineView.loading) {
                                webEngineView.stop()
                            } else {
                                reloadTimer.start()
                            }
                        }
                    }
                }

                Component.onCompleted: ScriptDiscoveryService.scriptsModelFilter.filterRegExp = new RegExp("^.*$", "i")

                Keys.onPressed: {
                    if (event.key === Qt.Key_Return) {
                        goTo(addressBarInput.text)
                        event.accepted = true;
                    }
                }

                onEditTextChanged: {
                    if (addressBar.editText !== "" && addressBar.editText !== webEngineView.url.toString()) {
                        suggestionRequestTimer.restart();
                    } else {
                        addressBar.model = []
                        addressBar.popup.close()
                    }

                }

                Layout.fillWidth: true
                editText: webEngineView.url
                onAccepted: goTo(addressBarInput.text)
            }

            HifiControls.WebGlyphButton {
                checkable: true
                checked: webEngineView.audioMuted
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
            background: Rectangle {
                implicitHeight: 2
                color: "#6A6A6A"
            }

            contentItem: Item {
                implicitHeight: 2

                Rectangle {
                    width: loadProgressBar.visualPosition * parent.width
                    height: parent.height
                    color: "#00B4EF"
                }
            }

            width: parent.width;
            from: 0
            to: 100
            value: webEngineView.loadProgress
            height: 2
        }

        Component {
            id: webDialogComponent
            Rectangle {
                property alias webDialogView: webDialogView
                color: "white"
                HifiControls.BaseWebView {
                    id: webDialogView
                    anchors.fill: parent

                    settings.autoLoadImages: true
                    settings.javascriptEnabled: true
                    settings.errorPageEnabled: true
                    settings.pluginsEnabled: true
                    settings.fullScreenSupportEnabled: true
                    settings.autoLoadIconsForPage: true
                    settings.touchIconsEnabled: true

                    onWindowCloseRequested: {
                        webDialog.active = false
                        webDialog.request = null
                    }
                }
            }
        }

        Item {
            width: parent.width;
            property real webViewHeight: root.height - loadProgressBar.height - 48 - 4
            height: keyboardEnabled && keyboardRaised ? webViewHeight - keyboard.height : webViewHeight

            HifiControls.BaseWebView {
                id: webEngineView
                anchors.fill: parent

                focus: true
                objectName: "tabletWebEngineView"

                url: "https://www.highfidelity.com"

                //profile: HFWebEngineProfile;

                property string userScriptUrl: ""

                onLoadingChanged: {
                    if (!loading) {
                        suggestionRequestTimer.stop()
                        addressBar.popup.close()
                    }
                }

                onLinkHovered: {
                    //TODO: change cursor shape?
                    console.error("hoveredUrl:", hoveredUrl)
                }

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
                settings.fullScreenSupportEnabled: true
                settings.autoLoadIconsForPage: true
                settings.touchIconsEnabled: true

                onCertificateError: {
                    error.defer();
                }

                Component.onCompleted: {
                    webChannel.registerObject("eventBridge", eventBridge);
                    webChannel.registerObject("eventBridgeWrapper", eventBridgeWrapper);
                    //webEngineView.profile.httpUserAgent = "Mozilla/5.0 Chrome (HighFidelityInterface)";
                }

                onFeaturePermissionRequested: {
                    grantFeaturePermission(securityOrigin, feature, true);
                }

                onNewViewRequested: {
                    console.error("new view requested:", request.destination)
                    if (request.destination == WebEngineView.NewViewInDialog) {
                        webDialog.request = request
                        webDialog.active = true
                    } else {
                        request.openIn(webEngineView);
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

                    console.error("Render process exited with code " + exitCode + " " + status);
                    reloadTimer.running = true;
                }

                onFullScreenRequested: {
                    console.error("FS requested:", request.destination)
                    if (request.toggleOn) {
                        webEngineView.state = "FullScreen";
                    } else {
                        webEngineView.state = "";
                    }
                    request.accept();
                }

                onWindowCloseRequested: {
                    console.error("window close requested:", request.destination)
                }

                Timer {
                    id: reloadTimer
                    interval: 0
                    running: false
                    repeat: false
                    onTriggered: webEngineView.reload()
                }
            }

            Loader {
                id: webDialog
                property WebEngineNewViewRequest request: null
                anchors.fill: parent
                anchors.margins: 10

                active: false
                sourceComponent: webDialogComponent
                onStatusChanged: {
                    if (Loader.Ready === status) {
                        focus = true
                        item.webDialogView.profile = webEngineView.profile
                        request.openIn(item.webDialogView)
                    }
                }
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
