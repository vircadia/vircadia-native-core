//
//  HelpControls.qml
//
//  Created by Zach Fox on 2019-08-07
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.3
import "../../simplifiedConstants" as SimplifiedConstants
import "../../simplifiedControls" as SimplifiedControls
import stylesUit 1.0 as HifiStylesUit
import QtQuick.Layouts 1.3

Flickable {
    id: root
    contentWidth: parent.width
    contentHeight: controlsColumnLayout.height
    clip: true

    onVisibleChanged: {
        if (visible) {
            root.contentX = 0;
            root.contentY = 0;
        }
    }

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }


    Image {
        id: accent
        source: "../images/accent1.svg"
        anchors.left: parent.left
        anchors.top: parent.top
        width: 83
        height: 156
        transform: Scale {
            xScale: -1
            origin.x: accent.width / 2
            origin.y: accent.height / 2
        }
    }


    ColumnLayout {
        id: controlsColumnLayout
        anchors.left: parent.left
        anchors.leftMargin: 26
        anchors.right: parent.right
        anchors.rightMargin: 26
        anchors.top: parent.top
        spacing: 0

        HifiStylesUit.GraphikSemiBold {
            text: "HQ Controls"
            Layout.preferredWidth: parent.width
            Layout.topMargin: 16
            height: paintedHeight
            size: 22
            color: simplifiedUI.colors.text.white
        }

        HifiStylesUit.GraphikRegular {
            text: "You can use the following controls to move your avatar around your HQ:"
            Layout.preferredWidth: parent.width
            wrapMode: Text.Wrap
            height: paintedHeight
            size: 18
            color: simplifiedUI.colors.text.white
        }

        ControlsTable {
            Layout.topMargin: 8
            Layout.preferredWidth: parent.width
        }

        SimplifiedControls.Button {
            Layout.topMargin: 14
            Layout.preferredWidth: 200
            height: 32
            text: "VIEW ALL CONTROLS"
            temporaryText: "Viewing!"

            onClicked: {
                Qt.openUrlExternally("https://www.highfidelity.com/knowledge/get-around");
            }
        }
    }
}
