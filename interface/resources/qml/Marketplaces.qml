//
//  Marketplaces.qml
//
//  Created by Elisa Lupin-Jimenez on 3 Aug 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtWebChannel 1.0
import QtWebEngine 1.2
import QtWebSockets 1.0

import "controls"
import "controls-uit" as Controls
import "styles"
import "styles-uit"


Rectangle {
    HifiConstants { id: hifi }
    id: marketplace
    anchors.fill: parent

    property var marketplacesUrl: "../../scripts/system/html/marketplaces.html"
    property int statusBarHeight: 50
    property int statusMargin: 50
    property string standardMessage: "Check out other marketplaces."
    property string claraMessage: "Choose a model and click Download -> Autodesk FBX."
    property string claraError: "High Fidelity only supports Autodesk FBX models."

    onVisibleChanged: {
        keyboardEnabled = HMD.active;
    }

    Controls.BaseWebView {
        id: webview
        url: marketplacesUrl
        anchors.top: marketplace.top
        width: parent.width
        height: parent.height - statusBarHeight - keyboard.height
        focus: true

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
            sourceUrl: resourceDirectoryUrl + "/html/raiseAndLowerKeyboard.js"
            worldId: WebEngineScript.MainWorld
        }

        userScripts: [ createGlobalEventBridge, raiseAndLowerKeyboard ]

        Timer {
            id: alertTimer
            running: false
            repeat: false
            interval: 9000
            property var handler;
            onTriggered: handler();
        }

        function displayErrorStatus() {
            alertTimer.handler = function() {
                statusLabel.text = claraMessage;
                statusBar.color = hifi.colors.blueHighlight;
                statusIcon.text = hifi.glyphs.info;
            }
            alertTimer.start();
        }

        // In library page:
        // - Open each item in "image" view.
        property string updateLibraryPage: 'if ($) {
                                                $(document).ready(function() {
                                                    var elements = $("a.thumbnail");
                                                    for (var i = 0, length = elements.length; i < length; i++) {
                                                        var value = elements[i].getAttribute("href");
                                                        if (value.slice(-6) !== "/image") {
                                                            elements[i].setAttribute("href", value + "/image");
                                                        }
                                                    }
                                                });
                                            }';

        // In item page:
        // - Fix up library link URL.
        // - Reuse FBX download button as HiFi download button.
        // - Remove "Edit Online", "Get Embed Code", and other download buttons.
        property string updateItemPage:    'if ($) {
                                                var element = $("a[href^=\'/library\']")[0];
                                                var parameters = "?gameCheck=true&public=true";
                                                var href = element.getAttribute("href");
                                                if (href.slice(-parameters.length) !== parameters) {
                                                    element.setAttribute("href", href + parameters);
                                                }
                                                var buttons = $("a.embed-button").parent("div");
                                                if (buttons.length > 0) {
                                                    var downloadFBX = buttons.find("a[data-extension=\'fbx\']")[0];
                                                    downloadFBX.addEventListener("click", startAutoDownload);
                                                    var firstButton = buttons.children(":first-child")[0];
                                                    buttons[0].insertBefore(downloadFBX, firstButton);
                                                    downloadFBX.setAttribute("class", "btn btn-primary download");
                                                    downloadFBX.innerHTML = "<i class=\'glyphicon glyphicon-download-alt\'></i> Download to High Fidelity";
                                                    buttons.children(":nth-child(2), .btn-group , .embed-button").each(function () { this.remove(); });
                                                }
                                                var downloadTimer;
                                                function startAutoDownload() {
                                                    if (!downloadTimer) {
                                                        downloadTimer = setInterval(autoDownload, 1000);
                                                    }
                                                }
                                                function autoDownload() {
                                                    if ($("div.download-body").length !== 0) {
                                                        var downloadButton = $("div.download-body a.download-file");
                                                        if (downloadButton.length > 0) {
                                                            clearInterval(downloadTimer);
                                                            downloadTimer = null;
                                                            var href = downloadButton[0].href;
                                                            EventBridge.emitWebEvent("CLARA.IO DOWNLOAD " + href);
                                                            console.log("Clara.io FBX file download initiated");
                                                            $("a.btn.cancel").click();
                                                            setTimeout(function () { window.open(href); }, 500);  // Let cancel click take effect.
                                                        };
                                                    } else {
                                                        clearInterval(downloadTimer);
                                                        downloadTimer = null;
                                                    }
                                                }
                                            }';

        property var notFbxHandler:     'var element = $("a.btn.btn-primary.viewer-button.download-file");
                                         element.click();'

        property var autoCancel: 'var element = $("a.btn.cancel");
                                  element.click();'

        onUrlChanged: {
            var location = url.toString();
            if (location.indexOf("clara.io/library") !== -1) {
                // Catalog page.
                runJavaScript(updateLibraryPage, function() { console.log("Library link JS injection"); });
            }
            if (location.indexOf("clara.io/view/") !== -1) {
                // Item page.
                runJavaScript(updateItemPage, function() { console.log("Item link JS injection"); });
            }
        }

        onLinkHovered: {
            desktop.currentUrl = hoveredUrl;
            if (File.isZipped(desktop.currentUrl)) {
                statusLabel.text = claraError;
                statusBar.color = hifi.colors.redHighlight;
                statusIcon.text = hifi.glyphs.alert;
                runJavaScript(notFbxHandler, displayErrorStatus());
            }
        }

        onLoadingChanged: {
            if (File.isClaraLink(webview.url)) {
                statusLabel.text = claraMessage;
            } else {
                statusLabel.text = standardMessage;
            }
            statusBar.color = hifi.colors.blueHighlight;
            statusIcon.text = hifi.glyphs.info;
        }

        onNewViewRequested: {
            var component = Qt.createComponent("Browser.qml");
            var newWindow = component.createObject(desktop);

            // Hide brief flash of browser window behind marketplace window.
            newWindow.x = x;
            newWindow.y = y;
            newWindow.z = 0;
            newWindow.width = 0;
            newWindow.height = 0;

            request.openIn(newWindow.webView);
            if (File.isZippedFbx(desktop.currentUrl)) {
                newWindow.setAutoAdd(true);
                runJavaScript(autoCancel);
                newWindow.loadingChanged.connect(function(status) {
                    if (status > 0) {
                        newWindow.destroy();  // Download has kicked off so we can destroy the Web window.
                    }
                });
            }
        }

        function onWebEventReceived(event) {
            if (event.slice(0, 17) === "CLARA.IO DOWNLOAD") {
                desktop.currentUrl = event.slice(18);
                ApplicationInterface.addAssetToWorldInitiate();
            }
        }

        Component.onCompleted: {
            eventBridge.webEventReceived.connect(onWebEventReceived);
        }
    }

    Rectangle {
        id: statusBar
        anchors.top: webview.bottom
        anchors.bottom: keyboard.top
        anchors.left: parent.left
        anchors.right: parent.right
        color: hifi.colors.blueHighlight

        Controls.Button {
            id: switchMarketView
            anchors.right: parent.right
            anchors.rightMargin: statusMargin
            anchors.verticalCenter: parent.verticalCenter
            width: 150
            text: "See all markets"
            onClicked: {
                webview.url = marketplacesUrl;
                statusLabel.text = standardMessage;
            }
        }

        Controls.Label {
            id: statusLabel
            anchors.verticalCenter: switchMarketView.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: statusMargin
            color: hifi.colors.white
            text: standardMessage
            size: 18
        }

        HiFiGlyphs {
            id: statusIcon
            anchors.right: statusLabel.left
            anchors.verticalCenter: statusLabel.verticalCenter
            text: hifi.glyphs.info
            color: hifi.colors.white
            size: hifi.fontSizes.tableHeadingIcon
        }
    }

    Controls.Keyboard {
        id: keyboard
        enabled: keyboardEnabled
        raised: keyboardEnabled && keyboardRaised
        numeric: punctuationMode
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }
}
