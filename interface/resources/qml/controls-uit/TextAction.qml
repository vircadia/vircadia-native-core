//
//  TextField.qml
//
//  Created by David Rowe on 21 Apr 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

import "../styles-uit"
import "../controls-uit" as HifiControls

Item {
    property string icon: ""
    property int iconSize: 30
    property string text: ""

    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light

    signal clicked()

    height: Math.max(glyph.visible ? glyph.height - 4 : 0, string.visible ? string.height : 0)
    width: glyph.width + string.anchors.leftMargin + string.width

    HiFiGlyphs {
        id: glyph
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: -2
        text: parent.icon
        size: parent.iconSize
        color: isLightColorScheme
               ? (mouseArea.containsMouse ? hifi.colors.baseGrayHighlight : hifi.colors.lightGray)
               : (mouseArea.containsMouse ? hifi.colors.faintGray : hifi.colors.lightGrayText)
        visible: text !== ""
        width: visible ? implicitWidth : 0
    }

    RalewaySemiBold {
        id: string
        anchors {
            left: glyph.visible ? glyph.right : parent.left
            leftMargin: visible && glyph.visible ? hifi.dimensions.contentSpacing.x : 0
            verticalCenter: glyph.visible ? glyph.verticalCenter : undefined
        }
        text: parent.text
        size: hifi.fontSizes.inputLabel
        color: isLightColorScheme
               ? (mouseArea.containsMouse ? hifi.colors.baseGrayHighlight : hifi.colors.lightGray)
               : (mouseArea.containsMouse ? hifi.colors.faintGray : hifi.colors.lightGrayText)
        font.underline: true;
        visible: text !== ""
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: parent.clicked()
    }
}
