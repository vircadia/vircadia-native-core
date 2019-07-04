//
//  EditTabButton.qml
//  qml/hifi/tablet
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
    property alias title: control.text
    property alias active: control.checkable
    height: 40
    padding: 0
    spacing: 0
    HifiConstants { id: hifi; }

    contentItem: Text {
        id: text
        text: control.text
        font.pixelSize: 14
        font.bold: true
        color: "white"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        fontSizeMode: Text.HorizontalFit
        property string glyphtext: ""
        HiFiGlyphs {
            anchors.centerIn: parent
            size: 30
            color: "#ffffff"
            text: text.glyphtext
        }
        Component.onCompleted: {
            if (control.text === "P") {
                text.text = "   ";
                text.glyphtext = "\ue004";
            }
        }
    }

    background: Rectangle {
        color: control.checked ? "#404040" :"black"
        implicitWidth: control.contentItem.width + 42 > text.paintedWidth ? control.contentItem.width + 42 :
                                                        text.paintedWidth + 10
        implicitHeight: 40
    }
}
