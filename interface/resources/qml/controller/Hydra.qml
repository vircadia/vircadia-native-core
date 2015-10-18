import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0

import "hydra"

Item {
    id: root
    width: 480
    height: width * 3.0 / 4.0
    property var device
    property real scale: width / 480
    property real rightOffset: (width / 2) * scale
    
    Image {
        anchors.fill: parent
        source: "hydra/hydra.png"

        HydraStick {
            leftStick: true
            scale: root.scale
            device: root.device
        }
        
        
        HydraStick {
            leftStick: false
            scale: root.scale
            device: root.device
        }

    }
}
