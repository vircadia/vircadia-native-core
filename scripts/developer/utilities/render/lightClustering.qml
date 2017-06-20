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
        property var mainViewTask: Render.getConfig("RenderMainView");

        Column{
            PlotPerf {
                title: "Light CLustering Timing"
                height: 50
                object: mainViewTask.getConfig("LightClustering")
                valueUnit: "ms"
                valueScale: 1
                valueNumDigits: "4"
                plots: [
                    {
                       object: mainViewTask.getConfig("LightClustering"),
                       prop: "cpuRunTime",
                       label: "time",
                       scale: 1,
                       color: "#FFFFFF"
                   }
                ]
            }

            PlotPerf {
                title: "Lights"
                height: 50
                object: mainViewTask.getConfig("LightClustering")
                valueUnit: ""
                valueScale: 1
                valueNumDigits: "0"
                plots: [
                    {
                       object: mainViewTask.getConfig("LightClustering"),
                       prop: "numClusteredLights",
                       label: "visible",
                       color: "#D959FE"
                   },
                   {
                        object: mainViewTask.getConfig("LightClustering"),
                        prop: "numInputLights",
                        label: "input",
                        color: "#FED959"
                    }
                ]
            }

             PlotPerf {
                title: "Scene Lights"
                height: 80
                object: mainViewTask.getConfig("LightClustering")
                valueUnit: ""
                valueScale: 1
                valueNumDigits: "0"
                plots: [
                    {
                       object: mainViewTask.getConfig("LightClustering"),
                       prop: "numSceneLights",
                       label: "current",
                       color: "#00B4EF"
                   },
                   {
                        object: mainViewTask.getConfig("LightClustering"),
                        prop: "numFreeSceneLights",
                        label: "free",
                        color: "#1AC567"
                    },
                   {
                        object: mainViewTask.getConfig("LightClustering"),
                        prop: "numAllocatedSceneLights",
                        label: "allocated",
                        color: "#9495FF"
                    }
                ]
            }

            ConfigSlider {
                label: qsTr("Range Near [m]")
                integral: false
                config: mainViewTask.getConfig("LightClustering")
                property: "rangeNear"
                max: 20.0
                min: 0.1
            }
            ConfigSlider {
                label: qsTr("Range Far [m]")
                integral: false
                config: mainViewTask.getConfig("LightClustering")
                property: "rangeFar"
                max: 500.0
                min: 100.0
            }
            ConfigSlider {
                label: qsTr("Grid X")
                integral: true
                config: mainViewTask.getConfig("LightClustering")
                property: "dimX"
                max: 32
                min: 1
            }
            ConfigSlider {
                label: qsTr("Grid Y")
                integral: true
                config: mainViewTask.getConfig("LightClustering")
                property: "dimY"
                max: 32
                min: 1
            }
            ConfigSlider {
                label: qsTr("Grid Z")
                integral: true
                config: mainViewTask.getConfig("LightClustering")
                property: "dimZ"
                max: 31
                min: 1
            }
            CheckBox {
                    text: "Freeze"
                    checked: mainViewTask.getConfig("LightClustering")["freeze"]
                    onCheckedChanged: { mainViewTask.getConfig("LightClustering")["freeze"] = checked }
            }
            CheckBox {
                    text: "Draw Grid"
                    checked: mainViewTask.getConfig("DebugLightClusters")["doDrawGrid"]
                    onCheckedChanged: { mainViewTask.getConfig("DebugLightClusters")["doDrawGrid"] = checked }
            }
            CheckBox {
                    text: "Draw Cluster From Depth"
                    checked: mainViewTask.getConfig("DebugLightClusters")["doDrawClusterFromDepth"]
                    onCheckedChanged: { mainViewTask.getConfig("DebugLightClusters")["doDrawClusterFromDepth"] = checked }
            }
            CheckBox {
                    text: "Draw Content"
                    checked: mainViewTask.getConfig("DebugLightClusters")["doDrawContent"]
                    onCheckedChanged: { mainViewTask.getConfig("DebugLightClusters")["doDrawContent"] = checked }
            }
            Label {
                text:  "Num Cluster Items = " + mainViewTask.getConfig("LightClustering")["numClusteredLightReferences"].toFixed(0)
            }
            
        }
    }
}
