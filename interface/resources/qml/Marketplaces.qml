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
import QtWebEngine 1.1
import QtWebSockets 1.0
import "qrc:///qtwebchannel/qwebchannel.js" as WebChannel

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

    property bool keyboardEnabled: true
    property bool keyboardRaised: true
    property bool punctuationMode: false

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

        Timer {
            id: alertTimer
            running: false
            repeat: false
            interval: 9000
            property var handler;
            onTriggered: handler();
        }

        property var autoCancel: 'var element = $("a.btn.cancel");
                                  element.click();'

        property var simpleDownload: 'var element = $("a.download-file");
                                      element.removeClass("download-file");
                                      element.removeAttr("download");'

        function displayErrorStatus() {
            alertTimer.handler = function() {
                statusLabel.text = claraMessage;
                statusBar.color = hifi.colors.blueHighlight;
                statusIcon.text = hifi.glyphs.info;
            }
            alertTimer.start();
        }

        property var notFbxHandler:     'var element = $("a.btn.btn-primary.viewer-button.download-file")
                                         element.click();'

        // Replace Clara FBX download link action with Cara API action.
        property string replaceFBXDownload:    'var element = $("a[data-extension=\'fbx\']:first");
                                                element.unbind("click");
                                                element.bind("click", function(event) {
                                                    console.log("Initiate Clara.io FBX file download");
                                                    window.open("https://clara.io/api/scenes/{uuid}/export/fbx?fbxVersion=7.4&fbxEmbedTextures=true&centerScene=true&alignSceneGound=true");
                                                    return false;
                                                });'

        onLinkHovered: {
            desktop.currentUrl = hoveredUrl;

            if (desktop.isClaraFBXZipDownload(desktop.currentUrl)) {
                var doReplaceFBXDownload = replaceFBXDownload.replace("{uuid}", desktop.currentUrl.slice(desktop.currentUrl.lastIndexOf("/") + 1, -1));
                runJavaScript(doReplaceFBXDownload);
            }

            if (File.isZippedFbx(desktop.currentUrl)) {
                runJavaScript(simpleDownload, function(){console.log("Download JS injection");});
                return;
            }

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
            if (desktop.isClaraFBXZipDownload(desktop.currentUrl)) {
                newWindow.setAutoAdd(true);
                runJavaScript(autoCancel);
                newWindow.loadingChanged.connect(function(status) {
                    if (status > 0) {
                        newWindow.destroy();  // Download has kicked off so we can destroy the Web window.
                    }
                });
            }
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
