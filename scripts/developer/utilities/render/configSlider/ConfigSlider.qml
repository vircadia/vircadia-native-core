//
//  ConfigSlider.qml
//  examples/utilities/tools/render
//
//  Created by Zach Pomerantz on 2/8/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4

Item {
    id: root
    width: 400
    height: 24
    property bool integral: false
    property var config
    property string property
    property alias label: labelControl.text
    property alias min: sliderControl.minimumValue
    property alias max: sliderControl.maximumValue

    Component.onCompleted: {
        // Binding favors qml value, so set it first
        sliderControl.value = root.config[root.property];
        bindingControl.when = true;
    }

    Label {
        id: labelControl
        text: root.label
        anchors.left: root.left
        anchors.leftMargin: 8
        anchors.top: root.top
        anchors.topMargin: 7
    }

    Label {
        text: sliderControl.value.toFixed(root.integral ? 0 : 2)
        anchors.left: root.left
        anchors.leftMargin: 200
        anchors.top: root.top
        anchors.topMargin: 7
    }

    Binding {
        id: bindingControl
        target: root.config
        property: root.property
        value: sliderControl.value
        when: false
    }

    Slider {
        id: sliderControl
        stepSize: root.integral ? 1.0 : 0.0
        width: 150
        height: 20
        anchors.right: root.right
        anchors.rightMargin: 8
        anchors.top: root.top
        anchors.topMargin: 3
    }
}
