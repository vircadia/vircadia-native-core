//
//  Slider.qml
//
//  Created by Zach Fox on 2019-05-06
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.3
import stylesUit 1.0 as HifiStylesUit
import TabletScriptingInterface 1.0
import "../simplifiedConstants" as SimplifiedConstants

Item {
    id: root

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    property alias labelText: sliderText.text
    property alias labelTextSize: sliderText.size
    property alias labelTextColor: sliderText.color
    property alias value: sliderControl.value
    property alias from: sliderControl.from
    property alias to: sliderControl.to
    property alias live: sliderControl.live
    property alias stepSize: sliderControl.stepSize
    property alias snapMode: sliderControl.snapMode
    property alias pressed: sliderControl.pressed
    property real defaultValue: 0.0

    HifiStylesUit.GraphikRegular {
        id: sliderText
        text: ""
        anchors.right: sliderControl.left
        anchors.rightMargin: 8
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: -4 // Necessary for vertical alignment
        anchors.bottom: parent.bottom
        horizontalAlignment: Text.AlignRight
        visible: sliderText.text != ""
        color: root.enabled ? simplifiedUI.colors.controls.slider.text.enabled : simplifiedUI.colors.controls.slider.text.disabled
        size: simplifiedUI.sizes.controls.slider.labelTextSize
    }

    Slider {
        id: sliderControl
        height: simplifiedUI.sizes.controls.slider.height
        width: root.width * 0.6
        enabled: root.enabled
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        onPressedChanged: {
            if (pressed) {
                Tablet.playSound(TabletEnums.ButtonClick);
            }
        }

        onHoveredChanged: {
            if (hovered) {
                Tablet.playSound(TabletEnums.ButtonHover);
            }
        }

        background: Rectangle {
            id: sliderBackground
            width: sliderControl.width - sliderHandle.width
            height: simplifiedUI.sizes.controls.slider.backgroundHeight
            x: sliderHandle.width / 2
            y: sliderControl.height / 2 - height / 2
            radius: height / 2
            color: simplifiedUI.colors.controls.slider.background.empty

            Rectangle {
                width: sliderControl.visualPosition * sliderBackground.width
                height: parent.height
                radius: height / 2
                gradient: Gradient {
                    GradientStop { position: 0.0; color: simplifiedUI.colors.controls.slider.background.filled.start }
                    GradientStop { position: 1.0; color: simplifiedUI.colors.controls.slider.background.filled.finish }
                }
            }
        }

        handle: Rectangle {
            id: sliderHandle
            width: sliderControl.height
            height: width
            x: sliderControl.visualPosition * sliderBackground.width
            y: sliderControl.height / 2 - height / 2
            radius: height / 2
            color: "#000000"
            border.width: 1
            border.color: sliderControl.hovered || sliderControl.pressed ? simplifiedUI.colors.controls.slider.handle.enabledBorder : simplifiedUI.colors.controls.slider.handle.disabledBorder

            Rectangle {
                visible: root.enabled
                height: sliderControl.pressed ? parent.height : parent.height - 4
                width: height
                radius: height / 2
                anchors.centerIn: parent
                gradient: Gradient {
                    GradientStop {
                        position: 0.2
                        color: sliderControl.enabled ? (sliderControl.hovered ? simplifiedUI.colors.controls.slider.handle.hover.start :
                            (sliderControl.pressed
                            ? (simplifiedUI.colors.controls.slider.handle.pressed.start)
                            : (simplifiedUI.colors.controls.slider.handle.normal.start))) : simplifiedUI.colors.controls.slider.handle.disabled.start
                    }
                    GradientStop {
                        position: 1.0
                        color: sliderControl.enabled ? (sliderControl.hovered ? simplifiedUI.colors.controls.slider.handle.hover.finish :
                            (sliderControl.pressed
                            ? (simplifiedUI.colors.controls.slider.handle.pressed.finish)
                            : (simplifiedUI.colors.controls.slider.handle.normal.finish))) : simplifiedUI.colors.controls.slider.handle.disabled.finish
                    }
                }
            }
        }
    }
}
