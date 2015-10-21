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
    property var xbox: null


    Component.onCompleted: {
        enabled = true
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
                text: "Standard Mapping"
                onClicked: {
                    var mapping = Controller.newMapping("Default");
                    mapping.from(standard.LX).to(actions.TranslateX);
                    mapping.from(standard.LY).to(actions.TranslateZ);
                    mapping.from(standard.RY).to(actions.Pitch);
                    mapping.from(standard.RX).to(actions.Yaw);
                    mapping.from(standard.DU).scale(0.5).to(actions.LONGITUDINAL_FORWARD);
                    mapping.from(standard.DD).scale(0.5).to(actions.LONGITUDINAL_BACKWARD);
                    mapping.from(standard.DL).scale(0.5).to(actions.LATERAL_LEFT);
                    mapping.from(standard.DR).scale(0.5).to(actions.LATERAL_RIGHT);
                    mapping.from(standard.X).to(actions.VERTICAL_DOWN);
                    mapping.from(standard.Y).to(actions.VERTICAL_UP);
                    mapping.from(standard.RT).scale(0.1).to(actions.BOOM_IN);
                    mapping.from(standard.LT).scale(0.1).to(actions.BOOM_OUT);
                    mapping.from(standard.B).to(actions.ACTION1);
                    mapping.from(standard.A).to(actions.ACTION2);
                    mapping.from(standard.RB).to(actions.SHIFT);
                    mapping.from(standard.Back).to(actions.TOGGLE_MUTE);
                    mapping.from(standard.Start).to(actions.CONTEXT_MENU);
                    Controller.enableMapping("Default");
                    enabled = false;
                    text = "Standard Built"
                }
            }
            
            Button {
                text: root.xbox ? "XBox Mapping" : "XBox not found"
                property bool built: false
                enabled: root.xbox && !built
                onClicked: {
                    var mapping = Controller.newMapping();
                    mapping.from(xbox.A).to(standard.A);
                    mapping.from(xbox.B).to(standard.B);
                    mapping.from(xbox.X).to(standard.X);
                    mapping.from(xbox.Y).to(standard.Y);
                    mapping.from(xbox.Up).to(standard.DU);
                    mapping.from(xbox.Down).to(standard.DD);
                    mapping.from(xbox.Left).to(standard.DL);
                    mapping.from(xbox.Right).to(standard.Right);
                    mapping.from(xbox.LB).to(standard.LB);
                    mapping.from(xbox.RB).to(standard.RB);
                    mapping.from(xbox.LS).to(standard.LS);
                    mapping.from(xbox.RS).to(standard.RS);
                    mapping.from(xbox.Start).to(standard.Start);
                    mapping.from(xbox.Back).to(standard.Back);
                    mapping.from(xbox.LY).to(standard.LY);
                    mapping.from(xbox.LX).to(standard.LX);
                    mapping.from(xbox.RY).to(standard.RY);
                    mapping.from(xbox.RX).to(standard.RX);
                    mapping.from(xbox.LT).to(standard.LT);
                    mapping.from(xbox.RT).to(standard.RT);
                    mapping.enable();
                    built = false;
                    text = "XBox Built"
                }
            }

            Button {
                text: root.hydra ? "Hydra Mapping" : "Hydra Not Found"
                property bool built: false
                enabled: root.hydra && !built
                onClicked: {
                    var mapping = Controller.newMapping();
                    mapping.from(hydra.LY).invert().to(standard.LY);
                    mapping.from(hydra.LX).to(standard.LX);
                    mapping.from(hydra.RY).invert().to(standard.RY);
                    mapping.from(hydra.RX).to(standard.RX);
                    mapping.from(hydra.LT).to(standard.LT);
                    mapping.from(hydra.RT).to(standard.RT);
                    mapping.enable();
                    built = false;
                    text = "Hydra Built"
                }
            }

            Button {
                text: "Test Mapping"
                onClicked: {
                    var mapping = Controller.newMapping();
                    // Inverting a value
                    mapping.from(hydra.RY).invert().to(standard.RY);
                    mapping.from(hydra.RX).to(standard.RX);
                    mapping.from(hydra.LY).to(standard.LY);
                    mapping.from(hydra.LX).to(standard.LX);
                    // Assigning a value from a function
                    // mapping.from(function() { return Math.sin(Date.now() / 250); }).to(standard.RX);
                    // Constrainting a value to -1, 0, or 1, with a deadzone
//                    mapping.from(xbox.LY).deadZone(0.5).constrainToInteger().to(standard.LY);
                    mapping.makeAxis(standard.LB, standard.RB).to(actions.Yaw);
//                    mapping.from(actions.Yaw).clamp(0, 1).invert().to(actions.YAW_RIGHT);
//                    mapping.from(actions.Yaw).clamp(-1, 0).to(actions.YAW_LEFT);
                    // mapping.modifier(keyboard.Ctrl).scale(2.0)
//                    mapping.from(keyboard.A).to(actions.TranslateLeft)
//                    mapping.from(keyboard.A, keyboard.Shift).to(actions.TurnLeft)
//                    mapping.from(keyboard.A, keyboard.Shift, keyboard.Ctrl).scale(2.0).to(actions.TurnLeft)
//                    // First loopbacks
//                    // Then non-loopbacks by constraint level (number of inputs)
//                    mapping.from(xbox.RX).deadZone(0.2).to(xbox.RX)
//                    mapping.from(standard.RB, standard.LB, keyboard.Shift).to(actions.TurnLeft)
//                    mapping.from(keyboard.A, keyboard.Shift).to(actions.TurnLeft)
//                    mapping.from(keyboard.W).when(keyboard.Shift).to(actions.Forward)
                    testMapping = mapping;
                    enabled = false
                    text = "Built"
                }
            }

            Button {
                text: "Enable Mapping"
                onClicked: root.testMapping.enable()
            }

            Button {
                text: "Disable Mapping"
                onClicked: root.testMapping.disable()
            }

            Button {
                text: "Enable Mapping"
                onClicked: print(Controller.getValue(root.xbox.LY));
            }
        }

        Row {
            Xbox { device: root.standard; label: "Standard"; width: 360 }
        }
        
        Row {
            spacing: 8
            Xbox { device: root.xbox; label: "XBox"; width: 360 }
            Hydra { device: root.hydra; width: 360 }
        }
//        Row {
//            spacing: 8
//            ScrollingGraph {
//                controlId: Controller.Actions.Yaw
//                label: "Yaw"
//                min: -3.0
//                max: 3.0
//                size: 128
//            }
//
//            ScrollingGraph {
//                controlId: Controller.Actions.YAW_LEFT
//                label: "Yaw Left"
//                min: -3.0
//                max: 3.0
//                size: 128
//            }
//
//            ScrollingGraph {
//                controlId: Controller.Actions.YAW_RIGHT
//                label: "Yaw Right"
//                min: -3.0
//                max: 3.0
//                size: 128
//            }
//        }
    }
} // dialog





