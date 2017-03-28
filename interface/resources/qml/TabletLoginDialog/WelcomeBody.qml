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

    property bool welcomeBack: false

    function setTitle() {
        loginDialogRoot.title = (welcomeBack ? qsTr("Welcome back <b>") : qsTr("Welcome <b>")) + Account.username + qsTr("</b>!")
        loginDialogRoot.iconText = ""
        d.resize();
    }

    QtObject {
        id: d
        function resize() {}
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

            onClicked: bodyLoader.popup()
        }
    }

    Component.onCompleted: {
        welcomeBody.setTitle()
    }

    Connections {
        target: Account
        onUsernameChanged: welcomeBody.setTitle()
    }
}
