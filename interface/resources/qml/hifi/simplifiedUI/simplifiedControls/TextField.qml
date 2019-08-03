//
//  TextField.qml
//
//  Created by Zach Fox on 2019-05-06
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.3
import TabletScriptingInterface 1.0
import "../simplifiedConstants" as SimplifiedConstants
import stylesUit 1.0 as HifiStylesUit

TextField {
    id: root

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    color: simplifiedUI.colors.text.white
    font.family: "Graphik Medium"
    font.pixelSize: 22
    selectionColor: simplifiedUI.colors.text.white
    selectedTextColor: simplifiedUI.colors.text.darkGrey
    verticalAlignment: TextInput.AlignVCenter
    horizontalAlignment: TextInput.AlignLeft
    autoScroll: false
    hoverEnabled: true
    leftPadding: 0
    rightPadding: editPencil.implicitWidth + simplifiedUI.sizes.controls.textField.editPencilPadding

    onFocusChanged: {
        if (focus) {
            Tablet.playSound(TabletEnums.ButtonClick);
        }
    }

    onHoveredChanged: {
        if (hovered) {
            Tablet.playSound(TabletEnums.ButtonHover);
        }
    }

    background: Item {
        anchors.fill: parent

        Rectangle {
            id: bottomRectangle
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: root.focus ? simplifiedUI.colors.controls.textField.focus :
                (root.hovered ? simplifiedUI.colors.controls.textField.hover : simplifiedUI.colors.controls.textField.normal)
        }

        HifiStylesUit.HiFiGlyphs {
            id: editPencil
            text: simplifiedUI.glyphs.pencil
            // Text Size
            size: root.font.pixelSize * 1.5
            // Anchors
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            // Style
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: bottomRectangle.color
        }
    }
}
