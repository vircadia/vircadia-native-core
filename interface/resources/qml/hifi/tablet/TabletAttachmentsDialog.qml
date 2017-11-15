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

    signal sendToScript(var message);

    anchors.fill: parent

    HifiConstants { id: hifi }

    Rectangle {
        id: pane  // Surrogate for ScrollingWindow's pane.
        anchors.fill: parent
    }

    function closeDialog() {
        Tablet.getTablet("com.highfidelity.interface.tablet.system").gotoHomeScreen();
    }

    anchors.topMargin: hifi.dimensions.tabletMenuHeader  // Space for header.

    HifiControls.TabletHeader {
        id: header
        title: root.title

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.top
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
