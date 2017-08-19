//
// Antialiasing.qml
//
//  Created by Sam Gateau on 8/14/2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import "configSlider"
import "../lib/plotperf"

Column {
    spacing: 8
    Column {
        id: antialiasing
        spacing: 10

        Column{
            ConfigSlider {
                label: qsTr("Debug X")
                integral: false
                config: Render.getConfig("RenderMainView.Antialiasing")
                property: "debugX"
                max: 1.0
                min: 0.0
            }
            ConfigSlider {
                label: qsTr("Source blend")
                integral: false
                config: Render.getConfig("RenderMainView.Antialiasing")
                property: "blend"
                max: 1.0
                min: 0.0
            }
            ConfigSlider {
                label: qsTr("Velocity scale")
                integral: false
                config: Render.getConfig("RenderMainView.Antialiasing")
                property: "velocityScale"
                max: 1.0
                min: 0.0
            }
            ConfigSlider {
                label: qsTr("Debug Velocity Threshold [pix]")
                integral: false
                config: Render.getConfig("RenderMainView.Antialiasing")
                property: "debugShowVelocityThreshold"
                max: 20
                min: 0.0
            }
            CheckBox {
                    text: "Freeze "
                    checked: Render.getConfig("RenderMainView.JitterCam")["freeze"]
                    onCheckedChanged: { Render.getConfig("RenderMainView.JitterCam")["freeze"] = checked }
            }
           
        }
    }
}
