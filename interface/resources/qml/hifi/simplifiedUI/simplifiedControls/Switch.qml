//
//  Switch.qml
//
//  Created by Zach Fox on 2019-05-08
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.3 as Original
import stylesUit 1.0 as HifiStylesUit
import TabletScriptingInterface 1.0
import "../simplifiedConstants" as SimplifiedConstants

Item {
    id: root

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    property alias switchWidth: switchBackground.width
    property alias labelTextOff: labelOff.text
    property alias labelTextOffSize: labelOff.size
    property alias labelGlyphOffText: labelGlyphOff.text
    property alias labelGlyphOffSize: labelGlyphOff.size
    property alias labelTextOn: labelOn.text
    property alias labelTextOnSize: labelOn.size
    property alias labelGlyphOnText: labelGlyphOn.text
    property alias labelGlyphOnSize: labelGlyphOn.size
    property alias checked: originalSwitch.checked
    property string backgroundOnColor: "#252525"
    signal clicked

    function changeColor() {
        if (originalSwitch.checked) {
            if (originalSwitch.hovered) {
                switchHandle.color = simplifiedUI.colors.controls.simplifiedSwitch.handle.hover;
            } else {
                switchHandle.color = simplifiedUI.colors.controls.simplifiedSwitch.handle.on;
            }
        } else {
            if (originalSwitch.hovered) {
                switchHandle.color = simplifiedUI.colors.controls.simplifiedSwitch.handle.hover;
            } else {
                switchHandle.color = simplifiedUI.colors.controls.simplifiedSwitch.handle.off;
            }
        }
    }

    onClicked: {
        Tablet.playSound(TabletEnums.ButtonClick);
    }

    Original.Switch {
        id: originalSwitch
        focusPolicy: Qt.ClickFocus
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: labelOff.text === "" ? undefined : parent.horizontalCenter
        anchors.left: labelOff.text === "" ? parent.left : undefined
        width: simplifiedUI.sizes.controls.simplifiedSwitch.switchBackgroundWidth
        hoverEnabled: true

        onCheckedChanged: {
            root.checkedChanged();
            Tablet.playSound(TabletEnums.ButtonClick);
            root.changeColor();
        }

        onClicked: {
            root.clicked();
            root.changeColor();
        }

        onHoveredChanged: {
            if (hovered) {
                Tablet.playSound(TabletEnums.ButtonHover);
            }
            root.changeColor();
        }

        background: Rectangle {
            id: switchBackground
            anchors.verticalCenter: parent.verticalCenter
            color: originalSwitch.checked ? simplifiedUI.colors.controls.simplifiedSwitch.background.on : simplifiedUI.colors.controls.simplifiedSwitch.background.off
            width: originalSwitch.width
            height: root.height
            radius: height/2
        }

        indicator: Rectangle {
            id: switchHandle
            anchors.verticalCenter: parent.verticalCenter
            width: switchBackground.height - (2 * simplifiedUI.margins.controls.simplifiedSwitch.handleMargins)
            height: switchHandle.width
            radius: switchHandle.width/2
            color: originalSwitch.checked ? simplifiedUI.colors.controls.simplifiedSwitch.handle.on : simplifiedUI.colors.controls.simplifiedSwitch.handle.off
            x: originalSwitch.visualPosition * switchBackground.width - (switchHandle.width / 2) + (originalSwitch.checked ? -5 * simplifiedUI.margins.controls.simplifiedSwitch.handleMargins : 5 * simplifiedUI.margins.controls.simplifiedSwitch.handleMargins)
            y: simplifiedUI.margins.controls.simplifiedSwitch.handleMargins
            Behavior on x {
                enabled: !originalSwitch.down
                SmoothedAnimation { velocity: 200 }
            }
        }
    }

    // OFF Label
    Item {
        anchors.right: originalSwitch.left
        anchors.rightMargin: 10
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        HifiStylesUit.GraphikRegular {
            id: labelOff
            text: ""
            size: simplifiedUI.sizes.controls.simplifiedSwitch.labelTextSize
            color: originalSwitch.checked ? simplifiedUI.colors.controls.simplifiedSwitch.text.off : simplifiedUI.colors.controls.simplifiedSwitch.text.on
            anchors.top: parent.top
            anchors.topMargin: -2 // Necessary for text alignment
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: paintedWidth
            verticalAlignment: Text.AlignVCenter
        }

        HifiStylesUit.HiFiGlyphs {
            id: labelGlyphOff
            text: ""
            size: simplifiedUI.sizes.controls.simplifiedSwitch.labelGlyphSize
            color: labelOff.color
            anchors.top: parent.top
            anchors.topMargin: 2
            anchors.right: labelOff.left
            anchors.rightMargin: 4
        }

        MouseArea {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: labelGlyphOff.left
            anchors.right: labelOff.right
            onClicked: {
                if (labelOn.text === "" && labelGlyphOn.text === "") {
                    originalSwitch.checked = !originalSwitch.checked;
                } else {
                    originalSwitch.checked = false;
                }

                root.clicked();
            }
        }
    }

    // ON Label
    Item {
        anchors.left: originalSwitch.right
        anchors.leftMargin: 10
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        HifiStylesUit.GraphikRegular {
            id: labelOn
            text: ""
            size: simplifiedUI.sizes.controls.simplifiedSwitch.labelTextSize
            color: originalSwitch.checked ? simplifiedUI.colors.controls.simplifiedSwitch.text.on : simplifiedUI.colors.controls.simplifiedSwitch.text.off
            anchors.top: parent.top
            anchors.topMargin: -2 // Necessary for text alignment
            anchors.left: parent.left
            width: paintedWidth
            height: parent.height
            verticalAlignment: Text.AlignVCenter
        }

        HifiStylesUit.HiFiGlyphs {
            id: labelGlyphOn
            text: ""
            size: simplifiedUI.sizes.controls.simplifiedSwitch.labelGlyphSize
            color: labelOn.color
            anchors.top: parent.top
            anchors.topMargin: 2
            anchors.left: labelOn.right
            anchors.leftMargin: 4
        }

        MouseArea {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: labelOn.left
            anchors.right: labelGlyphOn.right
            onClicked: {
                if (labelOff.text === "" && labelGlyphOff.text === "") {
                    originalSwitch.checked = !originalSwitch.checked;
                } else {
                    originalSwitch.checked = true;
                }
                
                root.clicked();
            }
        }
    }
}
