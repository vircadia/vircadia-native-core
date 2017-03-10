//
//  TabletAttachmentsDialog.qml
//
//  Created by David Rowe on 9 Mar 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4

import "../../controls-uit" as HifiControls
import "../../styles-uit"
import "../dialogs/content"

Item {
    id: root
    objectName: "AttachmentsDialog"

    property string title: "Avatar Attachments"

    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false

    anchors.fill: parent

    HifiConstants { id: hifi }

    Rectangle {
        id: pane  // Surrogate for ScrollingWindow's pane.
        anchors.fill: parent
    }

    function closeDialog() {
        Tablet.getTablet("com.highfidelity.interface.tablet.system").gotoHomeScreen();
    }

    anchors.topMargin: 90  // Space for header.

    // FIXME: Refactor with other tablet headers.
    Rectangle {
        id: header
        height: 90
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.top
        }
        z: 100

        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#2b2b2b"
            }

            GradientStop {
                position: 1
                color: "#1e1e1e"
            }
        }

        RalewayBold {
            text: title
            size: 26
            color: "#34a2c7"
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: hifi.dimensions.contentMargin.x
        }
    }

    AttachmentsContent {
        id: attachments

        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            bottom: keyboard.top
        }

        MouseArea {
            // Defocuses any current control so that the keyboard gets hidden.
            id: defocuser
            anchors.fill: parent
            propagateComposedEvents: true
            acceptedButtons: Qt.AllButtons
            onPressed: {
                parent.forceActiveFocus();
                mouse.accepted = false;
            }
        }
    }

    HifiControls.Keyboard {
        id: keyboard
        raised: parent.keyboardEnabled && parent.keyboardRaised
        numeric: parent.punctuationMode
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }

    MouseArea {
        id: activator
        anchors.fill: parent
        propagateComposedEvents: true
        enabled: true
        acceptedButtons: Qt.AllButtons
        onPressed: {
            mouse.accepted = false;
        }
    }

    Component.onCompleted: {
        keyboardEnabled = HMD.active;
    }
}
