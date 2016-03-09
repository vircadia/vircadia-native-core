//
//  Button.qml
//
//  Created by David Rowe on 16 Feb 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4 as Original
import QtQuick.Controls.Styles 1.4

import "../styles-uit"

Original.Button {
    id: button
    property int color: 0
    width: 120
    height: 28

    style: ButtonStyle {

        background: Rectangle {
            radius: hifi.buttons.radius
            gradient: Gradient {
                GradientStop {
                    position: 0.2
                    color: enabled
                           ? (!pressed && button.color != hifi.buttons.black || (!hovered || pressed) && button.color == hifi.buttons.black
                              ? hifi.buttons.colorStart[button.color] : hifi.buttons.colorFinish[button.color])
                           : hifi.buttons.colorStart[hifi.buttons.white]
                }
                GradientStop {
                    position: 1.0
                    color: enabled
                           ? ((!hovered || pressed) && button.color != hifi.buttons.black || !pressed && button.color == hifi.buttons.black
                              ? hifi.buttons.colorFinish[button.color] : hifi.buttons.colorStart[button.color])
                           : hifi.buttons.colorFinish[hifi.buttons.white]
                }
            }
        }

        label: RalewayBold {
            font.capitalization: Font.AllUppercase
            color: enabled ? hifi.buttons.textColor[button.color] : hifi.colors.lightGrayText
            size: hifi.fontSizes.buttonLabel
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            text: control.text
        }
    }
}
