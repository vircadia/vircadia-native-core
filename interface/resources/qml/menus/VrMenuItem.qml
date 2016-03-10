//
//  VrMenuItem.qml
//
//  Created by Bradley Austin Davis on 29 Apr 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

import "../controls"
import "../styles"

Item {
    id: root
    HifiConstants { id: hifi  }
    property alias text: label.text
    property var source

    implicitHeight: source.visible ? label.implicitHeight * 1.5 : 0
    implicitWidth: label.width + label.height * 2.5
    visible: source.visible
    width: parent.width

    FontAwesome {
        clip: true
        id: check
        verticalAlignment:  Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        anchors.verticalCenter: parent.verticalCenter
        color: label.color
        text: checkText()
        size: label.height
        visible: source.visible
        font.pixelSize: size
        function checkText() {
            if (!source || source.type != 1 || !source.checkable) {
                return ""
            }
            // FIXME this works for native QML menus but I don't think it will
            // for proxied QML menus
            if (source.exclusiveGroup) {
                return source.checked ? "\uF05D" : "\uF10C"
            }
            return source.checked ? "\uF046" : "\uF096"
        }
    }

    Text {
        id: label
        anchors.left: check.right
        anchors.leftMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        verticalAlignment: Text.AlignVCenter
        color: source.enabled ? hifi.colors.text : hifi.colors.disabledText
        enabled: source.visible && (source.type !== 0 ? source.enabled : false)
        visible: source.visible
    }

    FontAwesome {
        id: tag
        x: root.parent.width - width
        size: label.height
        width: implicitWidth
        visible: source.visible && (source.type == 2)
        text: "\uF0DA"
        anchors.verticalCenter: parent.verticalCenter
        color: label.color
    }
}
