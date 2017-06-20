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

Column {
    spacing: 8
    Column {
        id: surfaceGeometry
        spacing: 10
        property var mainViewTask: Render.getConfig("RenderMainView");

        Column{
                ConfigSlider {
                    label: qsTr("Depth Threshold [cm]")
                    integral: false
                    config: mainViewTask.getConfig("SurfaceGeometry")
                    property: "depthThreshold"
                    max: 5.0
                    min: 0.0
                }
            Repeater {
                model: [
                    "Basis Scale:basisScale:2.0:false",
                    "Curvature Scale:curvatureScale:100.0:false",
                ]
                ConfigSlider {
                    label: qsTr(modelData.split(":")[0])
                    integral: (modelData.split(":")[3] == 'true')
                    config: mainViewTask.getConfig("SurfaceGeometry")
                    property: modelData.split(":")[1]
                    max: modelData.split(":")[2]
                    min: 0.0
                }
            }
            CheckBox {
                    text: "Half Resolution"
                    checked: mainViewTask.getConfig("SurfaceGeometry")["resolutionLevel"]
                    onCheckedChanged: { mainViewTask.getConfig("SurfaceGeometry")["resolutionLevel"] = checked }
            }
    
            Repeater {
                model: [ "Diffusion Scale:SurfaceGeometry:diffuseFilterScale:2.0",
                         "Diffusion Depth Threshold:SurfaceGeometry:diffuseDepthThreshold:1.0"
                ]
                ConfigSlider {
                    label: qsTr(modelData.split(":")[0])
                    integral: false
                    config: mainViewTask.getConfig(modelData.split(":")[1])
                    property: modelData.split(":")[2]
                    max: modelData.split(":")[3]
                    min: 0.0
                }
            }
        }
    }
}
