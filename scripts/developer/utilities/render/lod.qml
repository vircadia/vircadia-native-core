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

import "qrc:///qml/styles-uit"
import "qrc:///qml/controls-uit" as HifiControls

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

            anchors.left: autoLOD.left
            anchors.right: parent.right 
        }

        HifiControls.CheckBox {
            id: autoLOD
            boxSize: 20
            text: "Auto LOD"
            checked: LODManager.automaticLODAdjust
            onCheckedChanged: { LODManager.automaticLODAdjust = (checked) } 
        }
 
        RichSlider {
            visible: !LODManager.automaticLODAdjust
            showLabel: true
            label: "LOD Angle [deg]"
            valueVar: LODManager["lodAngleDeg"]
            valueVarSetter: (function (v) { LODManager["lodAngleDeg"] = v })
            max: 90.0
            min: 0.5
            integral: false

            anchors.left: parent.left
            anchors.right: parent.right 
        }

        RichSlider {
            visible: LODManager.automaticLODAdjust
            showLabel: true
            label: "LOD PID Kp"
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
            visible: LODManager.automaticLODAdjust
            showLabel: true
            label: "LOD PID Ki"
            valueVar: LODManager["pidKi"]
            valueVarSetter: (function (v) { LODManager["pidKi"] = v })
            max: 0.000005
            min: 0.0
            integral: false
            numDigits: 8

            anchors.left: parent.left
            anchors.right: parent.right 
        }
        RichSlider {
            visible: LODManager.automaticLODAdjust
            showLabel: true
            label: "LOD PID Kd"
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
            visible: LODManager.automaticLODAdjust
            showLabel: true
            label: "LOD PID Num T"
            valueVar: LODManager["pidT"]
            valueVarSetter: (function (v) { LODManager["pidT"] = v })
            max: 10.0
            min: 0.0
            integral: true

            anchors.left: parent.left
            anchors.right: parent.right 
        }
    }

    Column {
        id: stats
        spacing: 8
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: topHeader.bottom
        anchors.bottom: parent.bottom

        function evalEvenHeight() {
            // Why do we have to do that manually ? cannot seem to find a qml / anchor / layout mode that does that ?
            return (height - topLine.height - bottomLine.height - spacing * (children.length - 3)) / (children.length - 2)
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
                    prop: "lodIncreaseFPS",
                    label: "LOD++",
                    color: "#66FF66"
                },
                {
                    prop: "fps",
                    label: "FPS",
                    color: "#FFFFFF"
                },
                {
                    prop: "lodDecreaseFPS",
                    label: "LOD--",
                    color: "#FF6666"
                }
            ]
        }
        PlotPerf {
            title: "LOD Angle"
            height: parent.evalEvenHeight()
            object: LODManager
            valueScale: 1.0
            valueUnit: "deg"
            plots: [
                {
                    prop: "lodAngleDeg",
                    label: "LOD Angle",
                    color: "#9999FF"
                }
            ]
        }
        PlotPerf {
            title: "PID Output"
            height: parent.evalEvenHeight()
            object: LODManager
            valueScale: 1.0
            valueUnit: "deg"
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
                }
            ]
        }      
        Separator {
            id: bottomLine
        }
    }
}
