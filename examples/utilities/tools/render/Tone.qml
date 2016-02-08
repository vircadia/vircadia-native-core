//
//  Tone.qml
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
    property string label: qsTr("Tone Mapping Exposure")

    function update() {
        var val = (slider.value - 0.5) * 20;
        stat.text = val.toFixed(2);
        Render.getConfig("ToneMapping").exposure = val;
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
        anchors.leftMargin: 150
    }

    Slider {
        id: slider
        y: 3
        width: 192
        height: 20
        value: Render.getConfig("ToneMapping").exposure
        onValueChanged: parent.update()
        anchors.right: parent.right
        anchors.rightMargin: 8
    }
}
