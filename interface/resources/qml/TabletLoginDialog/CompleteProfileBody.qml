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

    QtObject {
        id: d
        function resize() {}
    }

    Row {
        id: buttons
        anchors {
            top: parent.top
            horizontalCenter: parent.horizontalCenter
            margins: 0
            topMargin: 2 * hifi.dimensions.contentSpacing.y
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

            onClicked: bodyLoader.popup()
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

        text: "<a href='https://fake.link'>Already have a High Fidelity profile? Link to an existing profile here.</a>"

        wrapMode: Text.WordWrap
        lineHeight: 2
        lineHeightMode: Text.ProportionalHeight
        horizontalAlignment: Text.AlignHCenter

        onLinkActivated: {
            bodyLoader.setSource("LinkAccountBody.qml")
        }
    }

    InfoItem {
        id: termsContainer
        anchors {
            top: additionalTextContainer.bottom
            left: parent.left
            margins: 0
            topMargin: 2 * hifi.dimensions.contentSpacing.y
        }

        text: qsTr("By creating this user profile, you agree to <a href='https://highfidelity.com/terms'>High Fidelity's Terms of Service</a>")
        wrapMode: Text.WordWrap
        color: hifi.colors.baseGrayHighlight
        lineHeight: 1
        lineHeightMode: Text.ProportionalHeight
        horizontalAlignment: Text.AlignHCenter

        onLinkActivated: loginDialog.openUrl(link)
    }

    Component.onCompleted: {
        loginDialogRoot.title = qsTr("Complete Your Profile")
        loginDialogRoot.iconText = "<"
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

            bodyLoadersetSource("UsernameCollisionBody.qml")
        }
        onHandleLoginCompleted: {
            console.log("Login Succeeded")

            bodyLoader.setSource("WelcomeBody.qml", { "welcomeBack" : false })
        }
        onHandleLoginFailed: {
            console.log("Login Failed")
        }
    }
}
