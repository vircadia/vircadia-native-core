//
//  TextButton.qml
//
//  Created by Howard Stearns 11/12/16
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
import Hifi 1.0
import QtQuick 2.4
import "../styles-uit"

Rectangle {
    property alias text: label.text;
    property alias pixelSize: label.font.pixelSize;
    property bool selected: false;
    property int spacing: 2;
    property var action: function () { };
    property string highlightColor: hifi.colors.blueHighlight;
    width: label.width + 64;
    height: 32;
    radius: height / 2;
    border.width: (clickArea.containsMouse && !clickArea.containsPress) ? 3 : 0;
    border.color: highlightColor;
    color: clickArea.containsPress ? hifi.colors.darkGray : (selected ? highlightColor : "transparent");
    HifiConstants { id: hifi; }
    RalewaySemiBold {
        id: label;
        color: hifi.colors.white;
        font.pixelSize: 20;
        anchors {
            horizontalCenter: parent.horizontalCenter;
            verticalCenter: parent.verticalCenter;
        }
    }
    MouseArea {
        id: clickArea;
        anchors.fill: parent;
        acceptedButtons: Qt.LeftButton;
        onClicked: action(parent);
        hoverEnabled: true;
    }

}
