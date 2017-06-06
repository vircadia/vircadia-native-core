//
//  Switch.qml
//
//  Created by Zach Fox on 2017-06-06
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4 as Original
import QtQuick.Controls.Styles 1.4

import "../styles-uit"

Item {
    id: rootSwitch;

    property int colorScheme: hifi.colorSchemes.light;
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light;
    property int switchWidth: 100;
    readonly property int switchRadius: 3;
    property string offLabelText: "";
    property string onLabelText: "";
    property string switchLabelText: "";

    Original.Switch {
        id: originalSwitch;
        activeFocusOnPress: true;
        anchors.top: rootSwitch.top;
        anchors.left: switchLabel.right;
        anchors.leftMargin: 4;

        style: SwitchStyle {
            groove: Rectangle {
                implicitWidth: rootSwitch.switchWidth;
                implicitHeight: rootSwitch.height;
                radius: rootSwitch.switchRadius;
                border.color: control.activeFocus ? "darkblue" : "gray";
                border.width: 1;
            }

            handle: Rectangle {
                opacity: rootSwitch.enabled ? 1.0 : 0.5
                implicitWidth: rootSwitch.height - 5;
                implicitHeight: implicitWidth;
                radius: implicitWidth/2;
                border.color: hifi.colors.primaryHighlight;
            }
        }
    }

    MouseArea {
        anchors.fill: parent;
        onClicked: {
            originalSwitch.checked = !originalSwitch.checked;
        }
    }
}
