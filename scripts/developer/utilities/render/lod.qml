//
//  lod.qml
//  scripts/developer/utilities/render
//
//  Created by Andrew Meadows on 2018.01.10
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4

import stylesUit 1.0
import controlsUit 1.0 as HifiControls

import "../lib/plotperf"
import "configSlider"

Item {
    id: lodIU
    anchors.fill:parent

    Component.onCompleted: {
        Render.getConfig("RenderMainView.DrawSceneOctree").showVisibleCells = false
        Render.getConfig("RenderMainView.DrawSceneOctree").showEmptyCells = false
    }

    Component.onDestruction: {
        Render.getConfig("RenderMainView.DrawSceneOctree").enabled = false
    }

    Column {
        id: topHeader
        spacing: 8
        anchors.right: parent.right
        anchors.left: parent.left

        HifiControls.CheckBox {
            boxSize: 20
            text: "Show LOD Reticule"
            checked: Render.getConfig("RenderMainView.DrawSceneOctree").enabled
            onCheckedChanged: { Render.getConfig("RenderMainView.DrawSceneOctree").enabled = checked }
        }

        RichSlider {
            showLabel: true
            showValue: false
            label: "World Quality"
            valueVar: LODManager["worldDetailQuality"]
            valueVarSetter: (function (v) { LODManager["worldDetailQuality"] = v })
            max: 0.75
            min: 0.25
            integral: false

            anchors.left: parent.left
            anchors.right: parent.right 
        }

        Row {
            HifiControls.CheckBox {
                id: autoLOD
                boxSize: 20
                text: "Auto LOD"
                checked: LODManager.automaticLODAdjust
                onCheckedChanged: { LODManager.automaticLODAdjust = (checked) } 
            }
            HifiControls.CheckBox {
                id: showLODRegulatorDetails
                visible: LODManager.automaticLODAdjust
                boxSize: 20
                text: "Show LOD Details"
            }
        }

        RichSlider {
            visible: !LODManager.automaticLODAdjust
            showLabel: true
            label: "LOD Angle [deg]"
            valueVar: LODManager["lodAngleDeg"]
            valueVarSetter: (function (v) { LODManager["lodAngleDeg"] = v })
            max: 90.0
            min: 0.01
            integral: false

            anchors.left: parent.left
            anchors.right: parent.right 
        }
        Column {
            id: lodRegulatorDetails
            visible: LODManager.automaticLODAdjust && showLODRegulatorDetails.checked 
            anchors.left: parent.left
            anchors.right: parent.right 
            RichSlider {
                visible: lodRegulatorDetails.visible
                showLabel: true
                label: "LOD Kp"
                valueVar: LODManager["pidKp"]
                valueVarSetter: (function (v) { LODManager["pidKp"] = v })
                max: 2.0
                min: 0.0
                integral: false
                numDigits: 3

                anchors.left: parent.left
                anchors.right: parent.right 
            }
            RichSlider {
                visible: false && lodRegulatorDetails.visible
                showLabel: true
                label: "LOD Ki"
                valueVar: LODManager["pidKi"]
                valueVarSetter: (function (v) { LODManager["pidKi"] = v })
                max: 0.1
                min: 0.0
                integral: false
                numDigits: 8

                anchors.left: parent.left
                anchors.right: parent.right 
            }
            RichSlider {
                visible: false && lodRegulatorDetails.visible
                showLabel: true
                label: "LOD Kd"
                valueVar: LODManager["pidKd"]
                valueVarSetter: (function (v) { LODManager["pidKd"] = v })
                max: 10.0
                min: 0.0
                integral: false
                numDigits: 3

                anchors.left: parent.left
                anchors.right: parent.right 
            }
            RichSlider {
                visible: lodRegulatorDetails.visible
                showLabel: true
                label: "LOD Kv"
                valueVar: LODManager["pidKv"]
                valueVarSetter: (function (v) { LODManager["pidKv"] = v })
                max: 2.0
                min: 0.0
                integral: false

                anchors.left: parent.left
                anchors.right: parent.right 
            }
            RichSlider {
                visible: lodRegulatorDetails.visible
                showLabel: true
                label: "LOD Smooth Scale"
                valueVar: LODManager["smoothScale"]
                valueVarSetter: (function (v) { LODManager["smoothScale"] = v })
                max: 20.0
                min: 1.0
                integral: true

                anchors.left: parent.left
                anchors.right: parent.right 
            }
        }
    }

    Column {
        id: stats
        spacing: 4
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: topHeader.bottom
        anchors.bottom: parent.bottom

        function evalEvenHeight() {
            // Why do we have to do that manually ? cannot seem to find a qml / anchor / layout mode that does that ?
            var numPlots = (children.length + (lodRegulatorDetails.visible ? 1 : 0) - 2)
            return (height - topLine.height - bottomLine.height - spacing * (numPlots - 1)) / (numPlots)
        }

        Separator {
            id: topLine
        }

        PlotPerf {
            title: "Load Indicators"
            height: parent.evalEvenHeight()
            object: LODManager
            valueScale: 1
            valueUnit: "ms"
            plots: [
                {
                    prop: "presentTime",
                    label: "present",
                    color: "#FFFF00"
                },
                {
                    prop: "batchTime",
                    label: "batch",
                    color: "#00FF00"
                },
                {
                    prop: "engineRunTime",
                    label: "engineRun",
                    color: "#FF00FF"
                },
                {
                    prop: "gpuTime",
                    label: "gpu",
                    color: "#00FFFF"
                }
            ]
        }
        PlotPerf {
            title: "FPS"
            height: parent.evalEvenHeight()
            object: LODManager
            valueScale: 1
            valueUnit: "Hz"
            plots: [
                {
                    prop: "lodTargetFPS",
                    label: "target",
                    color: "#66FF66"
                },
                {
                    prop: "nowRenderFPS",
                    label: "FPS",
                    color: "#FFFF55"
                },
                {
                    prop: "smoothRenderFPS",
                    label: "Smooth FPS",
                    color: "#9999FF"
                }
            ]
        }
        PlotPerf {
            title: "LOD Angle"
            height: parent.evalEvenHeight()
            object: LODManager
            valueScale: 1.0
            valueUnit: "deg"
            valueNumDigits: 2
            plots: [
                {
                    prop: "lodAngleDeg",
                    label: "LOD Angle",
                    color: "#9999FF"
                }
            ]
        }
        PlotPerf {
          //  visible: lodRegulatorDetails.visible
            title: "PID Output"
            height: parent.evalEvenHeight()
            object: LODManager
            valueScale: 1.0
            valueUnit: "deg"
            valueNumDigits: 1
            displayMinAt0: false
            plots: [
                {
                    prop: "pidOp",
                    label: "Op",
                    color: "#9999FF"
                },
                {
                    prop: "pidOi",
                    label: "Oi",
                    color: "#FFFFFF"
                },
                {
                    prop: "pidOd",
                    label: "Od",
                    color: "#FF6666"
                },
                {
                    prop: "pidO",
                    label: "Output",
                    color: "#66FF66"
                }
            ]
        }      
        Separator {
            id: bottomLine
        }
    }
}
