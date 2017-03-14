//
//  TabletEntityStatistics.qml
//
//  Created by Vlad Stelmahovsky on  3/11/17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0

import "../../styles-uit"
import "../../controls-uit" as HifiControls

Column {
    id: root
    property int colorScheme: hifi.colorSchemes.dark

    property alias titleText: titleLabel.text
    property alias text: valueLabel.text
    property alias color: valueLabel.color

    HifiConstants { id: hifi }

    anchors.left: parent.left
    anchors.right: parent.right
    spacing: 10

    HifiControls.Label {
        id: titleLabel
        size: 20
        anchors.left: parent.left
        anchors.right: parent.right
        colorScheme: root.colorScheme
    }

    RalewaySemiBold {
        id: valueLabel
        anchors.left: parent.left
        anchors.right: parent.right
        wrapMode: Text.WordWrap
        size: 16
        color: enabled ? (root.colorScheme == hifi.colorSchemes.light ? hifi.colors.lightGray : hifi.colors.lightGrayText)
                       : (root.colorScheme == hifi.colorSchemes.light ? hifi.colors.lightGrayText : hifi.colors.baseGrayHighlight);
    }
}
