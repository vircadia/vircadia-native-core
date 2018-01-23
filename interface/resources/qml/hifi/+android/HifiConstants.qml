//
//  HifiAndroidConstants.qml
//  interface/resources/qml/+android
//
//  Created by Gabriel Calero & Cristian Duarte on 23 Oct 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.4

Item {

    id: android

    readonly property alias dimen: dimen
    readonly property alias color: color

    Item {
        id: dimen
        readonly property real windowLessWidth: 126
        readonly property real windowLessHeight: 64

        readonly property real windowZ: 100

        readonly property real headerHeight: 276

        readonly property real headerIconPosX: 90
        readonly property real headerIconPosY: 108
        readonly property real headerIconWidth: 111
        readonly property real headerIconHeight: 111
        readonly property real headerIconTitleDistance: 151

        readonly property real headerHideWidth: 150
        readonly property real headerHideHeight: 150
        readonly property real headerHideRightMargin: 110
        readonly property real headerHideTopMargin: 90
        readonly property real headerHideIconWidth: 70
        readonly property real headerHideIconHeight: 45
        readonly property real headerHideTextTopMargin: 36

        readonly property real botomHudWidth: 366
        readonly property real botomHudHeight: 180

    }

    Item {
        id: color
        readonly property color gradientTop: "#4E4E4E"
        readonly property color gradientBottom: "#242424"
    }
}
