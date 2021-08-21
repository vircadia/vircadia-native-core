//
//  Frame.qml
//
//  Created by Bradley Austin Davis on 12 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtGraphicalEffects 1.0

import stylesUit 1.0
import "../js/Utils.js" as Utils

Item {
    id: frame
    objectName: "Frame"
    HifiConstants { id: hifi }

    default property var decoration
    property string qmlFile: "N/A"
    property bool gradientsSupported: desktop.gradientsSupported

    readonly property int frameMarginLeft: frame.decoration ? frame.decoration.frameMarginLeft : 0
    readonly property int frameMarginRight: frame.decoration ? frame.decoration.frameMarginRight : 0
    readonly property int frameMarginTop: frame.decoration ? frame.decoration.frameMarginTop : 0
    readonly property int frameMarginBottom: frame.decoration ? frame.decoration.frameMarginBottom : 0

    // Frames always fill their parents, but their decorations may extend
    // beyond the window via negative margin sizes
    anchors.fill: parent

    children: [
        focusShadow,
        decoration,
        sizeOutline,
        debugZ,
        sizeDrag
    ]

    Text {
        id: debugZ
        visible: (typeof DebugQML !== 'undefined') ? DebugQML : false
        color: "red"
        text: (window ? "Z: " + window.z : "") + " " + qmlFile
        y: window ? window.height + 4 : 0
    }

    function deltaSize(dx, dy) {
        var newSize = Qt.vector2d(window.width + dx, window.height + dy);
        newSize = Utils.clampVector(newSize, window.minSize, window.maxSize);
        window.width = newSize.x
        window.height = newSize.y
    }

    RadialGradient {
        id: focusShadow
        width: 1.66 * window.width
        height: 1.66 * window.height
        x: (window.width - width) / 2
        y: window.height / 2 - 0.375 * height
        visible: gradientsSupported && window && window.focus && window.content.visible
        gradient: Gradient {
            // GradientStop position 0.5 is at full circumference of circle that fits inside the square.
            GradientStop { position: 0.0; color: "#ff000000" }    // black, 100% opacity
            GradientStop { position: 0.333; color: "#1f000000" }  // black, 12% opacity
            GradientStop { position: 0.5; color: "#00000000" }    // black, 0% opacity
            GradientStop { position: 1.0; color: "#00000000" }
        }
        cached: true
    }

    Rectangle {
        id: sizeOutline
        x: -frameMarginLeft
        y: -frameMarginTop
        width: window ? window.width + frameMarginLeft + frameMarginRight + 2 : 0
        height: window ? window.height + frameMarginTop + frameMarginBottom + 2 : 0
        color: hifi.colors.baseGrayHighlight15
        border.width: 3
        border.color: hifi.colors.white50
        radius: hifi.dimensions.borderRadius
        visible: window ? !window.content.visible : false
    }

    MouseArea {
        // Resize handle
        id: sizeDrag
        width: hifi.dimensions.frameIconSize
        height: hifi.dimensions.frameIconSize
        enabled: window ? window.resizable : false
        hoverEnabled: true
        x: window ? window.width + frameMarginRight - hifi.dimensions.frameIconSize : 0
        y: window ? window.height + 4 : 0
        property vector2d pressOrigin
        property vector2d sizeOrigin
        property bool hid: false
        onPressed: {
            //console.log("Pressed on size")
            pressOrigin = Qt.vector2d(mouseX, mouseY)
            sizeOrigin = Qt.vector2d(window.content.width, window.content.height)
            hid = false;
        }
        onReleased: {
            if (hid) {
                window.content.visible = true
                hid = false;
            }
        }
        onPositionChanged: {
            if (pressed) {
                if (window.content.visible) {
                    window.content.visible = false;
                    hid = true;
                }
                var delta = Qt.vector2d(mouseX, mouseY).minus(pressOrigin);
                frame.deltaSize(delta.x, delta.y)
            }
        }
        HiFiGlyphs {
            visible: sizeDrag.enabled
            x: -11  // Move a little to visually align
            y: window.modality == Qt.ApplicationModal ? -6 : -4
            text: hifi.glyphs.resizeHandle
            size: hifi.dimensions.frameIconSize + 10
            color: sizeDrag.containsMouse || sizeDrag.pressed
                   ? hifi.colors.white
                   : (window.colorScheme == hifi.colorSchemes.dark ? hifi.colors.white50 : hifi.colors.lightGrayText80)
        }
    }
}
