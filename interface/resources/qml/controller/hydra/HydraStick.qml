import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0

import ".."

Item {
    id: root
    property var device
    property real scale: 1.0
    property bool leftStick: true
    width: parent.width / 2; height: parent.height
    x: leftStick ? 0 : parent.width / 2 

    Text {
        x: parent.width / 2 - width / 2; y: parent.height / 2 - height / 2
        text: root.leftStick ? "L" : "R"
        color: 'green'
    }
            
    // Analog Stick
    AnalogStick {
        size: 64 * root.scale
        x: 127 * root.scale - width / 2; y: 45 * root.scale - width / 2; z: 100
        invertY: true
        controlIds: [ 
            root.leftStick ? root.device.LX : root.device.RX, 
            root.leftStick ? root.device.LY : root.device.RY 
        ]
    }

    // Stick press
    ToggleButton {
        controlId: root.leftStick ? root.device.LS : root.device.RS 
        width: 16 * root.scale; height: 16 * root.scale
        x: 127 * root.scale - width / 2; y: 45 * root.scale - width / 2; 
        color: 'yellow'
    }

    // Trigger
    AnalogButton {
        controlId: root.leftStick ? root.device.LT : root.device.RT
        width: 8 * root.scale ; height: 64 * root.scale 
        y: 24 * root.scale
        x: root.leftStick ? (48 * root.scale) : root.width - (48 * root.scale) - width / 2   
    }

    // Bumper
    ToggleButton {
        controlId: root.leftStick ? root.device.LB : root.device.RB
        height: 16 * root.scale; width: 32 * root.scale 
        x: 128 * root.scale  - width / 2; y: 24 * root.scale
        color: 'red'
    }

    ToggleButton {
        controlId: root.leftStick ? root.device.L0 : root.device.R0
        height: 16 * root.scale; width: 4 * root.scale 
        x: 128 * root.scale - width / 2; y: 109 * root.scale
        color: 'yellow'
    }

    ToggleButton {
        controlId: root.leftStick ? root.device.L1 : root.device.R1
        width: 16 * root.scale; height: 16 * root.scale
        x: 103 * root.scale  - width / 2; y: 100 * root.scale - height / 2
        color: 'yellow'
    }

    ToggleButton {
        controlId: root.leftStick ? root.device.L2 : root.device.R2
        width: 16 * root.scale; height: 16 * root.scale
        x: 148 * root.scale - width / 2; y: 100 * root.scale - height / 2
        color: 'yellow'
    }

    ToggleButton {
        controlId: root.leftStick ? root.device.L3 : root.device.R3
        width: 16 * root.scale; height: 16 * root.scale
        x: 97 * root.scale - width / 2; y: 76 * root.scale - height / 2
        color: 'yellow'
    }

    ToggleButton {
        controlId: root.leftStick ? root.device.L4 : root.device.R4
        width: 16 * root.scale; height: 16 * root.scale
        x: 155 * root.scale - width / 2; y: 76 * root.scale - height / 2
        color: 'yellow'
    }
}       
