//
//  ErrorDialog.qml
//
//  Created by David Rowe on 30 May 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import "controls"
import "styles"

Item {
    id: root
    HifiConstants { id: hifi }

    property int animationDuration: hifi.effects.fadeInDuration
    property bool destroyOnInvisible: true

    Component.onCompleted: {
        enabled = true
    }

    onParentChanged: {
        if (visible && enabled) {
            forceActiveFocus();
        }
    }

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    x: parent ? parent.width / 2 - width / 2 : 0
    y: parent ? parent.height / 2 - height / 2 : 0

    Hifi.ErrorDialog {
        id: content

        implicitWidth: box.width
        implicitHeight: icon.height + hifi.layout.spacing * 2

        Border {
            id: box

            width: 512
            color: "#ebebeb"
            radius: 2
            border.width: 1
            border.color: "#000000"

            Image {
                id: icon

                source: "../images/address-bar-error-icon.svg"
                width: 40
                height: 40
                anchors {
                    left: parent.left
                    leftMargin: hifi.layout.spacing
                    verticalCenter: parent.verticalCenter
                }
            }

            Text {
                id: messageText

                font.pointSize: 10
                font.weight: Font.Bold

                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }

                text: content.text
            }

            Image {
                source: "../images/address-bar-error-close.svg"
                width: 20
                height: 20
                anchors {
                    right: parent.right
                    rightMargin: hifi.layout.spacing * 2
                    verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        content.accept();
                    }
                }
            }
        }
    }

    // The UI enables an object, rather than manipulating its visibility, so that we can do animations in both directions.
    // Because visibility and enabled are booleans, they cannot be animated. So when enabled is changed, we modify a property
    // that can be animated, like scale or opacity, and then when the target animation value is reached, we can modify the
    // visibility.
    enabled: false
    opacity: 0.0

    onEnabledChanged: {
        opacity = enabled ? 1.0 : 0.0
    }

    Behavior on opacity {
        // Animate opacity.
        NumberAnimation {
            duration: animationDuration
            easing.type: Easing.OutCubic
        }
    }

    onOpacityChanged: {
        // Once we're transparent, disable the dialog's visibility.
        visible = (opacity != 0.0)
    }

    onVisibleChanged: {
        if (!visible) {
            // Some dialogs should be destroyed when they become invisible.
            if (destroyOnInvisible) {
                destroy()
            }
        }
    }

    Keys.onPressed: {
        if (event.modifiers === Qt.ControlModifier)
            switch (event.key) {
            case Qt.Key_W:
                event.accepted = true
                content.accept()
                break
        } else switch (event.key) {
            case Qt.Key_Escape:
            case Qt.Key_Back:
            case Qt.Key_Enter:
            case Qt.Key_Return:
                event.accepted = true
                content.accept()
                break
        }
    }
}
