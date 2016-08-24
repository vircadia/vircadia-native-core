//
//  MarketplaceComboBox.qml
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
    id: marketplaceComboBox
    anchors.fill: parent
    color: hifi.colors.baseGrayShadow
    property var currentUrl: "https://metaverse.highfidelity.com/marketplace"

    Controls.WebView {
        id: webview
        url: currentUrl
        anchors.top: switchMarketView.bottom
        width: parent.width
        height: parent.height - 40
        focus: true

        Timer {
            id: zipTimer
            running: false
            repeat: false
            interval: 1500
            property var handler;
            onTriggered: handler();
        }

        property var autoCancel: 'var element = $("a.btn.cancel");
                                  element.click();'

        onNewViewRequested: {
            var component = Qt.createComponent("Browser.qml");
            var newWindow = component.createObject(desktop);
            request.openIn(newWindow.webView);
            if (File.isZippedFbx(desktop.currentUrl)) {
                zipTimer.handler = function() {
                    newWindow.destroy();
                    runJavaScript(autoCancel);
                }
                zipTimer.start();
            }
        }

        property var simpleDownload: 'var element = $("a.download-file");
                                      element.removeClass("download-file");
                                      element.removeAttr("download");'

        onLinkHovered: {
            desktop.currentUrl = hoveredUrl;
            // add an error message for non-fbx files
            if (File.isZippedFbx(desktop.currentUrl)) {
                runJavaScript(simpleDownload, function(){console.log("ran the JS");});
            }

        }

    }

    Controls.ComboBox {
        id: switchMarketView
        anchors.top: parent.top
        anchors.right: parent.right
        colorScheme: hifi.colorSchemes.dark
        width: 200
        height: 40
        visible: true
        model: ["Marketplace", "Clara.io"]
        onCurrentIndexChanged: {
            if (currentIndex === 0) { webview.url = "https://metaverse.highfidelity.com/marketplace"; }
            if (currentIndex === 1) { webview.url = "https://clara.io/library"; }
        }
        
    }

    Controls.Label {
        id: switchMarketLabel
        anchors.verticalCenter: switchMarketView.verticalCenter
        anchors.right: switchMarketView.left
        color: hifi.colors.white
        text: "Explore interesting content from: "
    }

}