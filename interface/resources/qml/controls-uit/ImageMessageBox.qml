//
//  ImageMessageBox.qml
//
//  Created by Dante Ruiz on 7/5/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import "../styles-uit"

Item {
    id: imageBox
    visible: false
    anchors.fill: parent
    property alias source: image.source
    property alias imageWidth: image.width
    property alias imageHeight: image.height

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.3
    }

    Image {
        id: image
        anchors.centerIn: parent

        HiFiGlyphs {
            id: closeGlyphButton
            text: hifi.glyphs.close
            size: 25

            anchors {
                top: parent.top
                topMargin: 15
                right: parent.right
                rightMargin: 15
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true

                onEntered: {
                    parent.text = hifi.glyphs.closeInverted;
                }

                onExited: {
                    parent.text = hifi.glyphs.close;
                }

                onClicked: {
                    imageBox.visible = false;
                }
            }
        }
    }

}
