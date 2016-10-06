//
//  lightClustering.qml
//
//  Created by Sam Gateau on 9/9/2016
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
        id: lightClustering
        spacing: 10

        Column{
            PlotPerf {
                title: "Light CLustering Timing"
                height: 50
                object: Render.getConfig("LightClustering")
                valueUnit: "ms"
                valueScale: 1
                valueNumDigits: "4"
                plots: [
                    {
                       object: Render.getConfig("LightClustering"),
                       prop: "cpuRunTime",
                       label: "time",
                       scale: 1,
                       color: "#FFFFFF"
                   },
                   {
                        object: Render.getConfig("DrawLight"),
                        prop: "numDrawn", 
                        unit: "",
                        scale: 0.01,
                        numDigits: 0,
                        label: "Lights",
                        color: "#FED959"
                    }
                ]
            }

            ConfigSlider {
                label: qsTr("Range Near [m]")
                integral: false
                config: Render.getConfig("LightClustering")
                property: "rangeNear"
                max: 20.0
                min: 0.1
            }
            ConfigSlider {
                label: qsTr("Range Far [m]")
                integral: false
                config: Render.getConfig("LightClustering")
                property: "rangeFar"
                max: 500.0
                min: 100.0
            }
            ConfigSlider {
                label: qsTr("Grid X")
                integral: true
                config: Render.getConfig("LightClustering")
                property: "dimX"
                max: 32
                min: 1
            }
            ConfigSlider {
                label: qsTr("Grid Y")
                integral: true
                config: Render.getConfig("LightClustering")
                property: "dimY"
                max: 32
                min: 1
            }
            ConfigSlider {
                label: qsTr("Grid Z")
                integral: true
                config: Render.getConfig("LightClustering")
                property: "dimZ"
                max: 31
                min: 1
            }
            CheckBox {
                    text: "Freeze"
                    checked: Render.getConfig("LightClustering")["freeze"]
                    onCheckedChanged: { Render.getConfig("LightClustering")["freeze"] = checked }
            }
            CheckBox {
                    text: "Draw Grid"
                    checked: Render.getConfig("DebugLightClusters")["doDrawGrid"]
                    onCheckedChanged: { Render.getConfig("DebugLightClusters")["doDrawGrid"] = checked }
            }
            CheckBox {
                    text: "Draw Cluster From Depth"
                    checked: Render.getConfig("DebugLightClusters")["doDrawClusterFromDepth"]
                    onCheckedChanged: { Render.getConfig("DebugLightClusters")["doDrawClusterFromDepth"] = checked }
            }
            CheckBox {
                    text: "Draw Content"
                    checked: Render.getConfig("DebugLightClusters")["doDrawContent"]
                    onCheckedChanged: { Render.getConfig("DebugLightClusters")["doDrawContent"] = checked }
            }
            Label {
                text:  "Num Cluster Items = " + Render.getConfig("LightClustering")["numClusteredLights"].toFixed(0)
            }
            
        }
    }
}
