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
            Repeater {
                model: [ "Depth Threshold:depthThreshold:1.0", "Basis Scale:basisScale:1.0", "Curvature Scale:curvatureScale:200.0" ]
                ConfigSlider {
                    label: qsTr(modelData.split(":")[0])
                    integral: false
                    config: Render.getConfig("SurfaceGeometry")
                    property: modelData.split(":")[1]
                    max: modelData.split(":")[2]
                    min: 0.0
                }
            }
        }

        Column{
            Repeater {
                model: [ "Blur Scale:DiffuseCurvature:filterScale:2.0", "Blur Depth Threshold:DiffuseCurvature:depthThreshold:10.0", "Blur Scale2:DiffuseCurvature2:filterScale:2.0", "Blur Depth Threshold 2:DiffuseCurvature2:depthThreshold:10.0"]
                ConfigSlider {
                    label: qsTr(modelData.split(":")[0])
                    integral: false
                    config: Render.getConfig(modelData.split(":")[1])
                    property: modelData.split(":")[2]
                    max: modelData.split(":")[3]
                    min: 0.0
                }
            }
        }

        Column{
            Repeater {
                model: [ "Scattering Bent Red:Scattering:bentRed:2.0",
                         "Scattering Bent Green:Scattering:bentGreen:2.0",
                         "Scattering Bent Blue:Scattering:bentBlue:2.0",
                         "Scattering Bent Scale:Scattering:bentScale:2.0",
                         "Scattering Curvature Offset:Scattering:curvatureOffset:1.0",
                         "Scattering Curvature Scale:Scattering:curvatureScale:1.0",
                          ]
                ConfigSlider {
                    label: qsTr(modelData.split(":")[0])
                    integral: false
                    config: Render.getConfig(modelData.split(":")[1])
                    property: modelData.split(":")[2]
                    max: modelData.split(":")[3]
                    min: 0.0
                }
            }
        }
        CheckBox {
            text: "Show scatteringLUT"
            checked: false
            onCheckedChanged: { Render.getConfig("Scattering").showLUT = checked }
        }

    }
}
