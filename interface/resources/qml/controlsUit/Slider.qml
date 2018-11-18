//
//  Slider.qml
//
//  Created by David Rowe on 27 Feb 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 2.2

import "../stylesUit"
import "." as HifiControls

Slider {
    id: slider

    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light
    property string label: ""
    property real controlHeight: height + (sliderLabel.visible ? sliderLabel.height + sliderLabel.anchors.bottomMargin : 0)

    property alias minimumValue: slider.from
    property alias maximumValue: slider.to
    property alias step: slider.stepSize
    property bool tickmarksEnabled: false

    height: hifi.fontSizes.textFieldInput + 14  // Match height of TextField control.
    y: sliderLabel.visible ? sliderLabel.height + sliderLabel.anchors.bottomMargin : 0

    background: Rectangle {
        x: slider.leftPadding
        y: slider.topPadding + slider.availableHeight / 2 - height / 2

        implicitWidth: 50
        implicitHeight: hifi.dimensions.sliderGrooveHeight
        width: slider.availableWidth
        height: implicitHeight
        radius: height / 2
        color: isLightColorScheme ? hifi.colors.sliderGutterLight : hifi.colors.sliderGutterDark

        Rectangle {
            width: slider.visualPosition * parent.width
            height: parent.height
            radius: height / 2
            gradient: Gradient {
                GradientStop { position: 0.0; color: hifi.colors.blueAccent }
                GradientStop { position: 1.0; color: hifi.colors.primaryHighlight }
            }
        }
    }

    handle: Rectangle {
        x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
        y: slider.topPadding + slider.availableHeight / 2 - height / 2
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
