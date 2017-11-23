//
//  LetterboxMessage.qml
//  qml/hifi
//
//  Created by Dante Ruiz on 7/21/2017
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
    property alias headerGlyphSize: headerGlyph.size
    property real popupRadius: hifi.dimensions.borderRadius
    property real headerTextPixelSize: 22
    property real popupTextPixelSize: 16
    property real headerTextMargin: -5
    property real headerGlyphMargin: -15
    property bool isDesktop: false
    FontLoader { id: ralewayRegular; source: "../../fonts/Raleway-Regular.ttf"; }
    FontLoader { id: ralewaySemiBold; source: "../../fonts/Raleway-SemiBold.ttf"; }
    visible: false
    id: letterbox
    anchors.fill: parent
    Rectangle {
        id: textContainer;
        width: parent.width
        height: parent.height
        anchors.centerIn: parent
        radius: popupRadius
        color: "white"
        Item {
            id: contentContainer
            width: parent.width - 50
            height: childrenRect.height
            anchors.centerIn: parent
            Item {
                id: popupHeaderContainer
                visible: headerText.text !== "" || headerGlyph.text !== ""
                height: 30
                // Anchors
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                // Header Glyph
                HiFiGlyphs {
                    id: headerGlyph
                    visible: headerGlyph.text !== ""
                    // Size
                    height: parent.height
                    // Anchors
                    anchors.left: parent.left
                    anchors.leftMargin: headerGlyphMargin
                    // Text Size
                    size: headerTextPixelSize*2.5
                    // Style
                    horizontalAlignment: Text.AlignHLeft
                    verticalAlignment: Text.AlignVCenter
                    color: hifi.colors.darkGray
                }
                // Header Text
                Text {
                    id: headerText
                    visible: headerText.text !== ""
                    // Size

                    height: parent.height
                    // Anchors
                    anchors.left: headerGlyph.right
                    anchors.leftMargin: headerTextMargin
                    // Text Size
                    font.pixelSize: headerTextPixelSize
                    // Style
                    font.family: ralewaySemiBold.name
                    color: hifi.colors.darkGray
                    horizontalAlignment: Text.AlignHLeft
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    textFormat: Text.StyledText
                }
            }
            // Popup Text
            Text {
                id: popupText
                // Size
                width: parent.width
                // Anchors
                anchors.top: popupHeaderContainer.visible ? popupHeaderContainer.bottom : parent.top
                anchors.topMargin: popupHeaderContainer.visible ? 15 : 0
                anchors.left: parent.left
                anchors.right: parent.right
                // Text alignment
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHLeft
                // Style
                font.pixelSize: popupTextPixelSize
                font.family: ralewayRegular.name
                color: hifi.colors.darkGray
                wrapMode: Text.WordWrap
                textFormat: Text.StyledText
                onLinkActivated: {
                    Qt.openUrlExternally(link)
                }
            }
        }
    }
    // Left gray MouseArea
    MouseArea {
        anchors.left: parent.left;
        anchors.right: textContainer.left;
        anchors.top: textContainer.top;
        anchors.bottom: textContainer.bottom;
        acceptedButtons: Qt.LeftButton
        onClicked: {
            letterbox.visible = false
        }
    }
    // Right gray MouseArea
    MouseArea {
        anchors.left: textContainer.left;
        anchors.right: parent.left;
        anchors.top: textContainer.top;
        anchors.bottom: textContainer.bottom;
        acceptedButtons: Qt.LeftButton
        onClicked: {
            letterbox.visible = false
        }
    }
    // Top gray MouseArea
    MouseArea {
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.top: parent.top;
        anchors.bottom: textContainer.top;
        acceptedButtons: Qt.LeftButton
        onClicked: {
            letterbox.visible = false
        }
    }
    // Bottom gray MouseArea
    MouseArea {
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.top: textContainer.bottom;
        anchors.bottom: parent.bottom;
        acceptedButtons: Qt.LeftButton
        onClicked: {
            letterbox.visible = false
        }
    }
}
