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
    color: "transparent";
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
    Rectangle {
        // This is crazy. All of this stuff (except the opacity) ought to be in the parent, with the label drawn on top.
        // But there's a bug in QT such that if you select this TextButton, AND THEN enter the area of
        // a TextButton created before this one, AND THEN enter a ListView with a highlight, then our label
        // will draw as though it on the bottom. (If the phase of the moon is right, it will do this for a
        // about half a second and then render normally. But if you're not lucky it just stays this way.)
        // So.... here we deliberately put the rectangle on TOP of the text so that you can't tell when the bug
        // is happening.
        anchors.fill: parent;
        radius: height / 2;
        border.width: 4;
        border.color: clickArea.containsMouse ? highlightColor : "transparent";
        color:  clickArea.containsPress ? hifi.colors.darkGray : (selected ? hifi.colors.blueAccent : "transparent");
        opacity: (clickArea.containsMouse && !clickArea.containsPress) ? 0.8 : 0.5;
    }
    MouseArea {
        id: clickArea;
        anchors.fill: parent;
        acceptedButtons: Qt.LeftButton;
        onClicked: action(parent);
        hoverEnabled: true;
    }
}
