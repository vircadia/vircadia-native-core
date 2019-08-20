//
//  HelpSupport.qml
//
//  Created by Zach Fox on 2019-08-20
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

Item {
    id: root
    width: parent.width
    height: parent.height


    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }


    Image {
        id: accent
        source: "../images/accent2.svg"
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
        anchors.left: parent.left
        anchors.leftMargin: 26
        anchors.right: parent.right
        anchors.rightMargin: 26
        anchors.top: parent.top
        spacing: 0

        HifiStylesUit.GraphikSemiBold {
            text: "Support"
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: paintedHeight
            Layout.topMargin: 16
            size: 22
            color: simplifiedUI.colors.text.white
        }

        HifiStylesUit.GraphikRegular {
            text: "You can quickly get the support that you need by clicking the button below."
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: paintedHeight
            Layout.topMargin: 8
            size: 18
            wrapMode: Text.Wrap
            color: simplifiedUI.colors.text.white
        }

        SimplifiedControls.Button {
            Layout.topMargin: 8
            width: 200
            height: 32
            text: "CONTACT SUPPORT"
            temporaryText: "Opening browser..."

            onClicked: {
                Qt.openUrlExternally("https://www.highfidelity.com/knowledge/kb-tickets/new");
            }
        }
    }

    signal sendToScript(var message);
}
