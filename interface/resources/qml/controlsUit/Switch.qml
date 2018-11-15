//
//  Switch.qml
//
//  Created by Zach Fox on 2017-06-06
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 2.2 as Original

import "../stylesUit"

Item {
    id: rootSwitch;

    property int colorScheme: hifi.colorSchemes.light;
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light;
    property int switchWidth: 70;
    readonly property int switchRadius: height/2;
    property string labelTextOff: "";
    property string labelGlyphOffText: "";
    property int labelGlyphOffSize: 32;
    property string labelTextOn: "";
    property string labelGlyphOnText: "";
    property int labelGlyphOnSize: 32;
    property alias checked: originalSwitch.checked;
    signal onCheckedChanged;
    signal clicked;

    Original.Switch {
        id: originalSwitch;
        focusPolicy: Qt.ClickFocus
        anchors.top: rootSwitch.top;
        anchors.left: rootSwitch.left;
        anchors.leftMargin: rootSwitch.width/2 - rootSwitch.switchWidth/2;
        onCheckedChanged: rootSwitch.onCheckedChanged();
        onClicked: rootSwitch.clicked();
        hoverEnabled: true

        topPadding: 3;
        leftPadding: 3;
        rightPadding: 3;
        bottomPadding: 3;

        onHoveredChanged: {
            if (hovered) {
                switchHandle.color = hifi.colors.blueHighlight;
            } else {
                switchHandle.color = hifi.colors.lightGray;
            }
        }

        background: Rectangle {
            color: "#252525";
            implicitWidth: rootSwitch.switchWidth;
            implicitHeight: rootSwitch.height;
            radius: rootSwitch.switchRadius;
        }

        indicator: Rectangle {
            id: switchHandle;
            implicitWidth: rootSwitch.height - originalSwitch.topPadding - originalSwitch.bottomPadding;
            implicitHeight: implicitWidth;
            radius: implicitWidth/2;
            border.color: hifi.colors.lightGrayText;
            color: hifi.colors.lightGray;
            //x: originalSwitch.leftPadding
            x: Math.max(0, Math.min(parent.width - width, originalSwitch.visualPosition * parent.width - (width / 2)))
            y: parent.height / 2 - height / 2
            Behavior on x {
                enabled: !originalSwitch.down
                SmoothedAnimation { velocity: 200 }
            }

        }
    }

    // OFF Label
    Item {
        anchors.right: originalSwitch.left;
        anchors.rightMargin: 10;
        anchors.top: rootSwitch.top;
        height: rootSwitch.height;

        RalewaySemiBold {
            id: labelOff;
            text: labelTextOff;
            size: hifi.fontSizes.inputLabel;
            color: originalSwitch.checked ? hifi.colors.lightGrayText : "#FFFFFF";
            anchors.top: parent.top;
            anchors.right: parent.right;
            width: paintedWidth;
            height: parent.height;
            verticalAlignment: Text.AlignVCenter;
        }

        HiFiGlyphs {
            id: labelGlyphOff;
            text: labelGlyphOffText;
            size: labelGlyphOffSize;
            color: labelOff.color;
            anchors.top: parent.top;
            anchors.topMargin: 2;
            anchors.right: labelOff.left;
            anchors.rightMargin: 4;
        }

        MouseArea {
            anchors.top: parent.top;
            anchors.bottom: parent.bottom;
            anchors.left: labelGlyphOff.left;
            anchors.right: labelOff.right;
            onClicked: {
                originalSwitch.checked = false;
            }
        }
    }

    // ON Label
    Item {
        anchors.left: originalSwitch.right;
        anchors.leftMargin: 10;
        anchors.top: rootSwitch.top;
        height: rootSwitch.height;

        RalewaySemiBold {
            id: labelOn;
            text: labelTextOn;
            size: hifi.fontSizes.inputLabel;
            color: originalSwitch.checked ? "#FFFFFF" : hifi.colors.lightGrayText;
            anchors.top: parent.top;
            anchors.left: parent.left;
            width: paintedWidth;
            height: parent.height;
            verticalAlignment: Text.AlignVCenter;
        }

        HiFiGlyphs {
            id: labelGlyphOn;
            text: labelGlyphOnText;
            size: labelGlyphOnSize;
            color: labelOn.color;
            anchors.top: parent.top;
            anchors.left: labelOn.right;
        }

        MouseArea {
            anchors.top: parent.top;
            anchors.bottom: parent.bottom;
            anchors.left: labelOn.left;
            anchors.right: labelGlyphOn.right;
            onClicked: {
                originalSwitch.checked = true;
            }
        }
    }
}
