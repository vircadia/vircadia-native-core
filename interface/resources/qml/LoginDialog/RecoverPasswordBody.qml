//
//  RecoverPasswordBody.qml
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
    id: recoverPasswordBody
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
            var targetWidth = Math.max(titleWidth, mainTextContainer.contentWidth)
            var targetHeight = mainTextContainer.height +
                               3 * hifi.dimensions.contentSpacing.y + emailField.height +
                               4 * hifi.dimensions.contentSpacing.y + buttons.height

            root.width = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth))
            root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight))
        }
    }

    MenuItem {
        id: mainTextContainer
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: 0
            topMargin: hifi.dimensions.contentSpacing.y
        }

        text: qsTr("In order to help you reset your password, we will send an<br/>email with instructions to your email address.")
        wrapMode: Text.WordWrap
        color: hifi.colors.baseGrayHighlight
        lineHeight: 1
        horizontalAlignment: Text.AlignHLeft
    }


    TextField {
        id: emailField
        anchors {
            top: mainTextContainer.bottom
            left: parent.left
            margins: 0
            topMargin: 2 * hifi.dimensions.contentSpacing.y
        }

        width: 350

        label: "Email address"
    }

    Row {
        id: buttons
        anchors {
            top: emailField.bottom
            right: parent.right
            margins: 0
            topMargin: 3 * hifi.dimensions.contentSpacing.y
        }
        spacing: hifi.dimensions.contentSpacing.x
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Button {
            anchors.verticalCenter: parent.verticalCenter
            width: 200

            text: qsTr("Send recovery email")
            color: hifi.buttons.blue
        }

        Button {
            anchors.verticalCenter: parent.verticalCenter

            text: qsTr("Back")
        }
    }

    Component.onCompleted: {
        root.title = qsTr("Recover Password")
        root.iconText = "<"
        d.resize();
    }
}
