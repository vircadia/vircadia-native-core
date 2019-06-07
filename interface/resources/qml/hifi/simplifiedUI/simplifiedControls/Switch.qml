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

    onClicked: {
        Tablet.playSound(TabletEnums.ButtonClick);
    }

    Original.Switch {
        id: originalSwitch
        enabled: root.enabled
        focusPolicy: Qt.ClickFocus
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: labelOff.text === "" ? undefined : parent.horizontalCenter
        anchors.left: labelOff.text === "" ? parent.left : undefined
        anchors.leftMargin: (outerSwitchHandle.width - innerSwitchHandle.width) / 2
        width: simplifiedUI.sizes.controls.simplifiedSwitch.switchBackgroundWidth
        hoverEnabled: true

        function changeColor() {
            if (!originalSwitch.enabled) {
                innerSwitchHandle.color = simplifiedUI.colors.controls.simplifiedSwitch.handle.disabled;
                return;
            }
            if (originalSwitch.checked) {
                if (originalSwitch.hovered) {
                    innerSwitchHandle.color = simplifiedUI.colors.controls.simplifiedSwitch.handle.hover;
                } else {
                    innerSwitchHandle.color = simplifiedUI.colors.controls.simplifiedSwitch.handle.on;
                }
            } else {
                if (originalSwitch.hovered) {
                    innerSwitchHandle.color = simplifiedUI.colors.controls.simplifiedSwitch.handle.hover;
                } else {
                    innerSwitchHandle.color = simplifiedUI.colors.controls.simplifiedSwitch.handle.off;
                }
            }
        }

        onCheckedChanged: {
            originalSwitch.changeColor();
        }

        onClicked: {
            root.clicked();
            originalSwitch.changeColor();
        }

        onHoveredChanged: {
            if (hovered) {
                Tablet.playSound(TabletEnums.ButtonHover);
            }
            originalSwitch.changeColor();
        }

        background: Rectangle {
            id: switchBackground
            anchors.verticalCenter: parent.verticalCenter
            color: originalSwitch.checked ? simplifiedUI.colors.controls.simplifiedSwitch.background.on : simplifiedUI.colors.controls.simplifiedSwitch.background.off
            width: originalSwitch.width
            height: simplifiedUI.sizes.controls.simplifiedSwitch.switchBackgroundHeight
            radius: height/2
        }

        indicator: Item {
            anchors.verticalCenter: parent.verticalCenter
            width: simplifiedUI.sizes.controls.simplifiedSwitch.switchHandleOuterWidth
            height: width
            x: originalSwitch.visualPosition * switchBackground.width - (innerSwitchHandle.width * (originalSwitch.checked ? 1 : 0)) - ((outerSwitchHandle.width - innerSwitchHandle.width) / 2)

            Behavior on x {
                enabled: !originalSwitch.down
                SmoothedAnimation { velocity: 200 }
            }

            Rectangle {
                id: outerSwitchHandle
                visible: originalSwitch.hovered
                anchors.centerIn: parent
                width: simplifiedUI.sizes.controls.simplifiedSwitch.switchHandleOuterWidth
                height: width
                radius: width/2
                color: "transparent"
                border.width: simplifiedUI.sizes.controls.simplifiedSwitch.switchHandleBorderSize
                border.color: simplifiedUI.colors.controls.simplifiedSwitch.handle.activeBorder
            }

            Rectangle {
                id: innerSwitchHandle
                anchors.centerIn: parent
                width: simplifiedUI.sizes.controls.simplifiedSwitch.switchHandleInnerWidth
                height: width
                radius: width/2
                color: simplifiedUI.colors.controls.simplifiedSwitch.handle.off
                border.width: originalSwitch.pressed || originalSwitch.checked ? simplifiedUI.sizes.controls.simplifiedSwitch.switchHandleBorderSize : 0
                border.color: originalSwitch.pressed ? simplifiedUI.colors.controls.simplifiedSwitch.handle.activeBorder : simplifiedUI.colors.controls.simplifiedSwitch.handle.checkedBorder

                Component.onCompleted: {
                    originalSwitch.changeColor();
                }
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
