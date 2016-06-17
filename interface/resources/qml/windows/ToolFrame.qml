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

Frame {
    HifiConstants { id: hifi }

    Rectangle {
        // Dialog frame
        id: frameContent
        readonly property int frameMargin: 6
        readonly property int frameMarginLeft: frameMargin
        readonly property int frameMarginRight: frameMargin
        readonly property int frameMarginTop: frameMargin
        readonly property int frameMarginBottom: frameMargin
        anchors {
            topMargin: -frameMargin
            leftMargin: -frameMargin
            rightMargin: -frameMargin
            bottomMargin: -frameMargin
        }
        anchors.fill: parent
        color: hifi.colors.baseGrayHighlight40
        border {
            width: hifi.dimensions.borderWidth
            color: hifi.colors.faintGray50
        }
        radius: hifi.dimensions.borderRadius / 2

        // Enable dragging of the window
        MouseArea {
            anchors.fill: parent
            drag.target: window
        }
    }
}

