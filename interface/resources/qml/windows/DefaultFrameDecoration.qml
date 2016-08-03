//
//  DefaultFrame.qml
//
//  Created by Bradley Austin Davis on 12 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtGraphicalEffects 1.0

import "."
import "../styles-uit"

Decoration {
    HifiConstants { id: hifi }

    // Dialog frame
    id: root

    property int iconSize: hifi.dimensions.frameIconSize
    frameMargin: 9
    frameMarginLeft: frameMargin
    frameMarginRight: frameMargin
    frameMarginTop: 2 * frameMargin + iconSize
    frameMarginBottom: iconSize + 11

    onInflateDecorations: {
        if (!HMD.active) {
            return;
        }
        root.frameMargin = 18
        titleText.size = hifi.fontSizes.overlayTitle * 2
        root.iconSize = hifi.dimensions.frameIconSize * 2
    }

    onDeflateDecorations: {
        root.frameMargin = 9
        titleText.size = hifi.fontSizes.overlayTitle
        root.iconSize = hifi.dimensions.frameIconSize
    }


    Row {
        id: controlsRow
        anchors {
            right: parent.right;
            top: parent.top;
            topMargin: root.frameMargin + 1  // Move down a little to visually align with the title
            rightMargin: root.frameMarginRight;
        }
        spacing: root.iconSize / 4

        HiFiGlyphs {
            // "Pin" button
            visible: window.pinnable
            text: window.pinned ? hifi.glyphs.pinInverted : hifi.glyphs.pin
            color: pinClickArea.pressed ? hifi.colors.redHighlight : hifi.colors.white
            size: root.iconSize
            MouseArea {
                id: pinClickArea
                anchors.fill: parent
                hoverEnabled: true
                propagateComposedEvents: true
                onClicked: window.pinned = !window.pinned;
            }
        }

        HiFiGlyphs {
            // "Close" button
            visible: window ? window.closable : false
            text: closeClickArea.containsPress ? hifi.glyphs.closeInverted : hifi.glyphs.close
            color: closeClickArea.containsMouse ? hifi.colors.redHighlight : hifi.colors.white
            size: root.iconSize
            MouseArea {
                id: closeClickArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    window.shown = false;
                    window.windowClosed();
                }
            }
        }
    }

    RalewayRegular {
        // Title
        id: titleText
        anchors {
            left: parent.left
            leftMargin: root.frameMarginLeft + hifi.dimensions.contentMargin.x
            right: controlsRow.left
            rightMargin: root.iconSize
            top: parent.top
            topMargin: root.frameMargin
        }
        text: window ? window.title : ""
        color: hifi.colors.white
        size: hifi.fontSizes.overlayTitle
    }

    DropShadow {
        source: titleText
        anchors.fill: titleText
        horizontalOffset: 2
        verticalOffset: 2
        samples: 2
        color: hifi.colors.baseGrayShadow60
        visible: (window && window.focus)
        cached: true
    }
}

