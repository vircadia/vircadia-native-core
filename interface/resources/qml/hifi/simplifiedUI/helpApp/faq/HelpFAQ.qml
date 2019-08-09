//
//  HelpFAQ.qml
//
//  Created by Zach Fox on 2019-08-08
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
import controlsUit 1.0 as HifiControlsUit
import QtQuick.Layouts 1.3
import PerformanceEnums 1.0

Item {
    id: root
    width: parent.width
    height: parent.height
    

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }


    Image {
        id: accent
        source: "../images/accent3.svg"
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
        id: faqColumnLayout
        anchors.left: parent.left
        anchors.leftMargin: 26
        anchors.right: parent.right
        anchors.rightMargin: 26
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 16
        spacing: 0

        HifiStylesUit.GraphikSemiBold {
            text: "FAQ"
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: paintedHeight
            Layout.topMargin: 16
            size: 22
            color: simplifiedUI.colors.text.white
        }

        HifiControlsUit.WebView {
            url: "https://www.highfidelity.com/knowledge"
            Layout.preferredWidth: parent.width
            Layout.fillHeight: true
            Layout.topMargin: 16
        }
    }
}
