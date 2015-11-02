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

    Xbox {
        width: root.width; height: root.height
        device: root.device
    }

    // Left primary
    ToggleButton {
        x: 0; y: parent.height - height; 
        controlId: root.device.LeftPrimaryThumb
        width: 16 * root.scale; height: 16 * root.scale
    }

    // Left primary
    ToggleButton {
        x: parent.width - width; y: parent.height - height; 
        controlId: root.device.RightPrimaryThumb
        width: 16 * root.scale; height: 16 * root.scale
    }
}
