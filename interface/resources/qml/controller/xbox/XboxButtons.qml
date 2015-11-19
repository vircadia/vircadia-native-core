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
    property int spacer: size / 3
    property var device
    property color color: 'black'

    ToggleButton {
        controlId: device.Y
        x: spacer
        width: spacer; height: spacer
        color: 'yellow'
    }

    ToggleButton {
        controlId: device.X
        y: spacer
        width: spacer; height: spacer
        color: 'blue'
    }

    ToggleButton {
        controlId: device.B
        x: spacer * 2; y: spacer
        width: spacer; height: spacer
        color: 'red'
    }

    ToggleButton {
        controlId: device.A
        x: spacer; y: spacer * 2
        width: spacer; height: spacer
        color: 'green'
    }
}


