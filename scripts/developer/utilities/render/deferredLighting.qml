//
//  deferredLighting.qml
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
        id: deferredLighting
        spacing: 10

        CheckBox {
            text: "Point Lights"
            checked: true
            onCheckedChanged: { Render.getConfig("RenderDeferred").enablePointLights = checked }
        }
        CheckBox {
            text: "Spot Lights"
            checked: true
            onCheckedChanged: { Render.getConfig("RenderDeferred").enableSpotLights = checked }
        }

        Column{
            CheckBox {
                text: "Scattering"
                checked: true
                onCheckedChanged: { Render.getConfig("RenderDeferred").enableScattering = checked }
            }

            CheckBox {
                text: "Show Scattering BRDF"
                checked: Render.getConfig("RenderDeferred").showScatteringBRDF
                onCheckedChanged: { Render.getConfig("RenderDeferred").showScatteringBRDF = checked }
            }
            Repeater {
                model: [ "Scattering Bent Red:RenderDeferred:bentRed:2.0",
                         "Scattering Bent Green:RenderDeferred:bentGreen:2.0",
                         "Scattering Bent Blue:RenderDeferred:bentBlue:2.0",
                         "Scattering Bent Scale:RenderDeferred:bentScale:5.0",
                         "Scattering Curvature Offset:RenderDeferred:curvatureOffset:1.0",
                         "Scattering Curvature Scale:RenderDeferred:curvatureScale:2.0",
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
    }
}
