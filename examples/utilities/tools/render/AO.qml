//
//  AO.qml
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
    width: 200
    height: 200
    property QtObject config: Render.getConfig("AmbientOcclusion")

    Timer {
        interval: 500
        running: true
        repeat: true
        onTriggered: { parent.timer.text = config.gpuTime.toFixed(2) }
    }

    Label { text: qsTr("Ambient Occlusion") }
    Label { id: timer; x: 140 }

    CheckBox {
        y: 1 * 25
        text: qsTr("Dithering")
        partiallyCheckedEnabled: false
        checked: parent.config.ditheringEnabled
        onCheckedChanged: { parent.config.ditheringEnabled = checked }
    }

    ConfigSlider {
        y: 2 * 25
        config: parent.config
        prop: "resolutionLevel"
        label: qsTr("Resolution Level")
        min: 0; max: 4
    }
    ConfigSlider {
        y: 3 * 25
        config: parent.config
        prop: "obscuranceLevel"
        label: qsTr("Obscurance Level")
        min: 0; max: 1
    }
    ConfigSlider {
        y: 4 * 25
        config: parent.config
        prop: "radius"
        label: qsTr("Radius")
        min: 0; max: 2
    }
    ConfigSlider {
        y: 5 * 25
        config: parent.config
        prop: "numSamples"
        label: qsTr("Samples")
        min: 0; max: 32
    }
    ConfigSlider {
        y: 6 * 25
        config: parent.config
        prop: "numSpiralTurns"
        label: qsTr("Spiral Turns")
        min: 0; max: 30
    }
    ConfigSlider {
        y: 7 * 25
        config: parent.config
        prop: "falloffBias"
        label: qsTr("Falloff Bias")
        min: 0; max: 0.2
    }
    ConfigSlider {
        y: 8 * 25
        config: parent.config
        prop: "edgeSharpness"
        label: qsTr("Edge Sharpness")
        min: 0; max: 1
    }
    ConfigSlider {
        y: 9 * 25
        config: parent.config
        prop: "blurRadius"
        label: qsTr("Blur Radius")
        min: 0; max: 6
    }
    ConfigSlider {
        y: 10 * 25
        config: parent.config
        prop: "blurDeviation"
        label: qsTr("Blur Deviation")
        min: 0; max: 3
    }
}

