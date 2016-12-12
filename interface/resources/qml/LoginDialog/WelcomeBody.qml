//
//  WelcomeBody.qml
//
//  Created by Clement on 7/18/16
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4

import "../controls-uit"
import "../styles-uit"

Item {
    id: welcomeBody
    clip: true
    width: root.pane.width
    height: root.pane.height

    property bool welcomeBack: false

    function setTitle() {
        root.title = (welcomeBack ? qsTr("Welcome back <b>") : qsTr("Welcome <b>")) + Account.username + qsTr("</b>!")
        root.iconText = ""
        d.resize();
    }

    QtObject {
        id: d
        readonly property int minWidth: 480
        readonly property int maxWidth: 1280
        readonly property int minHeight: 120
        readonly property int maxHeight: 720

        function resize() {
            var targetWidth = Math.max(titleWidth, mainTextContainer.contentWidth)
            var targetHeight = mainTextContainer.height + 3 * hifi.dimensions.contentSpacing.y + buttons.height

            parent.width = root.width = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth))
            parent.height = root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight))
        }
    }

    InfoItem {
        id: mainTextContainer
        anchors {
            top: parent.top
            horizontalCenter: parent.horizontalCenter
            margins: 0
            topMargin: hifi.dimensions.contentSpacing.y
        }

        text: qsTr("You are now signed into High Fidelity")
        wrapMode: Text.WordWrap
        color: hifi.colors.baseGrayHighlight
        lineHeight: 2
        lineHeightMode: Text.ProportionalHeight
        horizontalAlignment: Text.AlignHCenter
    }

    Row {
        id: buttons
        anchors {
            top: mainTextContainer.bottom
            horizontalCenter: parent.horizontalCenter
            margins: 0
            topMargin: 2 * hifi.dimensions.contentSpacing.y
        }
        spacing: hifi.dimensions.contentSpacing.x
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Button {
            anchors.verticalCenter: parent.verticalCenter

            text: qsTr("Close");

            onClicked: root.destroy()
        }
    }

    Component.onCompleted: welcomeBody.setTitle()

    Connections {
        target: Account
        onUsernameChanged: welcomeBody.setTitle()
    }
}
