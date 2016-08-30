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
    property var currentUrl: "https://metaverse.highfidelity.com/marketplace"
    property int statusBarHeight: 50
    property int statusMargin: 50

    Controls.WebView {
        id: webview
        url: currentUrl
        anchors.top: marketplace.top
        width: parent.width
        height: parent.height - statusBarHeight
        focus: true
        newWindowHook: function (component, newWindow) {
            if (File.isZippedFbx(desktop.currentUrl)) {
                runJavaScript(autoCancel);                
                zipTimer.handler = function() {
                    console.log("timer started", newWindow)
                    newWindow.destroy();
                }
                zipTimer.start();
            }
        }

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

    Rectangle {
        id: statusBar
        anchors.top: webview.bottom
        anchors.bottom: parent.bottom
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
            onClicked: webview.url = "file:///E:/GitHub/hifi/scripts/system/html/marketplaces.html"
        }

        Controls.Label {
            id: statusLabel
            anchors.verticalCenter: switchMarketView.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: statusMargin
            color: hifi.colors.white
            text: "Check out other marketplaces"
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

}