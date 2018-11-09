//
//  ScrollBar.qml
//
//  Created by Vlad Stelmahovsky on 27 Nov 2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 2.2

import "../stylesUit"

ScrollBar {
    visible: size < 1.0

    HifiConstants { id: hifi }
    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light

    background: Item {
        implicitWidth: hifi.dimensions.scrollbarBackgroundWidth
        Rectangle {
            anchors { fill: parent; topMargin: 3; bottomMargin: 3 }
            radius: hifi.dimensions.scrollbarHandleWidth/2
            color: isLightColorScheme ? hifi.colors.tableScrollBackgroundLight
                                      : hifi.colors.tableScrollBackgroundDark
        }
    }

    contentItem: Item {
        implicitWidth: hifi.dimensions.scrollbarHandleWidth
        Rectangle {
            anchors { fill: parent; topMargin: 1; bottomMargin: 1 }
            radius: hifi.dimensions.scrollbarHandleWidth/2
            color: isLightColorScheme ? hifi.colors.tableScrollHandleLight : hifi.colors.tableScrollHandleDark
        }
    }
}
