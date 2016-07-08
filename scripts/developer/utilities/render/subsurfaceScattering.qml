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
                checked: Render.getConfig("Scattering").enableScattering
                onCheckedChanged: { Render.getConfig("Scattering").enableScattering = checked }
            }

            CheckBox {
                text: "Show Scattering BRDF"
                checked: Render.getConfig("Scattering").showScatteringBRDF
                onCheckedChanged: { Render.getConfig("Scattering").showScatteringBRDF = checked }
            }
            CheckBox {
                text: "Show Curvature"
                checked: Render.getConfig("Scattering").showCurvature
                onCheckedChanged: { Render.getConfig("Scattering").showCurvature = checked }
            }
            CheckBox {
                text: "Show Diffused Normal"
                checked: Render.getConfig("Scattering").showDiffusedNormal
                onCheckedChanged: { Render.getConfig("Scattering").showDiffusedNormal = checked }
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
                    config: Render.getConfig(modelData.split(":")[1])
                    property: modelData.split(":")[2]
                    max: modelData.split(":")[3]
                    min: 0.0
                }
            }
            CheckBox {
                text: "Scattering Profile"
                checked: Render.getConfig("DebugScattering").showProfile
                onCheckedChanged: { Render.getConfig("DebugScattering").showProfile = checked }
            }
            CheckBox {
                text: "Scattering Table"
                checked: Render.getConfig("DebugScattering").showLUT
                onCheckedChanged: { Render.getConfig("DebugScattering").showLUT = checked }
            }
            CheckBox {
                text: "Cursor Pixel"
                checked: Render.getConfig("DebugScattering").showCursorPixel
                onCheckedChanged: { Render.getConfig("DebugScattering").showCursorPixel = checked }
            }
            CheckBox {
                text: "Skin Specular Beckmann"
                checked: Render.getConfig("DebugScattering").showSpecularTable
                onCheckedChanged: { Render.getConfig("DebugScattering").showSpecularTable = checked }
            }
        }
    }
}
