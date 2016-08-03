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

// just temporary
        Controls.WebView {
            id: webview
            anchors.fill: parent
            url: "about:blank"
            focus: true
        }

        Controls.ComboBox {
            id: switchMarketView
            //anchors.top: parent.bottom
            //anchors.bottomMargin: 4
            anchors.fill: parent
            visible: true
            model: ListModel {
                id: cbItems
                ListElement { text: "Banana" }
                ListElement { text: "Apple" }
                ListElement { text: "Coconut" }
            }
        }
