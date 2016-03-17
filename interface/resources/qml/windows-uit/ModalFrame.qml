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
        id: modalFrame

        readonly property bool hasTitle: window.title != ""

        anchors {
            fill: parent
            topMargin: -hifi.dimensions.modalDialogMargin.y - (modalFrame.hasTitle ? hifi.dimensions.modalDialogTitleHeight + 10 : 0)
            leftMargin: -hifi.dimensions.modalDialogMargin.x
            rightMargin: -hifi.dimensions.modalDialogMargin.x
            bottomMargin: -hifi.dimensions.modalDialogMargin.y
        }

        border {
            width: hifi.dimensions.borderWidth
            color: hifi.colors.lightGrayText80
        }
        radius: hifi.dimensions.borderRadius
        color: hifi.colors.faintGray

        Item {
            visible: modalFrame.hasTitle
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
    }
}
