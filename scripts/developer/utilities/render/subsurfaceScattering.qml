//
//  subsurfaceScattering.qml
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
        id: scattering
        spacing: 10

        Column{
            CheckBox {
                text: "Scattering"
                checked: Render.getConfig("RenderMainView.Scattering").enableScattering
                onCheckedChanged: { Render.getConfig("RenderMainView.Scattering").enableScattering = checked }
            }

            CheckBox {
                text: "Show Scattering BRDF"
                checked: Render.getConfig("RenderMainView.Scattering").showScatteringBRDF
                onCheckedChanged: { Render.getConfig("RenderMainView.Scattering").showScatteringBRDF = checked }
            }
            CheckBox {
                text: "Show Curvature"
                checked: Render.getConfig("RenderMainView.Scattering").showCurvature
                onCheckedChanged: { Render.getConfig("RenderMainView.Scattering").showCurvature = checked }
            }
            CheckBox {
                text: "Show Diffused Normal"
                checked: Render.getConfig("RenderMainView.Scattering").showDiffusedNormal
                onCheckedChanged: { Render.getConfig("RenderMainView.Scattering").showDiffusedNormal = checked }
            }
            Repeater {
                model: [ "Scattering Bent Red:Scattering:bentRed:2.0",
                         "Scattering Bent Green:Scattering:bentGreen:2.0",
                         "Scattering Bent Blue:Scattering:bentBlue:2.0",
                         "Scattering Bent Scale:Scattering:bentScale:5.0",
                         "Scattering Curvature Offset:Scattering:curvatureOffset:1.0",
                         "Scattering Curvature Scale:Scattering:curvatureScale:2.0",
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
            CheckBox {
                text: "Scattering Profile"
                checked: Render.getConfig("RenderMainView.DebugScattering").showProfile
                onCheckedChanged: { Render.getConfig("RenderMainView.DebugScattering").showProfile = checked }
            }
            CheckBox {
                text: "Scattering Table"
                checked: Render.getConfig("RenderMainView.DebugScattering").showLUT
                onCheckedChanged: { Render.getConfig("RenderMainView.DebugScattering").showLUT = checked }
            }
            CheckBox {
                text: "Cursor Pixel"
                checked: Render.getConfig("RenderMainView.DebugScattering").showCursorPixel
                onCheckedChanged: { Render.getConfig("RenderMainView.DebugScattering").showCursorPixel = checked }
            }
            CheckBox {
                text: "Skin Specular Beckmann"
                checked: Render.getConfig("RenderMainView.DebugScattering").showSpecularTable
                onCheckedChanged: { Render.getConfig("RenderMainView.DebugScattering").showSpecularTable = checked }
            }
        }
    }
}
