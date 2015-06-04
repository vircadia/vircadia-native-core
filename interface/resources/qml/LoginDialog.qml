//
//  LoginDialog.qml
//
//  Created by David Rowe on 3 Jun 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import QtQuick.Controls 1.3  // TODO: Needed?
import "controls"
import "styles"

Dialog {
    id: root
    HifiConstants { id: hifi }

    objectName: "LoginDialog"

    property bool destroyOnInvisible: false

    implicitWidth: loginDialog.implicitWidth
    implicitHeight: loginDialog.implicitHeight

    x: parent ? parent.width / 2 - width / 2 : 0
    y: parent ? parent.height / 2 - height / 2 : 0
    property int maximumX: parent ? parent.width - width : 0
    property int maximumY: parent ? parent.height - height : 0

    function isCircular() {
        return loginDialog.dialogFormat == "circular"
    }

    LoginDialog {
        id: loginDialog

        implicitWidth: isCircular() ? circularBackground.width : rectangularBackground.width
        implicitHeight: isCircular() ? circularBackground.height : rectangularBackground.height

        Image {
            id: circularBackground
            visible: isCircular()

            source: "../images/login-circle.svg"
            width: 500
            height: 500
        }

        Image {
            id: rectangularBackground
            visible: !isCircular()

            source: "../images/login-rectangle.svg"
            width: 400
            height: 400
        }

        Text {
            id: closeText
            visible: isCircular()

            text: "Close"
            font.pixelSize: hifi.fonts.pixelSize * 0.8
            font.weight: Font.Bold
            color: "#175d74"

            anchors {
                horizontalCenter: circularBackground.horizontalCenter
                bottom: circularBackground.bottom
                bottomMargin: hifi.layout.spacing * 4
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: "PointingHandCursor"
                onClicked: {
                    root.enabled = false
                }
            }
        }

        Image {
            id: closeIcon
            visible: !isCircular()

            source: "../images/login-close.svg"

            width: 20
            height: 20
            anchors {
                top: rectangularBackground.top
                right: rectangularBackground.right
                topMargin: hifi.layout.spacing * 2
                rightMargin: hifi.layout.spacing * 2
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: "PointingHandCursor"
                onClicked: {
                    root.enabled = false
                }
            }
        }
    }

    Keys.onPressed: {
        switch (event.key) {
            case Qt.Key_Escape:
            case Qt.Key_Back:
                enabled = false;
                event.accepted = true
                break
        }
    }
}
