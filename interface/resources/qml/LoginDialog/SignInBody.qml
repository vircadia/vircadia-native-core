//
//  SignInBody.qml
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
    id: signInBody
    clip: true
    width: pane.width
    height: pane.height

    property bool required: false

    function login() {
        console.log("Trying to log in")
        loginDialog.loginThroughSteam()
    }

    function cancel() {
        root.destroy()
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

            root.width = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth))
            root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight))
        }
    }

    MenuItem {
        id: mainTextContainer
        anchors {
            top: parent.top
            horizontalCenter: parent.horizontalCenter
            margins: 0
            topMargin: hifi.dimensions.contentSpacing.y
        }

        text: required ? qsTr("This domain's owner requires that you sign in:")
                       : qsTr("Sign in to access your user account:")
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

            width: undefined // invalidate so that the image's size sets the width
            height: undefined // invalidate so that the image's size sets the height
            focus: true

            style: OriginalStyles.ButtonStyle {
                background: Image {
                    id: buttonImage
                    source: "../../images/steam-sign-in.png"
                }
            }
            onClicked: signInBody.login()
        }
        Button {
            anchors.verticalCenter: parent.verticalCenter

            text: qsTr("Cancel");

            onClicked: signInBody.cancel()
        }
    }

    Component.onCompleted: {
        root.title = required ? qsTr("Sign In Required")
                              : qsTr("Sign In")
        root.iconText = ""
        d.resize();
    }
}
