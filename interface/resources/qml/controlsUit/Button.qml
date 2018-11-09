//
//  Button.qml
//
//  Created by David Rowe on 16 Feb 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 2.3 as Original
import TabletScriptingInterface 1.0

import "../stylesUit"

Original.Button {
    id: control;

    property int color: 0
    property int colorScheme: hifi.colorSchemes.light
    property int fontSize: hifi.fontSizes.buttonLabel
    property int radius: hifi.buttons.radius
    property alias implicitTextWidth: buttonText.implicitWidth
    property string buttonGlyph: "";
    property int fontCapitalization: Font.AllUppercase

    width: hifi.dimensions.buttonWidth
    height: hifi.dimensions.controlLineHeight

    HifiConstants { id: hifi }

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
        radius: control.radius

        border.width: (control.color === hifi.buttons.none ||
                       (control.color === hifi.buttons.noneBorderless && control.hovered) ||
                       (control.color === hifi.buttons.noneBorderlessWhite && control.hovered) ||
                       (control.color === hifi.buttons.noneBorderlessGray && control.hovered)) ? 1 : 0;
        border.color: control.color === hifi.buttons.noneBorderless ? hifi.colors.blueHighlight :
                                                                      (control.color === hifi.buttons.noneBorderlessGray ? hifi.colors.baseGray : hifi.colors.white);

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
                    } else {
                        hifi.buttons.colorFinish[control.color]
                    }
                }
            }
        }
    }

    contentItem: Item {
        HiFiGlyphs {
            id: buttonGlyph;
            visible: control.buttonGlyph !== "";
            text: control.buttonGlyph === "" ? hifi.glyphs.question : control.buttonGlyph;
            // Size
            size: 34;
            // Anchors
            anchors.right: buttonText.left;
            anchors.top: parent.top;
            anchors.bottom: parent.bottom;
            // Style
            color: enabled ? hifi.buttons.textColor[control.color]
                           : hifi.buttons.disabledTextColor[control.colorScheme];
            // Alignment
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
        }
        RalewayBold {
            id: buttonText;
            anchors.centerIn: parent;
            font.capitalization: control.fontCapitalization
            color: enabled ? hifi.buttons.textColor[control.color]
                           : hifi.buttons.disabledTextColor[control.colorScheme]
            size: control.fontSize
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            text: control.text
        }
    }
}

