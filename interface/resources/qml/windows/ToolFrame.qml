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
    property bool horizontalSpacers: false
    property bool verticalSpacers: false

    Rectangle {
        // Dialog frame
        id: frameContent
        readonly property int frameMargin: 6
        readonly property int frameMarginLeft: frameMargin + (horizontalSpacers ? 12 : 0)
        readonly property int frameMarginRight: frameMargin + (horizontalSpacers ? 12 : 0)
        readonly property int frameMarginTop: frameMargin + (verticalSpacers ? 12 : 0)
        readonly property int frameMarginBottom: frameMargin + (verticalSpacers ? 12 : 0)

        Rectangle {
            visible: horizontalSpacers
            anchors.left: parent.left
            anchors.leftMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            width: 8
            height: window.height
            color: "gray";
            radius: 4
        }

        Rectangle {
            visible: horizontalSpacers
            anchors.right: parent.right
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            width: 8
            height: window.height
            color: "gray";
            radius: 4
        }

        Rectangle {
            visible: verticalSpacers
            anchors.top: parent.top
            anchors.topMargin: 6
            anchors.horizontalCenter: parent.horizontalCenter
            height: 8
            width: window.width
            color: "gray";
            radius: 4
        }

        Rectangle {
            visible: verticalSpacers
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 6
            anchors.horizontalCenter: parent.horizontalCenter
            height: 8
            width: window.width
            color: "gray";
            radius: 4
        }

        anchors {
            leftMargin: -frameMarginLeft
            rightMargin: -frameMarginRight
            topMargin: -frameMarginTop
            bottomMargin: -frameMarginBottom
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

