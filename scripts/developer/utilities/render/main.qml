//
//  main.qml
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
import "configSlider"

Column {
    id: root
    spacing: 16
    Switch {
        checked: true
        onClicked: ui.visible = checked
    }

    Column {
        id: ui
        spacing: 8

        Repeater {
            model: [ "Opaque:DrawOpaqueDeferred", "Transparent:DrawTransparentDeferred",
                    "Opaque Overlays:DrawOverlay3DOpaque", "Transparent Overlays:DrawOverlay3DTransparent" ]
            ConfigSlider {
                label: qsTr(modelData.split(":")[0])
                integral: true
                config: Render.getConfig(modelData.split(":")[1])
                property: "maxDrawn"
                max: config.numDrawn
                min: -1
            }
        }

        Row {
            CheckBox {
                text: qsTr("Display Status")
                onCheckedChanged: { Render.getConfig("DrawStatus").showDisplay = checked }
            }
            CheckBox {
                text: qsTr("Network/Physics Status")
                onCheckedChanged: { Render.getConfig("DrawStatus").showNetwork = checked }
            }
        }

        ConfigSlider {
            label: qsTr("Tone Mapping Exposure")
            config: Render.getConfig("ToneMapping")
            property: "exposure"
            min: -10; max: 10
        }

        Column {
            id: ambientOcclusion
            property var config: Render.getConfig("AmbientOcclusion")

            Label { text: qsTr("Ambient Occlusion") }
            // TODO: Add gpuTimer
            CheckBox { text: qsTr("Dithering"); checked: ambientOcclusion.config.ditheringEnabled }
            Repeater {
                model: [
                    "Resolution Level:resolutionLevel:4",
                    "Obscurance Level:obscuranceLevel:1",
                    "Radius:radius:2",
                    "Falloff Bias:falloffBias:0.2",
                    "Edge Sharpness:edgeSharpness:1",
                    "Blur Radius:blurRadius:6",
                    "Blur Deviation:blurDeviation:3"
                ]
                ConfigSlider {
                    label: qsTr(modelData.split(":")[0])
                    config: ambientOcclusion.config
                    property: modelData.split(":")[1]
                    max: modelData.split(":")[2]
                }
            }
            Repeater {
                model: [
                    "Samples:numSamples:32",
                    "Spiral Turns:numSpiralTurns:30:"
                ]
                ConfigSlider {
                    label: qsTr(modelData.split(":")[0])
                    integral: true
                    config: ambientOcclusion.config
                    property: modelData.split(":")[1]
                    max: modelData.split(":")[2]
                }
            }
        }

        Column {
            id: debug
            property var config: Render.getConfig("DebugDeferredBuffer")

            function setDebugMode(mode) {
                debug.config.enabled = (mode != 0);
                debug.config.mode = mode;
            }

            Label { text: qsTr("Debug Buffer") }
            ExclusiveGroup { id: bufferGroup }
            Repeater {
                model: [
                    "Off", "Diffuse", "Metallic", "Roughness", "Normal", "Depth",
                    "Lighting", "Shadow", "Pyramid Depth", "Ambient Occlusion", "Custom Shader"
                ]
               RadioButton {
                    text: qsTr(modelData)
                    exclusiveGroup: bufferGroup
                    checked: index == 0
                    onCheckedChanged: if (checked && index > 0) debug.setDebugMode(index - 1);
                }
            }
        }
    }
}
