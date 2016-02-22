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
    id: frame

    Item {
        anchors.fill: parent

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
