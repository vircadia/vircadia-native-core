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
        property var mainViewTask: Render.getConfig("RenderMainView");

       Column{
            CheckBox {
                text: "Scattering"
                checked: mainViewTask.getConfig("Scattering").enableScattering
                onCheckedChanged: { mainViewTask.getConfig("Scattering").enableScattering = checked }
            }

            CheckBox {
                text: "Show Scattering BRDF"
                checked: mainViewTask.getConfig("Scattering").showScatteringBRDF
                onCheckedChanged: { mainViewTask.getConfig("Scattering").showScatteringBRDF = checked }
            }
            CheckBox {
                text: "Show Curvature"
                checked: mainViewTask.getConfig("Scattering").showCurvature
                onCheckedChanged: { mainViewTask.getConfig("Scattering").showCurvature = checked }
            }
            CheckBox {
                text: "Show Diffused Normal"
                checked: mainViewTask.getConfig("Scattering").showDiffusedNormal
                onCheckedChanged: { mainViewTask.getConfig("Scattering").showDiffusedNormal = checked }
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
                checked: mainViewTask.getConfig("DebugScattering").showProfile
                onCheckedChanged: { mainViewTask.getConfig("DebugScattering").showProfile = checked }
            }
            CheckBox {
                text: "Scattering Table"
                checked: mainViewTask.getConfig("DebugScattering").showLUT
                onCheckedChanged: { mainViewTask.getConfig("DebugScattering").showLUT = checked }
            }
            CheckBox {
                text: "Cursor Pixel"
                checked: mainViewTask.getConfig("DebugScattering").showCursorPixel
                onCheckedChanged: { mainViewTask.getConfig("DebugScattering").showCursorPixel = checked }
            }
            CheckBox {
                text: "Skin Specular Beckmann"
                checked: mainViewTask.getConfig("DebugScattering").showSpecularTable
                onCheckedChanged: { mainViewTask.getConfig("DebugScattering").showSpecularTable = checked }
            }
        }
    }
}
