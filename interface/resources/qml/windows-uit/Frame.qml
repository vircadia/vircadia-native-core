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

import "../controls-uit"
import "../styles-uit"
import "../js/Utils.js" as Utils

Item {
    id: frame
    // Frames always fill their parents, but their decorations may extend
    // beyond the window via negative margin sizes
    anchors.fill: parent

    property alias window: frame.parent  // Convenience accessor for the window
    default property var decoration

    readonly property int iconSize: 22
    readonly property int frameMargin: 9
    readonly property int frameMarginLeft: frameMargin
    readonly property int frameMarginRight: frameMargin
    readonly property int frameMarginTop: 2 * frameMargin + iconSize
    readonly property int frameMarginBottom: iconSize + 2

    children: [
        focusShadow,
        decoration,
        sizeOutline,
        debugZ,
        sizeDrag
    ]

    Text {
        id: debugZ
        visible: DebugQML
        text: window ? "Z: " + window.z : ""
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
        width: 2 * window.width
        height: width
        x: -width / 4
        y: window.height / 2 - width / 2
        visible: window && window.focus && windowContent.visible
        gradient: Gradient {
            // GradientStop position 0.5 is at full circumference of circle that fits inside the square.
            GradientStop { position: 0.0; color: "#ff000000" }    // black, 100% opacity
            GradientStop { position: 0.333; color: "#1f000000" }  // black, 12% opacity
            GradientStop { position: 0.5; color: "#00000000" }    // black, 0% opacity
            GradientStop { position: 1.0; color: "#00000000" }
        }
    }

    Rectangle {
        id: sizeOutline
        x: -frameMarginLeft
        y: -frameMarginTop
        width: window ? window.width + frameMarginLeft + frameMarginRight : 0
        height: window ? window.height + frameMarginTop + frameMarginBottom : 0
        color: hifi.colors.baseGrayHighlight15
        border.width: 3
        border.color: hifi.colors.white50
        radius: hifi.dimensions.borderRadius
        visible: window ? !windowContent.visible : false
    }

    MouseArea {
        // Resize handle
        id: sizeDrag
        width: iconSize
        height: iconSize
        enabled: window ? window.resizable : false
        hoverEnabled: true
        x: window ? window.width + frameMarginRight - iconSize : 0
        y: window ? window.height : 0
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
                windowContent.visible = true
                frameContent.visible = true
                hid = false;
            }
        }
        onPositionChanged: {
            if (pressed) {
                if (windowContent.visible) {
                    windowContent.visible = false;
                    frameContent.visible = false
                    hid = true;
                }
                var delta = Qt.vector2d(mouseX, mouseY).minus(pressOrigin);
                frame.deltaSize(delta.x, delta.y)
            }
        }
        HiFiGlyphs {
            visible: sizeDrag.enabled
            x: -5  // Move a little to visually align
            y: -3  // ""
            text: "A"
            size: iconSize + 4
            color: sizeDrag.containsMouse || !(window && window.focus) ? hifi.colors.white : hifi.colors.lightGray
        }
    }

}
