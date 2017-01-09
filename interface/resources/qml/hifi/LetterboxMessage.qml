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
    property real radius: hifi.dimensions.borderRadius
    visible: false
    id: letterbox
    anchors.fill: parent
    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.5
        radius: radius
    }
    Rectangle {
        width: Math.max(parent.width * 0.75, 400)
        height: popupText.contentHeight*1.5
        anchors.centerIn: parent
        radius: radius
        color: "white"
        FiraSansSemiBold {
            id: popupText
            size: hifi.fontSizes.textFieldInput
            color: hifi.colors.darkGray
            horizontalAlignment: Text.AlignHCenter
            anchors.fill: parent
            anchors.leftMargin: 15
            anchors.rightMargin: 15
            wrapMode: Text.WordWrap
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
