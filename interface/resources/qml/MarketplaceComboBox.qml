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

Item {
    id: marketplaceComboBox
    anchors.fill: parent
    property var currentUrl: "https://metaverse.highfidelity.com/marketplace"
    Controls.WebView {
        id: webview
        url: currentUrl
        anchors.top: parent.top
        width: parent.width
        height: parent.height - 50
        //anchors.bottom: switchMarketView.top
        focus: true
    }

    Controls.ComboBox {
        id: switchMarketView
        anchors.bottom: parent.bottom
        width: parent.width
        height: 50
        visible: true
        label: "Market View: "
        model: ["Marketplace", "Clara.io"]
        onCurrentIndexChanged: {
            if (currentIndex === 0) { webview.url = "https://metaverse.highfidelity.com/marketplace"; }
            if (currentIndex === 1) { webview.url = "https://clara.io/library"; }
        }
        
    }

}