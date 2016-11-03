import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0

import "xbox"

Item {
    id: root
    property real aspect: 300.0 / 215.0
    width: 300
    height: width / aspect
    property var device
    property string label: ""
    property real scale: width / 300.0 
    
    Image {
        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            text: root.label
            visible: root.label != ""
        }
        anchors.fill: parent
        source: "xbox/xbox360-controller-md.png"

        LeftAnalogStick {
            device: root.device
            x: (65 * root.scale) - width / 2; y: (42 * root.scale) - height / 2
        }

        // Left stick press
        ToggleButton {
            controlId: root.device.LS
            width: 16 * root.scale; height: 16 * root.scale
            x: (65 * root.scale) - width / 2; y: (42 * root.scale) - height / 2
        }


        RightAnalogStick {
            device: root.device
            x: (193 * root.scale) - width / 2; y: (96 * root.scale) - height / 2
        }

        // Right stick press
        ToggleButton {
            controlId: root.device.RS
            width: 16 * root.scale; height: 16 * root.scale
            x: (193 * root.scale) - width / 2; y: (96 * root.scale) - height / 2
        }

        // Left trigger
        AnalogButton {
            controlId: root.device.LT
            width: 8; height: 64
            x: (20 * root.scale); y: (7 * root.scale)
        }

        // Right trigger
        AnalogButton {
            controlId: root.device.RT
            width: 8; height: 64
            x: (272 * root.scale); y: (7 * root.scale)
        }

        // Left bumper
        ToggleButton {
            controlId: root.device.LB
            width: 32 * root.scale; height: 16 * root.scale
            x: (40 * root.scale); y: (7 * root.scale)
        }

        // Right bumper
        ToggleButton {
            controlId: root.device.RB
            width: 32 * root.scale; height: 16 * root.scale
            x: (root.width - width) - (40 * root.scale); y: (7 * root.scale)
        }

        DPad {
            device: root.device
            size: 48 * root.scale
            x: (80 * root.scale); y: (71 * root.scale)
        }

        XboxButtons {
            device: root.device
            size: 65 * root.scale
            x: (206 * root.scale); y: (19 * root.scale)
        }

        ToggleButton {
            controlId: root.device.Back
            width: 16 * root.scale; height: 12 * root.scale
            x: (112 * root.scale); y: (45 * root.scale)
        }

        ToggleButton {
            controlId: root.device.Start
            width: 16 * root.scale; height: 12 * root.scale
            x: (177 * root.scale); y: (45 * root.scale)
        }
    }
}
