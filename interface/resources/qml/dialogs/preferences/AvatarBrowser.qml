//
//  AvatarBrowser.qml
//
//  Created by Bradley Austin Davis on 30 Aug 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtWebEngine 1.1

import "../../windows" as Windows
import "../../controls-uit" as Controls
import "../../styles-uit"

Windows.Window {
    id: root
    HifiConstants { id: hifi }
    width: 900; height: 700
    resizable: true
    modality: Qt.ApplicationModal

    Controls.WebView {
        id: webview
        anchors.fill: parent
        url: "https://metaverse.highfidelity.com/marketplace?category=avatars"
        focus: true
    }
}
