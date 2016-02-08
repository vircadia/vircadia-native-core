//
//  ItemsSlider.qml
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

    function update() {
        var val = slider.value;
        var max = config.numDrawn;
        var drawn = Math.round(val * max);
        stat.text = drawn + " / " + max;
        config.maxDrawn = (val == 1.0 ? -1 : drawn);
    }

    Timer {
        interval: 500
        running: true
        repeat: true
        onTriggered: parent.update()
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
        anchors.leftMargin: 108
    }

    Slider {
        id: slider
        y: 3
        width: 192
        height: 20
        value: 1.0
        onValueChanged: update()
        anchors.right: parent.right
        anchors.rightMargin: 8
    }
}
