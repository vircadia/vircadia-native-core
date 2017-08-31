//
//  GlyphButton.qml
//
//  Created by Vlad Stelmahovsky on 2017-06-21
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4 as Original
import QtQuick.Controls.Styles 1.4

import "../styles-uit"

Original.Button {
    id: control

    property int colorScheme: hifi.colorSchemes.light
    property string glyph: ""
    property int size: 32
    //colors
    readonly property color normalColor: "#AFAFAF"
    readonly property color hoverColor: "#00B4EF"
    readonly property color clickedColor: "#FFFFFF"
    readonly property color disabledColor: "#575757"

    style: ButtonStyle {
        background: Item {}


        label: HiFiGlyphs {
            color: control.enabled ? (control.pressed ? control.clickedColor :
                                                        (control.hovered ? control.hoverColor : control.normalColor)) :
                                     control.disabledColor
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            anchors {
                // Tweak horizontal alignment so that it looks right.
                left: parent.left
                leftMargin: -0.5
            }
            text: control.glyph
            size: control.size
        }
    }
}
