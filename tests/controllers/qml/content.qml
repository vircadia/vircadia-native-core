import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0

import "./xbox"
import "./controls"

Column {
    id: root
    property var actions: Controllers.Actions
    property var standard: Controllers.Standard
    property var testMapping: null
    property var xbox: null


    Component.onCompleted: {
        var patt = /^X360Controller/;
        for (var prop in Controllers.Hardware) {
            if(patt.test(prop)) {
                root.xbox = Controllers.Hardware[prop]
                break
            }
        }
    }

    spacing: 12

    Timer {
        interval: 50; running: true; repeat: true
        onTriggered: {
            Controllers.update();
        }
    }

    Row {
        spacing: 8
        Button {
            text: "Default Mapping"
            onClicked: {
                var mapping = Controllers.newMapping("Default");
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
                Controllers.enableMapping("Default");
                enabled = false;
                text = "Built"
            }
        }

        Button {
            text: "Build Mapping"
            onClicked: {
                var mapping = Controllers.newMapping();
                // Inverting a value
                mapping.from(xbox.RY).invert().to(standard.RY);
                // Assigning a value from a function
                mapping.from(function() { return Math.sin(Date.now() / 250); }).to(standard.RX);
                // Constrainting a value to -1, 0, or 1, with a deadzone
                mapping.from(xbox.LY).deadZone(0.5).constrainToInteger().to(standard.LY);
                // change join to makeAxis
                mapping.join(standard.LB, standard.RB).to(actions.Yaw);
                mapping.from(actions.Yaw).clamp(0, 1).invert().to(actions.YAW_RIGHT);
                mapping.from(actions.Yaw).clamp(-1, 0).to(actions.YAW_LEFT);

                // mapping.modifier(keyboard.Ctrl).scale(2.0)

//                mapping.from(keyboard.A).to(actions.TranslateLeft)
//                mapping.from(keyboard.A, keyboard.Shift).to(actions.TurnLeft)
//                mapping.from(keyboard.A, keyboard.Shift, keyboard.Ctrl).scale(2.0).to(actions.TurnLeft)

//                // First loopbacks
//                // Then non-loopbacks by constraint level (number of inputs)
//                mapping.from(xbox.RX).deadZone(0.2).to(xbox.RX)

//                mapping.from(standard.RB, standard.LB, keyboard.Shift).to(actions.TurnLeft)


//                mapping.from(keyboard.A, keyboard.Shift).to(actions.TurnLeft)


//                mapping.from(keyboard.W).when(keyboard.Shift).to(actions.Forward)
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
    }

    Row {
        spacing: 8
        Xbox { device: root.xbox }
        Xbox { device: root.standard }
    }


    Row {
        spacing: 8
        ScrollingGraph {
            controlId: Controllers.Actions.Yaw
            label: "Yaw"
            min: -3.0
            max: 3.0
            size: 128
        }

        ScrollingGraph {
            controlId: Controllers.Actions.YAW_LEFT
            label: "Yaw Left"
            min: -3.0
            max: 3.0
            size: 128
        }

        ScrollingGraph {
            controlId: Controllers.Actions.YAW_RIGHT
            label: "Yaw Right"
            min: -3.0
            max: 3.0
            size: 128
        }
    }
}

