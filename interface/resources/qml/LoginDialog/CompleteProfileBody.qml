//
//  CompleteProfileBody.qml
//
//  Created by Clement on 7/18/16
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import QtQuick.Controls.Styles 1.4 as OriginalStyles

import "../controls-uit"
import "../styles-uit"

Item {
    id: completeProfileBody
    clip: true
    width: pane.width
    height: pane.height

    QtObject {
        id: d
        readonly property int minWidth: 480
        readonly property int maxWidth: 1280
        readonly property int minHeight: 120
        readonly property int maxHeight: 720

        function resize() {
            var targetWidth = Math.max(titleWidth, additionalTextContainer.contentWidth)
            var targetHeight = 4 * hifi.dimensions.contentSpacing.y + buttons.height + additionalTextContainer.height

            root.width = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth))
            root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight))
        }
    }

    Row {
        id: buttons
        anchors {
            top: parent.top
            horizontalCenter: parent.horizontalCenter
            margins: 0
            topMargin: 3 * hifi.dimensions.contentSpacing.y
        }
        spacing: hifi.dimensions.contentSpacing.x
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Button {
            anchors.verticalCenter: parent.verticalCenter
            width: 200

            text: qsTr("Create your profile")
            color: hifi.buttons.blue

            onClicked: loginDialog.createAccountFromStream()
        }

        Button {
            anchors.verticalCenter: parent.verticalCenter

            text: qsTr("Cancel")


            onClicked: root.destroy()
        }
    }

    ShortcutText {
        id: additionalTextContainer
        anchors {
            top: buttons.bottom
            horizontalCenter: parent.horizontalCenter
            margins: 0
            topMargin: hifi.dimensions.contentSpacing.y
        }

        text: "Already have a High Fidelity profile? Link to an existing profile here."

        font.underline: true
        wrapMode: Text.WordWrap
        color: hifi.colors.blueAccent
        lineHeight: 2
        lineHeightMode: Text.ProportionalHeight
        horizontalAlignment: Text.AlignHCenter

        MouseArea {
            anchors.fill: parent
            onClicked: {
                bodyLoader.source = "LinkAccountBody.qml"
                bodyLoader.item.width = root.pane.width
                bodyLoader.item.height = root.pane.height
            }
        }
    }

    Component.onCompleted: {
        root.title = qsTr("Complete Your Profile")
        root.iconText = "<"
        d.resize();
    }

    Connections {
        target: loginDialog
        onHandleCreateCompleted: {
            console.log("Create Succeeded")

            loginDialog.loginThroughSteam()
        }
        onHandleCreateFailed: {
            console.log("Create Failed: " + error)

            bodyLoader.source = "UsernameCollisionBody.qml"
            bodyLoader.item.width = root.pane.width
            bodyLoader.item.height = root.pane.height
        }
        onHandleLoginCompleted: {
            console.log("Login Succeeded")

            bodyLoader.setSource("WelcomeBody.qml", { "welcomeBack" : false })
            bodyLoader.item.width = root.pane.width
            bodyLoader.item.height = root.pane.height
        }
        onHandleLoginFailed: {
            console.log("Login Failed")
        }
    }
}
