import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0

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


