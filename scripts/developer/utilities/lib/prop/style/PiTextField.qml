//
//  Prop/style/PiTextField.qml
//
//  Created by Sam Gateau on 9/24/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.12
import QtQuick.Controls 2.12

TextField {
    id: control
    Global { id: global }
    implicitHeight: global.slimHeight
    implicitWidth: 200

    placeholderText: qsTr("Enter description")

    color: global.fontColor
    font.pixelSize: global.fontSize
    font.family: global.fontFamily
    font.weight: global.fontWeight

    background: Rectangle {
        color: (control.text.length > 0) ? global.colorBackHighlight : global.colorBackShadow
        border.color: (control.text.length > 0) ? global.colorBorderHighight : "transparent"
    }
}
