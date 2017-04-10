//
//  TabletAddressDialog.qml
//
//  Created by Dante Ruiz on 2017/04/24
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import QtGraphicalEffects 1.0
import "../../controls"
import "../../styles"
import "../../windows"
import "../"
import "../toolbars"
import "../../styles-uit" as HifiStyles
import "../../controls-uit" as HifiControlsUit
import "../../controls" as HifiControls


Rectangle {
    id: cardRoot
    HifiStyles.HifiConstants { id: hifi }
    width: parent.width
    height: parent.height
    property string address: ""
    property alias eventBridge: webview.eventBridge
    function setUrl(url) {
        cardRoot.address = url;
        webview.url = url;
    }
    
    HifiControls.TabletWebView {
        id: webview
        parentStackItem: root
        anchors {
            top: parent.top
            right: parent.right
            left: parent.left
            bottom: parent.bottom
        }
    }   
}
