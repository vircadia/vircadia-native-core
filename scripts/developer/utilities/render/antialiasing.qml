//
// Antialiasing.qml
//
//  Created by Sam Gateau on 8/14/2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3

import stylesUit 1.0
import controlsUit 1.0 as HifiControls
    
import "configSlider"
import "../lib/plotperf"

Item {
Rectangle {
    id: root;

    HifiConstants { id: hifi; }
    color: hifi.colors.baseGray;

    Column {
        id: antialiasing
        spacing: 20
        padding: 10

        Column{
            spacing: 10
           
            Row {
                spacing: 10
                id: fxaaOnOff
                property bool debugFXAA: false
                HifiControls.Button {
                    function getTheText() {
                            if (Render.getConfig("RenderMainView.Antialiasing").fxaaOnOff) {
                                return "FXAA"
                            } else {
                                return "TAA"
                            }
                        }
                    text: getTheText()
                    onClicked: {
                        var onOff = !Render.getConfig("RenderMainView.Antialiasing").fxaaOnOff;
                         if (onOff) {
                            Render.getConfig("RenderMainView.JitterCam").none();
                            Render.getConfig("RenderMainView.Antialiasing").fxaaOnOff = true;
                         } else {
                            Render.getConfig("RenderMainView.JitterCam").play();
                            Render.getConfig("RenderMainView.Antialiasing").fxaaOnOff = false;
                         }
                         
                    }
                }
            }
            Separator {}          
            Row {
                spacing: 10
                
                HifiControls.Button {
                    text: {
                        var state = 2 - (Render.getConfig("RenderMainView.JitterCam").freeze * 1 - Render.getConfig("RenderMainView.JitterCam").stop * 2); 
                        if (state === 2) {
                            return "Jitter"
                        } else if  (state === 1) {
                            return "Paused at " + Render.getConfig("RenderMainView.JitterCam").index + ""
                        } else {
                            return "No Jitter"
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
            Separator {}          
            HifiControls.CheckBox {
                boxSize: 20
                text: "Constrain color"
                checked: Render.getConfig("RenderMainView.Antialiasing")["constrainColor"]
                onCheckedChanged: { Render.getConfig("RenderMainView.Antialiasing")["constrainColor"] = checked }
            }
            ConfigSlider {
                label: qsTr("Covariance gamma")
                integral: false
                config: Render.getConfig("RenderMainView.Antialiasing")
                property: "covarianceGamma"
                max: 1.5
                min: 0.5
                height: 38
            }                          
            Separator {}          
            HifiControls.CheckBox {
                boxSize: 20
                text: "Feedback history color"
                checked: Render.getConfig("RenderMainView.Antialiasing")["feedbackColor"]
                onCheckedChanged: { Render.getConfig("RenderMainView.Antialiasing")["feedbackColor"] = checked }
            }
    
            ConfigSlider {
                label: qsTr("Source blend")
                integral: false
                config: Render.getConfig("RenderMainView.Antialiasing")
                property: "blend"
                max: 1.0
                min: 0.0
                height: 38
            }
    
            ConfigSlider {
                label: qsTr("Post sharpen")
                integral: false
                config: Render.getConfig("RenderMainView.Antialiasing")
                property: "sharpen"
                max: 1.0
                min: 0.0
            }
            Separator {}                      
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
            HifiControls.CheckBox {
                boxSize: 20
                text: "Closest Fragment"
                checked: Render.getConfig("RenderMainView.Antialiasing")["showClosestFragment"]
                onCheckedChanged: { Render.getConfig("RenderMainView.Antialiasing")["showClosestFragment"] = checked }
            }           
            ConfigSlider {
                label: qsTr("Debug Velocity Threshold [pix]")
                integral: false
                config: Render.getConfig("RenderMainView.Antialiasing")
                property: "debugShowVelocityThreshold"
                max: 50
                min: 0.0
                height: 38
            }
            ConfigSlider {
                label: qsTr("Debug Orb Zoom")
                integral: false
                config: Render.getConfig("RenderMainView.Antialiasing")
                property: "debugOrbZoom"
                max: 32.0
                min: 1.0
                height: 38
            }    
        }
    }
}
}
