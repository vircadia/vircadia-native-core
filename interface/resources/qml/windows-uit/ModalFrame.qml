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
import "../controls"

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
        Rectangle {
            id: background
            anchors.fill: parent
            anchors.margins: -4096
            visible: window.visible
            color: "#7f7f7f7f";
            radius: 3;
        }

        Text {
            y: -implicitHeight - iconSize / 2
            text: window.title
            elide: Text.ElideRight
            font.bold: true
            color: window.focus ? "white" : "gray"
            style: Text.Outline;
            styleColor: "black"
        }
    }
}
