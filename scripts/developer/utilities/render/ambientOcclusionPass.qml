//
//  surfaceGeometryPass.qml
//
//  Created by Sam Gateau on 6/6/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import "configSlider"
import "../lib/plotperf"

Column {
    spacing: 8
    Column {
        id: surfaceGeometry
        spacing: 10

        Column{
            Repeater {
                model: [
                    "Radius:radius:2.0:false",
                    "Level:obscuranceLevel:1.0:false",
                    "Num Taps:numSamples:32:true",
                    "Taps Spiral:numSpiralTurns:10.0:false",
                    "Blur Radius:blurRadius:10.0:false",
                ]
                ConfigSlider {
                    label: qsTr(modelData.split(":")[0])
                    integral: (modelData.split(":")[3] == 'true')
                    config: Render.getConfig("AmbientOcclusion")
                    property: modelData.split(":")[1]
                    max: modelData.split(":")[2]
                    min: 0.0
                }
            }
        }

        Column {
            Repeater {
                model: [
                    "resolutionLevel:resolutionLevel",
                    "ditheringEnabled:ditheringEnabled",
                    "borderingEnabled:borderingEnabled",
                    "fetchMipsEnabled:fetchMipsEnabled",
                ]
                CheckBox {
                    text: qsTr(modelData.split(":")[0])
                    checked: Render.getConfig("AmbientOcclusion")[modelData.split(":")[1]]
                    onCheckedChanged: { Render.getConfig("AmbientOcclusion")[modelData.split(":")[1]] = checked }
                }
            }
        }

        PlotPerf {
            title: "Timing"
            height: 50
            object: Render.getConfig("AmbientOcclusion")
            valueUnit: "ms"
            valueScale: 1
            valueNumDigits: "4"
            plots: [
            {
                   prop: "gpuTime",
                   label: "gpu",
                   color: "#FFFFFF"
               }
            ]
        }
    }
}
