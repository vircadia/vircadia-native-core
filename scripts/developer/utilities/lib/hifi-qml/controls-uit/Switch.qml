//
//  Switch.qml
//
//  Created by Zach Fox on 2017-06-06
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4 as Original
import QtQuick.Controls.Styles 1.4

import "../styles-uit"

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
        activeFocusOnPress: true;
        anchors.top: rootSwitch.top;
        anchors.left: rootSwitch.left;
        anchors.leftMargin: rootSwitch.width/2 - rootSwitch.switchWidth/2;
        onCheckedChanged: rootSwitch.onCheckedChanged();
        onClicked: rootSwitch.clicked();

        style: SwitchStyle {

            padding {
                top: 3;
                left: 3;
                right: 3;
                bottom: 3;
            }

            groove: Rectangle {
                color: "#252525";
                implicitWidth: rootSwitch.switchWidth;
                implicitHeight: rootSwitch.height;
                radius: rootSwitch.switchRadius;
            }

            handle: Rectangle {
                id: switchHandle;
                implicitWidth: rootSwitch.height - padding.top - padding.bottom;
                implicitHeight: implicitWidth;
                radius: implicitWidth/2;
                border.color: hifi.colors.lightGrayText;
                color: hifi.colors.lightGray;
        
                MouseArea {
                    anchors.fill: parent;
                    hoverEnabled: true;
                    onEntered: parent.color = hifi.colors.blueHighlight;
                    onExited: parent.color = hifi.colors.lightGray;
                }
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
