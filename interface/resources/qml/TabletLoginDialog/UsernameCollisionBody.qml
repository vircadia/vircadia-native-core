//
//  UsernameCollisionBody.qml
//
//  Created by Clement on 7/18/16
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4 as OriginalStyles

import "../controls-uit"
import "../styles-uit"

Item {
    id: usernameCollisionBody
    clip: true
    width: parent.width
    height: parent.height

    function create() {
        mainTextContainer.visible = false
        loginDialog.createAccountFromStream(textField.text)
    }


    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false

    onKeyboardRaisedChanged: d.resize();

    QtObject {
        id: d
        function resize() {}
    }

    ShortcutText {
        id: mainTextContainer
        anchors {
            top: parent.top
            left: parent.left
            margins: 0
            topMargin: hifi.dimensions.contentSpacing.y
        }

        text: qsTr("Your Steam username is not available.")
        wrapMode: Text.WordWrap
        color: hifi.colors.redAccent
        lineHeight: 1
        lineHeightMode: Text.ProportionalHeight
        horizontalAlignment: Text.AlignHCenter
    }


    TextField {
        id: textField
        anchors {
            top: mainTextContainer.bottom
            left: parent.left
            margins: 0
            topMargin: hifi.dimensions.contentSpacing.y
        }
        width: 250

        placeholderText: "Choose your own"
    }

    // Override ScrollingWindow's keyboard that would be at very bottom of dialog.
    Keyboard {
        raised: keyboardEnabled && keyboardRaised
        numeric: punctuationMode
        anchors {
            left: parent.left
            right: parent.right
            bottom: buttons.top
            bottomMargin: keyboardRaised ? 2 * hifi.dimensions.contentSpacing.y : 0
        }
    }

    Row {
        id: buttons
        anchors {
            bottom: parent.bottom
            right: parent.right
            margins: 0
            topMargin: hifi.dimensions.contentSpacing.y
        }
        spacing: hifi.dimensions.contentSpacing.x
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Button {
            anchors.verticalCenter: parent.verticalCenter
            width: 200

            text: qsTr("Create your profile")
            color: hifi.buttons.blue

            onClicked: usernameCollisionBody.create()
        }

        Button {
            anchors.verticalCenter: parent.verticalCenter

            text: qsTr("Cancel")
            onClicked: bodyLoader.popup()
        }
    }

    Component.onCompleted: {
        loginDialogRoot.title = qsTr("Complete Your Profile")
        loginDialogRoot.iconText = "<"
        keyboardEnabled = HMD.active;
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

            mainTextContainer.visible = true
            mainTextContainer.text = "\"" + textField.text + qsTr("\" is invalid or already taken.")
        }
        onHandleLoginCompleted: {
            console.log("Login Succeeded")

            bodyLoader.setSource("WelcomeBody.qml", { "welcomeBack" : false })
        }
        onHandleLoginFailed: {
            console.log("Login Failed")
        }
    }

    Keys.onPressed: {
        if (!visible) {
            return
        }

        switch (event.key) {
            case Qt.Key_Enter:
            case Qt.Key_Return:
                event.accepted = true
                usernameCollisionBody.create()
                break
        }
    }
}
