import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0

import "controller"
import "controls" as HifiControls
import "styles"

HifiControls.VrDialog {
    id: root
    HifiConstants { id: hifi }
    title: "Controller Test"
    resizable: true
    contentImplicitWidth: clientArea.implicitWidth
    contentImplicitHeight: clientArea.implicitHeight
    backgroundColor: "beige"

    property var actions: Controller.Actions
    property var standard: Controller.Standard
    property var hydra: null
    property var testMapping: null
    property bool testMappingEnabled: false
    property var xbox: null

    function buildMapping() {
        testMapping = Controller.newMapping();
        testMapping.fromQml(standard.RY).invert().toQml(actions.Pitch);
        testMapping.fromQml(function(){
            return Math.sin(Date.now() / 250); 
        }).toQml(actions.Yaw);
        //testMapping.makeAxis(standard.LB, standard.RB).to(actions.Yaw);
        // Step yaw takes a number of degrees
        testMapping.fromQml(standard.LB).pulse(0.10).invert().scale(40.0).toQml(actions.StepYaw);
        testMapping.fromQml(standard.RB).pulse(0.10).scale(15.0).toQml(actions.StepYaw);
        testMapping.fromQml(standard.RX).scale(15.0).toQml(actions.StepYaw);
    }

    function toggleMapping() {
        testMapping.enable(!testMappingEnabled);
        testMappingEnabled = !testMappingEnabled;
    }

    Component.onCompleted: {
        var xboxRegex = /^GamePad/;
        var hydraRegex = /^Hydra/;
        for (var prop in Controller.Hardware) {
            if(xboxRegex.test(prop)) {
                root.xbox = Controller.Hardware[prop]
                print("found xbox")
                continue
            }
            if (hydraRegex.test(prop)) {
                root.hydra = Controller.Hardware[prop]
                print("found hydra")
                continue
            }
        }
    }

    Column {
        id: clientArea
        spacing: 12
        x: root.clientX
        y: root.clientY

        Row {
            spacing: 8
            
            Button {
                text: !root.testMapping ? "Build Mapping" : (root.testMappingEnabled ? "Disable Mapping" : "Enable Mapping")
                onClicked: {

                    if (!root.testMapping) {
                        root.buildMapping()
                    } else {
                        root.toggleMapping();
                    }
                }
            }
        }

        Row {
            Standard { device: root.standard; label: "Standard"; width: 180 }
        }
        
        Row {
            spacing: 8
            Xbox { device: root.xbox; label: "XBox"; width: 180 }
            Hydra { device: root.hydra; width: 180 }
        }
        
        Row {
            spacing: 4
            ScrollingGraph {
                controlId: Controller.Actions.Yaw
                label: "Yaw"
                min: -2.0
                max: 2.0
                size: 64
            }

            ScrollingGraph {
                controlId: Controller.Actions.YawLeft
                label: "Yaw Left"
                min: -2.0
                max: 2.0
                size: 64
            }

            ScrollingGraph {
                controlId: Controller.Actions.YawRight
                label: "Yaw Right"
                min: -2.0
                max: 2.0
                size: 64
            }

            ScrollingGraph {
                controlId: Controller.Actions.StepYaw
                label: "StepYaw"
                min: -20.0
                max: 20.0
                size: 64
            }
        }

        Row {
            ScrollingGraph {
                controlId: Controller.Actions.TranslateZ
                label: "TranslateZ"
                min: -2.0
                max: 2.0
                size: 64
            }

            ScrollingGraph {
                controlId: Controller.Actions.Forward
                label: "Forward"
                min: -2.0
                max: 2.0
                size: 64
            }

            ScrollingGraph {
                controlId: Controller.Actions.Backward
                label: "Backward"
                min: -2.0
                max: 2.0
                size: 64
            }

        }
    }
} // dialog





