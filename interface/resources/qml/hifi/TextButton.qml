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
    width: label.width + (4 * spacing);
    height: label.height + spacing;
    radius: 2 * spacing;
    color: selected ? "grey" : "transparent";
    HifiConstants { id: hifi; }
    RalewaySemiBold {
        id: label;
        color: hifi.colors.white;
        font.pixelSize: 25;
        anchors {
            horizontalCenter: parent.horizontalCenter;
            verticalCenter: parent.verticalCenter;
        }
    }
    MouseArea {
        anchors.fill: parent;
        acceptedButtons: Qt.LeftButton;
        onClicked: action(parent);
        hoverEnabled: true;
        onEntered: label.color = "#1DB5ED";
        onExited: label.color = hifi.colors.white;
    }

}
