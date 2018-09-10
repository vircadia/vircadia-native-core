//
//  ModalFrame.qml
//
//  Created by Bradley Austin Davis on 15 Jan 2016
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

import "."
import "../controls-uit"
import "../styles-uit"

Frame {
    HifiConstants { id: hifi }

    Rectangle {
        id: frameContent

        readonly property bool hasTitle: window.title != ""

        readonly property int frameMarginLeft: hifi.dimensions.modalDialogMargin.x
        readonly property int frameMarginRight: hifi.dimensions.modalDialogMargin.x
        readonly property int frameMarginTop: hifi.dimensions.modalDialogMargin.y + (frameContent.hasTitle ? hifi.dimensions.modalDialogTitleHeight + 10 : 0)
        readonly property int frameMarginBottom: hifi.dimensions.modalDialogMargin.y

        signal frameClicked();

        anchors {
            fill: parent
            topMargin: -frameMarginTop
            leftMargin: -frameMarginLeft
            rightMargin: -frameMarginRight
            bottomMargin: -frameMarginBottom
        }

        border {
            width: hifi.dimensions.borderWidth
            color: hifi.colors.lightGrayText80
        }
        radius: hifi.dimensions.borderRadius
        color: hifi.colors.faintGray

        // Enable dragging of the window
        MouseArea {
            anchors.fill: parent
            drag.target: window
            enabled: window.draggable
            onClicked: window.frameClicked();
        }

        Item {
            visible: frameContent.hasTitle
            anchors.fill: parent
            anchors {
                topMargin: -parent.anchors.topMargin
                leftMargin: -parent.anchors.leftMargin
                rightMargin: -parent.anchors.rightMargin
            }

            Item {
                width: title.width + (icon.text !== "" ? icon.width + hifi.dimensions.contentSpacing.x : 0)
                x: (parent.width - width) / 2

                onWidthChanged: window.titleWidth = width

                HiFiGlyphs {
                    id: icon
                    text: window.iconText ? window.iconText : ""
                    size: window.iconSize ? window.iconSize : 30
                    color: hifi.colors.lightGray
                    visible: text != ""
                    anchors.verticalCenter: title.verticalCenter
                    anchors.left: parent.left
                }
                RalewayRegular {
                    id: title
                    text: window.title
                    elide: Text.ElideRight
                    color: hifi.colors.baseGrayHighlight
                    size: hifi.fontSizes.overlayTitle
                    y: -hifi.dimensions.modalDialogTitleHeight
                    anchors.right: parent.right
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: hifi.colors.lightGray
            }
        }


        GlyphButton {
            id: closeButton
            visible: window.closeButtonVisible
            width: 30
            y: -hifi.dimensions.modalDialogTitleHeight
            anchors {
                top: parent.top
                right: parent.right
                topMargin: 10
                rightMargin: 10
            }
            glyph: hifi.glyphs.close
            size: 23
            onClicked: {
                window.clickedCloseButton = true;
                window.destroy();
            }
        }
    }
}
