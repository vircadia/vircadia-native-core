//
//  Slider.qml
//
//  Created by David Rowe on 27 Feb 2016
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

Slider {
    id: slider

    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light
    property string label: ""
    property real controlHeight: height + (sliderLabel.visible ? sliderLabel.height + sliderLabel.anchors.bottomMargin : 0)

    height: hifi.fontSizes.textFieldInput + 14  // Match height of TextField control.
    y: sliderLabel.visible ? sliderLabel.height + sliderLabel.anchors.bottomMargin : 0

    style: SliderStyle {

        groove: Rectangle {
            implicitWidth: 50
            implicitHeight: hifi.dimensions.sliderGrooveHeight
            radius: height / 2
            color: isLightColorScheme ? hifi.colors.sliderGutterLight : hifi.colors.sliderGutterDark

            Rectangle {
                width: parent.height - 2
                height: slider.width * (slider.value - slider.minimumValue) / (slider.maximumValue - slider.minimumValue) - 1
                radius: height / 2
                anchors {
                    top: parent.top
                    topMargin: width + 1
                    left: parent.left
                    leftMargin: 1
                }
                transformOrigin: Item.TopLeft
                rotation: -90
                gradient: Gradient {
                    GradientStop { position: 0.0; color: hifi.colors.blueAccent }
                    GradientStop { position: 1.0; color: hifi.colors.primaryHighlight }
                }
            }
        }

        handle: Rectangle {
            implicitWidth: hifi.dimensions.sliderHandleSize
            implicitHeight: hifi.dimensions.sliderHandleSize
            radius: height / 2
            border.width: 1
            border.color: isLightColorScheme ? hifi.colors.sliderBorderLight : hifi.colors.sliderBorderDark
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: pressed || hovered
                           ? (isLightColorScheme ? hifi.colors.sliderDarkStart : hifi.colors.sliderLightStart )
                           : (isLightColorScheme ? hifi.colors.sliderLightStart : hifi.colors.sliderDarkStart )
                }
                GradientStop {
                    position: 1.0
                    color: pressed || hovered
                           ? (isLightColorScheme ? hifi.colors.sliderDarkFinish : hifi.colors.sliderLightFinish )
                           : (isLightColorScheme ? hifi.colors.sliderLightFinish : hifi.colors.sliderDarkFinish )
                }
            }

            Rectangle {
                height: parent.height - 2
                width: height
                radius: height / 2
                anchors.centerIn: parent
                color: hifi.colors.transparent
                border.width: 1
                border.color: hifi.colors.black
            }
        }
    }

    HifiControls.Label {
        id: sliderLabel
        text: slider.label
        colorScheme: slider.colorScheme
        anchors.left: parent.left
        anchors.bottom: parent.top
        anchors.bottomMargin: 2
        visible: label != ""
    }
}
