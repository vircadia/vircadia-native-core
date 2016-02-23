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
    Item {
        id: modalFrame

        anchors.fill: parent
        anchors.margins: 0

        readonly property bool hasTitle: window.title != ""

        Rectangle {
            anchors {
                topMargin: -hifi.dimensions.modalDialogMargin - (modalFrame.hasTitle ? hifi.dimensions.modalDialogTitleHeight : 0)
                leftMargin: -hifi.dimensions.modalDialogMargin
                rightMargin: -hifi.dimensions.modalDialogMargin
                bottomMargin: -hifi.dimensions.modalDialogMargin
                fill: parent
            }
            border {
                width: hifi.dimensions.borderWidth
                color: hifi.colors.lightGrayText80
            }
            radius: hifi.dimensions.borderRadius
            color: hifi.colors.faintGray
        }

        Item {
            visible: modalFrame.hasTitle
            width: title.width + (window.iconText !== "" ? icon.width + hifi.dimensions.contentSpacing.x : 0)
            x: (parent.width - width) / 2

            FontAwesome {
                id: icon
                text: window.iconText
                size: 30
                color: hifi.colors.lightGrayText
                visible: text != ""
                y: -hifi.dimensions.modalDialogTitleHeight
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
            visible: modalFrame.hasTitle
        }
    }
}
