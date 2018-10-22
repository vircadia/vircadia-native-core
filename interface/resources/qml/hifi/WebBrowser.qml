
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
import QtGraphicalEffects 1.0

import QtWebEngine 1.5
import QtWebChannel 1.0

import stylesUit 1.0
import controlsUit 1.0 as HifiControls
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
    readonly property string searchUrlTemplate: "https://www.google.com/search?client=hifibrowser&q=";


    WebBrowserSuggestionsEngine {
        id: searchEngine

        onSuggestions: {
            if (suggestions.length > 0) {
                suggestionsList = []
                suggestionsList.push(addressBarInput.text); //do not overwrite edit text
                for(var i = 0; i < suggestions.length; i++) {
                    suggestionsList.push(suggestions[i]);
                }
                addressBar.model = suggestionsList
                if (!addressBar.popup.visible) {
                    addressBar.popup.open();
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
                searchEngine.querySuggestions(addressBarInput.text);
            }
        }
    }

    color: hifi.colors.baseGray;

    function goTo(url) {
        //must be valid attempt to open an site with dot
        var urlNew = url
        if (url.indexOf(".") > 0) {
            if (url.indexOf("http") < 0) {
                urlNew = "http://" + url;
            }
        } else {
            urlNew = searchUrlTemplate + url
        }

        addressBar.model = []
        //need to rebind if binfing was broken by selecting from suggestions
        addressBar.editText = Qt.binding( function() { return webStack.currentItem.webEngineView.url; });
        webStack.currentItem.webEngineView.url = urlNew
        suggestionRequestTimer.stop();
        addressBar.popup.close();
    }

    Column {
        spacing: 2
        width: parent.width;

        RowLayout {
            id: addressBarRow
            width: parent.width;
            height: 48

            HifiControls.WebGlyphButton {
                enabled: webStack.currentItem.webEngineView.canGoBack || webStack.depth > 1
                glyph: hifi.glyphs.backward;
                anchors.verticalCenter: parent.verticalCenter;
                size: 38;
                onClicked: {
                    if (webStack.currentItem.webEngineView.canGoBack) {
                        webStack.currentItem.webEngineView.goBack();
                    } else if (webStack.depth > 1) {
                        webStack.pop();
                    }
                }
            }

            HifiControls.WebGlyphButton {
                enabled: webStack.currentItem.webEngineView.canGoForward
                glyph: hifi.glyphs.forward;
                anchors.verticalCenter: parent.verticalCenter;
                size: 38;
                onClicked: {
                    webStack.currentItem.webEngineView.goForward();
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
                    goTo(textAt(index));
                }

                onHighlightedIndexChanged: {
                    if (highlightedIndex >= 0) {
                        addressBar.editText = textAt(highlightedIndex)
                    }
                }

                popup.height: webStack.height

                onFocusChanged: {
                    if (focus) {
                        addressBarInput.selectAll();
                    }
                }

                contentItem: QQControls.TextField {
                    id: addressBarInput
                    leftPadding: 26
                    rightPadding: hifi.dimensions.controlLineHeight + 5
                    text: addressBar.editText
                    placeholderText: qsTr("Enter URL")
                    font: addressBar.font
                    selectByMouse: true
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    onFocusChanged: {
                        if (focus) {
                            selectAll();
                        }
                    }

                    Keys.onDeletePressed: {
                        addressBarInput.text = ""
                    }

                    Keys.onPressed: {
                        if (event.key === Qt.Key_Return) {
                            goTo(addressBarInput.text);
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
                        source: webStack.currentItem.webEngineView.icon
                    }

                    HifiControls.WebGlyphButton {
                        glyph: webStack.currentItem.webEngineView.loading ? hifi.glyphs.closeSmall : hifi.glyphs.reloadSmall;
                        anchors.verticalCenter: parent.verticalCenter;
                        width: hifi.dimensions.controlLineHeight
                        z: 2
                        x: addressBarInput.width - implicitWidth
                        onClicked: {
                            if (webStack.currentItem.webEngineView.loading) {
                                webStack.currentItem.webEngineView.stop();
                            } else {
                                webStack.currentItem.reloadTimer.start();
                            }
                        }
                    }
                }

                Component.onCompleted: ScriptDiscoveryService.scriptsModelFilter.filterRegExp = new RegExp("^.*$", "i");

                Keys.onPressed: {
                    if (event.key === Qt.Key_Return) {
                        goTo(addressBarInput.text);
                        event.accepted = true;
                    }
                }

                onEditTextChanged: {
                    if (addressBar.editText !== "" && addressBar.editText !== webStack.currentItem.webEngineView.url.toString()) {
                        suggestionRequestTimer.restart();
                    } else {
                        addressBar.model = []
                        addressBar.popup.close();
                    }

                }

                Layout.fillWidth: true
                editText: webStack.currentItem.webEngineView.url
                onAccepted: goTo(addressBarInput.text);
            }

            HifiControls.WebGlyphButton {
                checkable: true
                checked: webStack.currentItem.webEngineView.audioMuted
                glyph: checked ? hifi.glyphs.unmuted : hifi.glyphs.muted
                anchors.verticalCenter: parent.verticalCenter;
                width: hifi.dimensions.controlLineHeight
                onClicked: {
                    webStack.currentItem.webEngineView.audioMuted = !webStack.currentItem.webEngineView.audioMuted
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
            value: webStack.currentItem.webEngineView.loadProgress
            height: 2
        }

        Component {
            id: webViewComponent
            Rectangle {
                property alias webEngineView: webEngineView
                property alias reloadTimer: reloadTimer

                property WebEngineNewViewRequest request: null

                property bool isDialog: QQControls.StackView.index > 0
                property real margins: isDialog ? 10 : 0

                color: "#d1d1d1"

                QQControls.StackView.onActivated: {
                    addressBar.editText = Qt.binding( function() { return webStack.currentItem.webEngineView.url; });
                }

                onRequestChanged: {
                    if (isDialog && request !== null && request !== undefined) {//is Dialog ?
                        request.openIn(webEngineView);
                    }
                }

                HifiControls.BaseWebView {
                    id: webEngineView
                    anchors.fill: parent
                    anchors.margins: parent.margins

                    layer.enabled: parent.isDialog
                    layer.effect: DropShadow {
                        verticalOffset: 8
                        horizontalOffset: 8
                        color: "#330066ff"
                        samples: 10
                        spread: 0.5
                    }

                    focus: true
                    objectName: "tabletWebEngineView"

                    //profile: HFWebEngineProfile;
                    profile.httpUserAgent: "Mozilla/5.0 (Android; Mobile; rv:13.0) Gecko/13.0 Firefox/13.0"

                    property string userScriptUrl: ""

                    onLoadingChanged: {
                        if (!loading) {
                            addressBarInput.cursorPosition = 0 //set input field cursot to beginning
                            suggestionRequestTimer.stop();
                            addressBar.popup.close();
                        }
                    }

                    onLinkHovered: {
                        //TODO: change cursor shape?
                    }

                    // creates a global EventBridge object.
                    WebEngineScript {
                        id: createGlobalEventBridge
                        sourceCode: eventBridgeJavaScriptToInject
                        injectionPoint: WebEngineScript.Deferred
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
                    }

                    onFeaturePermissionRequested: {
                        grantFeaturePermission(securityOrigin, feature, true);
                    }

                    onNewViewRequested: {
                        if (request.destination == WebEngineView.NewViewInDialog) {
                            webStack.push(webViewComponent, {"request": request});
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
                        if (request.toggleOn) {
                            webEngineView.state = "FullScreen";
                        } else {
                            webEngineView.state = "";
                        }
                        request.accept();
                    }

                    onWindowCloseRequested: {
                        webStack.pop();
                    }
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

        QQControls.StackView {
            id: webStack
            width: parent.width;
            property real webViewHeight: root.height - loadProgressBar.height - 48 - 4
            height: keyboardEnabled && keyboardRaised ? webViewHeight - keyboard.height : webViewHeight

            Component.onCompleted: webStack.push(webViewComponent, {"webEngineView.url": "https://www.highfidelity.com"});
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
