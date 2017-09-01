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
import "../lib/controls-uit" as HifiControls

    
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
            spacing: 10
            
            HifiControls.ConfigSlider {
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
            Separator {}
            Row {
                spacing: 10
                
                HifiControls.Button {
                    text: {
                        var state = 2 - (Render.getConfig("RenderMainView.JitterCam").freeze * 1 - Render.getConfig("RenderMainView.JitterCam").stop * 2); 
                        if (state === 2) {
                            return ">>"
                        } else if  (state === 1) {
                            return "|" + Render.getConfig("RenderMainView.JitterCam").index + "|"
                        } else {
                            return "[]"
                        }
                        }
                    onClicked: { Render.getConfig("RenderMainView.JitterCam").cycleStopPauseRun(); }
                }
                HifiControls.Button {
                    text: "<"
                    onClicked: { Render.getConfig("RenderMainView.JitterCam").prev(); }
                } 
                HifiControls.Button {
                    text: ">"
                    onClicked: { Render.getConfig("RenderMainView.JitterCam").next(); }
                }
            }
            Row {
                spacing: 10
                HifiControls.CheckBox {
                    boxSize: 20
                    text: "Unjitter"
                    checked: Render.getConfig("RenderMainView.Antialiasing")["unjitter"]
                    onCheckedChanged: { Render.getConfig("RenderMainView.Antialiasing")["unjitter"] = checked }
                }
                HifiControls.CheckBox {
                    boxSize: 20
                    text: "Show Sequence"
                    checked: Render.getConfig("RenderMainView.Antialiasing")["showJitterSequence"]
                    onCheckedChanged: { Render.getConfig("RenderMainView.Antialiasing")["showJitterSequence"] = checked }
                }
            }
            Separator {}          
            Row {
                spacing: 10
                HifiControls.CheckBox {
                    boxSize: 20
                    text: "Constrain color"
                    checked: Render.getConfig("RenderMainView.Antialiasing")["constrainColor"]
                    onCheckedChanged: { Render.getConfig("RenderMainView.Antialiasing")["constrainColor"] = checked }
                }
            }
            Row {

                spacing: 10
                HifiControls.CheckBox {
                    boxSize: 20
                    text: "Debug"
                    checked: Render.getConfig("RenderMainView.Antialiasing")["debug"]
                    onCheckedChanged: { Render.getConfig("RenderMainView.Antialiasing")["debug"] = checked }
                }
                HifiControls.CheckBox {
                    boxSize: 20
                    text: "Show Debug Cursor"
                    checked: Render.getConfig("RenderMainView.Antialiasing")["showCursorPixel"]
                    onCheckedChanged: { Render.getConfig("RenderMainView.Antialiasing")["showCursorPixel"] = checked }
                }
            }
            ConfigSlider {
                label: qsTr("Debug Region <")
                integral: false
                config: Render.getConfig("RenderMainView.Antialiasing")
                property: "debugX"
                max: 1.0
                min: 0.0
            }
            ConfigSlider {
                label: qsTr("FXAA Region >")
                integral: false
                config: Render.getConfig("RenderMainView.Antialiasing")
                property: "debugFXAAX"
                max: 1.0
                min: 0.0
            }
            Row {

                HifiControls.CheckBox {
                    boxSize: 20
                    text: "Closest Fragment"
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
                max: 32.0
                min: 1.0
            } 
        }
    }
}
