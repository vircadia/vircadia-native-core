//
//  TabletStats.qml
//  scripts/developer/utilities/audio
//
//  Created by David Rowe on 3 Mar 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3

import "../../../../resources/qml/styles-uit"

Item {
    id: dialog
    width: 480
    height: 720

    HifiConstants { id: hifi }

    Rectangle {
        id: header
        height: 90
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
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
            text: "Audio Interface Statistics"
            size: 26
            color: "#34a2c7"
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: hifi.dimensions.contentMargin.x  // ####### hifi is not defined
        }
    }

    Rectangle {
        id: main
        anchors {
            top: header.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }

        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#2b2b2b"
            }

            GradientStop {
                position: 1
                color: "#0f212e"
            }
        }

        Flickable {
            id: scrollView
            width: parent.width
            height: parent.height
            contentWidth: parent.width
            contentHeight: stats.height

            Stats {
                id: stats
            }
        }
    }
}
