//
//  Prop/style/PiButton.qml
//
//  Created by Sam Gateau on 17/09/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.6
import QtQuick.Controls 2.1

Button {
    Global { id: global }
    id: control
    text: ""
    spacing: 0
    property alias color: theContentItem.color

    contentItem: PiText {
        id: theContentItem
        text: control.text
        horizontalAlignment: Text.AlignHCenter
        color: global.fontColor
    }

    background: Rectangle {
        color: control.down ? global.colorBackHighlight : global.colorBackShadow 
        opacity: enabled ? 1 : 0.3
        border.color: control.down ? global.colorBorderHighight : (control.hovered ? global.colorBorderHighight : global.colorBorderLight)
        border.width: global.valueBorderWidth
        radius: global.valueBorderRadius
    }
}