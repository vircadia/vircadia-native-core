//
//  LetterboxMessage.qml
//  qml/hifi
//
//  Created by Zach Fox and Howard Stearns on 1/5/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import "../styles-uit"

Item {
    property alias text: popupText.text
    property alias headerGlyph: headerGlyph.text
    property alias headerText: headerText.text
    property real popupRadius: hifi.dimensions.borderRadius
    property real headerTextPixelSize: 22
    FontLoader { id: ralewayRegular; source: "../../fonts/Raleway-Regular.ttf"; }
    FontLoader { id: ralewaySemiBold; source: "../../fonts/Raleway-SemiBold.ttf"; }
    visible: false
    id: letterbox
    anchors.fill: parent
    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.5
        radius: popupRadius
    }
    Rectangle {
        width: Math.max(parent.width * 0.75, 400)
        height: contentContainer.height*1.5
        anchors.centerIn: parent
        radius: popupRadius
        color: "white"
        Item {
            id: contentContainer
            anchors.centerIn: parent
            anchors.margins: 20
            height: childrenRect.height
            Item {
                id: popupHeaderContainer
                visible: headerText.text !== "" || glyphText.text !== ""
                // Size
                width: parent.width
                height: childrenRect.height
                // Anchors
                anchors.top: parent.top
                anchors.left: parent.left
                // Header Glyph
                HiFiGlyphs {
                    id: headerGlyph
                    visible: headerText.text !== ""
                    // Text Size
                    size: headerTextPixelSize * 2.5
                    // Anchors
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.rightMargin: 5
                    // Style
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: hifi.colors.darkGray
                }
                Text {
                    id: headerText
                    visible: headerGlyph.text !== ""
                    // Text Size
                    font.pixelSize: headerTextPixelSize
                    // Anchors
                    anchors.top: parent.top
                    anchors.left: headerGlyph.right
                    // Style
                    font.family: ralewaySemiBold.name
                    color: hifi.colors.darkGray
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    textFormat: Text.StyledText
                }
            }
            Text {
                id: popupText
                // Anchors
                anchors.top: popupHeaderContainer.visible ? popupHeaderContainer.anchors.bottom : parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                // Text alignment
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                // Style
                font.pixelSize: hifi.fontSizes.textFieldInput
                font.family: ralewayRegular.name
                color: hifi.colors.darkGray
                wrapMode: Text.WordWrap
                textFormat: Text.StyledText
            }
        }
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onClicked: {
            letterbox.visible = false
        }
    }
}
