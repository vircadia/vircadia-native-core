//
//  surfaceGeometryPass.qml
//
//  Created by Sam Gateau on 6/6/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3

import "qrc:///qml/styles-uit"
import "qrc:///qml/controls-uit" as HifiControls
    
import "configSlider"
import "../lib/plotperf"

Rectangle {
    HifiConstants { id: hifi;}
    id: render;   
    anchors.margins: hifi.dimensions.contentMargin.x
    
    color: hifi.colors.baseGray;
    
    Column {
        id: surfaceGeometry
        spacing: 10
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: hifi.dimensions.contentMargin.x  

        Repeater {
            model: [
                "Radius:radius:2.0:false",
                "Level:obscuranceLevel:1.0:false",
                "Num Taps:numSamples:32:true",
                "Taps Spiral:numSpiralTurns:10.0:false",
                "Falloff Bias:falloffBias:0.2:false",
                "Edge Sharpness:edgeSharpness:1.0:false",
                "Blur Radius:blurRadius:10.0:false",
            ]
            ConfigSlider {
                label: qsTr(modelData.split(":")[0])
                integral: (modelData.split(":")[3] == 'true')
                config: Render.getConfig("RenderMainView.AmbientOcclusion")
                property: modelData.split(":")[1]
                max: modelData.split(":")[2]
                min: 0.0
                width: 280
                height:38
            }
        }

        Row{
            spacing: 10
            anchors.left: parent.left
            anchors.right: parent.right 

            Column {
                spacing: 10
            
                Repeater {
                    model: [
                        "resolutionLevel:resolutionLevel",
                        "ditheringEnabled:ditheringEnabled",
                        "fetchMipsEnabled:fetchMipsEnabled",
                        "borderingEnabled:borderingEnabled"
                    ]
                    HifiControls.CheckBox {
                        boxSize: 20
                        text: qsTr(modelData.split(":")[0])
                        checked: Render.getConfig("RenderMainView.AmbientOcclusion")[modelData.split(":")[1]]
                        onCheckedChanged: { Render.getConfig("RenderMainView.AmbientOcclusion")[modelData.split(":")[1]] = checked }
                    } 
                }
            }

            Column {
                spacing: 10
            
                Repeater {
                    model: [
                       "debugEnabled:showCursorPixel"
                    ]
                    HifiControls.CheckBox {
                        boxSize: 20
                        text: qsTr(modelData.split(":")[0])
                        checked: Render.getConfig("RenderMainView.DebugAmbientOcclusion")[modelData.split(":")[1]]
                        onCheckedChanged: { Render.getConfig("RenderMainView.DebugAmbientOcclusion")[modelData.split(":")[1]] = checked }
                    } 
                }
            }    
        }

        PlotPerf {
            anchors.left: parent.left
            anchors.right: parent.right
            title: "Timing"
            height: 50
            object: Render.getConfig("RenderMainView.AmbientOcclusion")
            valueUnit: "ms"
            valueScale: 1
            valueNumDigits: "3"
            plots: [
            {
                   prop: "gpuRunTime",
                   label: "gpu",
                   color: "#FFFFFF"
               }
            ]
        }
    }
}
