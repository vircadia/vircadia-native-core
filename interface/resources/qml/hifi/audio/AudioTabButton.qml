//
//  AudioTabButton.qml
//  qml/hifi/audio
//
//  Created by Vlad Stelmahovsky on 8/16/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 2.2
import controlsUit 1.0 as HifiControls
import stylesUit 1.0

TabButton {
    id: control
    font.pixelSize: height / 2

    HifiConstants { id: hifi; }

    contentItem: RalewaySemiBold {
        text: control.text
        font: control.font
        color: "white"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        color: control.checked ? hifi.colors.baseGray : "black"
    }
}
