//
//  GlyphButton.qml
//
//  Created by Clement on  3/7/16
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 2.2 as Original
import TabletScriptingInterface 1.0

import "../stylesUit"

Original.Button {
    id: control
    property int color: 0
    property int colorScheme: hifi.colorSchemes.light
    property string glyph: ""
    property int size: 32

    width: 120
    height: 28

    onHoveredChanged: {
        if (hovered) {
            Tablet.playSound(TabletEnums.ButtonHover);
        }
    }

    onFocusChanged: {
        if (focus) {
            Tablet.playSound(TabletEnums.ButtonHover);
        }
    }

    onClicked: {
        Tablet.playSound(TabletEnums.ButtonClick);
    }

    background: Rectangle {
        radius: hifi.buttons.radius

        gradient: Gradient {
            GradientStop {
                position: 0.2
                color: {
                    if (!control.enabled) {
                        hifi.buttons.disabledColorStart[control.colorScheme]
                    } else if (control.pressed) {
                        hifi.buttons.pressedColor[control.color]
                    } else if (control.hovered) {
                        hifi.buttons.hoveredColor[control.color]
                        } else if (!control.hovered && control.focus) {
                            hifi.buttons.focusedColor[control.color]
                    } else {
                        hifi.buttons.colorStart[control.color]
                    }
                }
            }
            GradientStop {
                position: 1.0
                color: {
                    if (!control.enabled) {
                        hifi.buttons.disabledColorFinish[control.colorScheme]
                    } else if (control.pressed) {
                        hifi.buttons.pressedColor[control.color]
                    } else if (control.hovered) {
                        hifi.buttons.hoveredColor[control.color]
                        } else if (!control.hovered && control.focus) {
                            hifi.buttons.focusedColor[control.color]
                    } else {
                        hifi.buttons.colorFinish[control.color]
                    }
                }
            }
        }
    }

    contentItem: HiFiGlyphs {
        color: enabled ? hifi.buttons.textColor[control.color]
                       : hifi.buttons.disabledTextColor[control.colorScheme]
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        text: control.glyph
        size: control.size
    }
}

