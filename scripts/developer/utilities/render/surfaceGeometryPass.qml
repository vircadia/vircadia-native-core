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

        Column{
                ConfigSlider {
                    label: qsTr("Depth Threshold [cm]")
                    integral: false
                    config: Render.getConfig("SurfaceGeometry")
                    property: "depthThreshold"
                    max: 5.0
                    min: 0.0
                }
            Repeater {
                model: [
                    "Basis Scale:basisScale:2.0:false",
                    "Curvature Scale:curvatureScale:100.0:false",
                    "Downscale:resolutionLevel:4:true"
                ]
                ConfigSlider {
                    label: qsTr(modelData.split(":")[0])
                    integral: (modelData.split(":")[3] == 'true')
                    config: Render.getConfig("SurfaceGeometry")
                    property: modelData.split(":")[1]
                    max: modelData.split(":")[2]
                    min: 0.0
                }
            }
        }

        Column{
            CheckBox {
                text: "Diffuse Curvature Mid"
                checked: true
                onCheckedChanged: { Render.getConfig("DiffuseCurvatureMid").enabled = checked }
            }        
            Repeater {
                model: [ "Blur Scale:DiffuseCurvatureMid:filterScale:2.0", "Blur Depth Threshold:DiffuseCurvatureMid:depthThreshold:1.0", "Blur Scale2:DiffuseCurvatureLow:filterScale:2.0", "Blur Depth Threshold 2:DiffuseCurvatureLow:depthThreshold:1.0"]
                ConfigSlider {
                    label: qsTr(modelData.split(":")[0])
                    integral: false
                    config: Render.getConfig(modelData.split(":")[1])
                    property: modelData.split(":")[2]
                    max: modelData.split(":")[3]
                    min: 0.0
                }
            }

            CheckBox {
                text: "Diffuse Curvature Low"
                checked: true
                onCheckedChanged: { Render.getConfig("DiffuseCurvatureLow").enabled = checked }
            }
        }
    }
}
