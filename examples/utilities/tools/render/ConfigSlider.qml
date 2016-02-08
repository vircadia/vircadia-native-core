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
    width: 400
    height: 24
    property string label
    property QtObject config
    property string prop
    property real min: 0.0
    property real max: 1.0

    function init() {
        stat.text = config[prop].toFixed(2);
        slider.value = (config[prop] - min) / (max - min);
    }
    Component.onCompleted: init()

    function update() {
        var val = min + (max - min) * slider.value;
        stat.text = val.toFixed(2);
        config[prop] = val;
    }

    Label {
        text: parent.label
        y: 7
        anchors.left: parent.left
        anchors.leftMargin: 8
    }

    Label {
        id: stat
        y: 7
        anchors.left: parent.left
        anchors.leftMargin: 140
    }

    Slider {
        id: slider
        y: 3
        width: 192
        height: 20
        onValueChanged: parent.update()
        anchors.right: parent.right
        anchors.rightMargin: 8
    }
}
