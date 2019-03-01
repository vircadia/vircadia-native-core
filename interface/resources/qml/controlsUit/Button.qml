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
    property string fontFamily: "Raleway"
    property int fontSize: hifi.fontSizes.buttonLabel
    property bool fontBold: true
    property int radius: hifi.buttons.radius
    property alias implicitTextWidth: buttonText.implicitWidth
    property string buttonGlyph: "";
    property int buttonGlyphSize: 34;
    property int buttonGlyphRightMargin: 0;
    property int fontCapitalization: Font.AllUppercase

    width: hifi.dimensions.buttonWidth
    height: hifi.dimensions.controlLineHeight

    property size implicitPadding: Qt.size(20, 16)
    property int implicitWidth: buttonContentItem.implicitWidth + implicitPadding.width
    property int implicitHeight: buttonContentItem.implicitHeight + implicitPadding.height

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
        id: buttonContentItem
        implicitWidth: (buttonGlyph.visible ? buttonGlyph.implicitWidth : 0) + buttonText.implicitWidth
        implicitHeight: buttonText.implicitHeight
        TextMetrics {
            id: buttonGlyphTextMetrics;
            font: buttonGlyph.font;
            text: buttonGlyph.text;
        }
        HiFiGlyphs {
            id: buttonGlyph;
            visible: control.buttonGlyph !== "";
            text: control.buttonGlyph === "" ? hifi.glyphs.question : control.buttonGlyph;
            // Size
            size: control.buttonGlyphSize;
            // Anchors
            anchors.right: buttonText.left;
            anchors.rightMargin: control.buttonGlyphRightMargin
            anchors.top: parent.top;
            anchors.bottom: parent.bottom;
            // Style
            color: enabled ? hifi.buttons.textColor[control.color]
                           : hifi.buttons.disabledTextColor[control.colorScheme];
            // Alignment
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
        }

        TextMetrics {
            id: buttonTextMetrics;
            font: buttonText.font;
            text: buttonText.text;
        }
        Text {
            id: buttonText;
            width: buttonTextMetrics.width
            anchors.verticalCenter: parent.verticalCenter;
            font.capitalization: control.fontCapitalization
            color: enabled ? hifi.buttons.textColor[control.color]
                           : hifi.buttons.disabledTextColor[control.colorScheme]
            font.family: control.fontFamily
            font.pixelSize: control.fontSize
            font.bold: control.fontBold
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            text: control.text
            Component.onCompleted: {
                if (control.buttonGlyph !== "") {
                    buttonText.x = buttonContentItem.width/2 - buttonTextMetrics.width/2 + (buttonGlyphTextMetrics.width + control.buttonGlyphRightMargin)/2;
                } else {
                    buttonText.anchors.centerIn = parent;
                }
            }
        }
    }
}
