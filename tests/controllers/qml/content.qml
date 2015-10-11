import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0

import "./xbox"
import "./controls"

Column {
    id: root
    property var xbox: NewControllers.Hardware.X360Controller1
    property var actions: NewControllers.Actions
    property var standard: NewControllers.Standard
    property string mappingName: "TestMapping"

    spacing: 12

    Timer {
        interval: 50; running: true; repeat: true
        onTriggered: {
            NewControllers.update();
        }
    }

    Row {
        spacing: 8
        Button {
            text: "Default Mapping"
            onClicked: {
                var mapping = NewControllers.newMapping("Default");
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
                NewControllers.enableMapping("Default");
            }
        }

        Button {
            text: "Build Mapping"
            onClicked: {
                var mapping = NewControllers.newMapping(root.mappingName);
                // Inverting a value
                mapping.from(xbox.RY).invert().to(standard.RY);
                // Assigning a value from a function
                mapping.from(function() { return Math.sin(Date.now() / 250); }).to(standard.RX);
                // Constrainting a value to -1, 0, or 1, with a deadzone
                mapping.from(xbox.LY).deadZone(0.5).constrainToInteger().to(standard.LY);
                mapping.join(standard.LB, standard.RB).pulse(0.5).to(actions.Yaw);
            }
        }

        Button {
            text: "Enable Mapping"
            onClicked: NewControllers.enableMapping(root.mappingName)
        }

        Button {
            text: "Disable Mapping"
            onClicked: NewControllers.disableMapping(root.mappingName)
        }
    }

    Row {
        spacing: 8
        Xbox { device: root.xbox }
        Xbox { device: root.standard }
    }


    Row {
        ScrollingGraph {
            controlId: NewControllers.Actions.Yaw
            label: "Yaw"
            min: -3.0
            max: 3.0
            size: 128
        }
    }
}

