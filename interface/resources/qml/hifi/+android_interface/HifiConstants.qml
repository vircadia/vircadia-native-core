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
import QtQuick.Window 2.2

Item {

    id: android

    readonly property alias dimen: dimen
    readonly property alias color: color

    Item {
        id: dimen
        readonly property bool atLeast1440p: Screen.width >= 2560 && Screen.height >= 1440
        readonly property real windowLessWidth: atLeast1440p ? 378 : 284
        readonly property real windowLessHeight: atLeast1440p ? 192 : 144

        readonly property real windowZ: 100

        readonly property real headerHeight: atLeast1440p ? 276 : 207

        readonly property real headerIconPosX: atLeast1440p ? 90 : 67
        readonly property real headerIconPosY: atLeast1440p ? 108 : 81
        readonly property real headerIconWidth: atLeast1440p ? 111 : 83
        readonly property real headerIconHeight: atLeast1440p ? 111 : 83
        readonly property real headerIconTitleDistance: atLeast1440p ? 151 : 113

        readonly property real headerHideWidth: atLeast1440p ? 150 : 112
        readonly property real headerHideHeight: atLeast1440p ? 150 : 112
        readonly property real headerHideRightMargin: atLeast1440p ? 110 : 82
        readonly property real headerHideTopMargin: atLeast1440p ? 90 : 67
        readonly property real headerHideIconWidth: atLeast1440p ? 70 : 52
        readonly property real headerHideIconHeight: atLeast1440p ? 45 : 33
        readonly property real headerHideTextTopMargin: atLeast1440p ? 36 : 27

        readonly property real botomHudWidth: 366
        readonly property real botomHudHeight: 180

    }

    Item {
        id: color
        readonly property color gradientTop: "#4E4E4E"
        readonly property color gradientBottom: "#242424"
    }
}
