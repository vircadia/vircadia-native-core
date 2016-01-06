//
//  ErrorDialog.qml
//
//  Created by David Rowe on 30 May 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import "controls"
import "styles"

DialogContainer {
    id: root
    HifiConstants { id: hifi }

    Component.onCompleted: {
        enabled = true
    }

    onParentChanged: {
        if (visible && enabled) {
            forceActiveFocus()
        }
    }

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    x: parent ? parent.width / 2 - width / 2 : 0
    y: parent ? parent.height / 2 - height / 2 : 0

    ErrorDialog {
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

                font.pixelSize: hifi.fonts.pixelSize * 0.6
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
                    cursorShape: "PointingHandCursor"
                    onClicked: {
                        content.accept()
                    }
                }
            }
        }
    }

    Keys.onPressed: {
        if (!enabled) {
            return
        }
        switch (event.key) {
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
