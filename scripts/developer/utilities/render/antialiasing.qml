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
import QtQuick.Layouts 1.3

import "../lib/styles-uit"
//import "../../controls-uit" as HifiControls

    
import "configSlider"
import "../lib/plotperf"

Rectangle {
    id: root;

    HifiConstants { id: hifi; }
    color: hifi.colors.baseGray;

    Column {
        id: antialiasing
        spacing: 20

        Column{
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
            Row {
                CheckBox {
                    text: "Unjitter"
                    checked: Render.getConfig("RenderMainView.Antialiasing")["unjitter"]
                    onCheckedChanged: { Render.getConfig("RenderMainView.Antialiasing")["unjitter"] = checked }
                }
            }
            Row {
                CheckBox {
                    text: "Debug"
                    checked: Render.getConfig("RenderMainView.Antialiasing")["debug"]
                    onCheckedChanged: { Render.getConfig("RenderMainView.Antialiasing")["debug"] = checked }
                }
                CheckBox {
                    text: "Freeze "
                    checked: Render.getConfig("RenderMainView.JitterCam")["freeze"]
                    onCheckedChanged: { Render.getConfig("RenderMainView.JitterCam")["freeze"] = checked }
                }
                CheckBox {
                    text: "Show Debug Cursor"
                    checked: Render.getConfig("RenderMainView.Antialiasing")["showCursorPixel"]
                    onCheckedChanged: { Render.getConfig("RenderMainView.Antialiasing")["showCursorPixel"] = checked }
                }
            }
            ConfigSlider {
                label: qsTr("Debug X")
                integral: false
                config: Render.getConfig("RenderMainView.Antialiasing")
                property: "debugX"
                max: 1.0
                min: 0.0
            }
            Row {
                CheckBox {
                    text: "Jitter Sequence"
                    checked: Render.getConfig("RenderMainView.Antialiasing")["showJitterSequence"]
                    onCheckedChanged: { Render.getConfig("RenderMainView.Antialiasing")["showJitterSequence"] = checked }
                }
                CheckBox {
                    text: "CLosest Fragment"
                    checked: Render.getConfig("RenderMainView.Antialiasing")["showClosestFragment"]
                    onCheckedChanged: { Render.getConfig("RenderMainView.Antialiasing")["showClosestFragment"] = checked }
                }                
            }            
            ConfigSlider {
                label: qsTr("Debug Velocity Threshold [pix]")
                integral: false
                config: Render.getConfig("RenderMainView.Antialiasing")
                property: "debugShowVelocityThreshold"
                max: 50
                min: 0.0
            }
            ConfigSlider {
                label: qsTr("Debug Orb Zoom")
                integral: false
                config: Render.getConfig("RenderMainView.Antialiasing")
                property: "debugOrbZoom"
                max: 8.0
                min: 0.0
            } 
        }
    }
}
