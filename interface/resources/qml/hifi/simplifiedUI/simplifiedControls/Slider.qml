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
        color: simplifiedUI.colors.text.white
        size: simplifiedUI.sizes.controls.slider.labelTextSize
    }

    Slider {
        id: sliderControl
        height: simplifiedUI.sizes.controls.slider.height
        width: root.width * 0.6
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
            width: sliderControl.width
            height: simplifiedUI.sizes.controls.slider.backgroundHeight
            y: sliderControl.height / 2 - height / 2
            radius: height / 2
            color: simplifiedUI.colors.controls.slider.background.empty

            Rectangle {
                width: sliderControl.visualPosition * sliderControl.width
                height: parent.height
                radius: height / 2
                gradient: Gradient {
                    GradientStop { position: 0.0; color: simplifiedUI.colors.controls.slider.background.filled.start }
                    GradientStop { position: 1.0; color: simplifiedUI.colors.controls.slider.background.filled.finish }
                }
            }
        }

        handle: Rectangle {
            width: sliderControl.height
            height: sliderControl.height
            x: sliderControl.visualPosition * (sliderControl.width - width)
            y: sliderControl.height / 2 - height / 2
            radius: height / 2
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: sliderControl.pressed
                        ? (simplifiedUI.colors.controls.slider.handle.pressed.start)
                        : (simplifiedUI.colors.controls.slider.handle.normal.start)
                }
                GradientStop {
                    position: 1.0
                    color: sliderControl.pressed
                        ? (simplifiedUI.colors.controls.slider.handle.pressed.finish)
                        : (simplifiedUI.colors.controls.slider.handle.normal.finish)
                }
            }

            Rectangle {
                height: parent.height - 2
                width: height
                radius: height / 2
                anchors.centerIn: parent
                color: "transparent"
                border.width: 1
                border.color: simplifiedUI.colors.controls.slider.handle.border
            }
        }
    }
}
