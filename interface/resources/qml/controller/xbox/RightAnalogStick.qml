import QtQuick 2.1

import ".."

Item {
    id: root
    property int size: 64
    width: size
    height: size
    property var device

    AnalogStick {
        size: size
        controlIds: [ device.RX, device.RY ]
    }
}


