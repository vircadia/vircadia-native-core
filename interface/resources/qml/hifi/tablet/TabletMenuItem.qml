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

import controlsUit 1.0
import stylesUit 1.0

Item {
    id: root
    HifiConstants { id: hifi  }
    property alias text: label.text
    property var source

    implicitHeight: source !== null ? source.visible ? 2 * label.implicitHeight : 0 : 0
    implicitWidth: 2 * hifi.dimensions.menuPadding.x + check.width + label.width + tail.width
    visible: source !== null ? source.visible : false
    width: parent.width

    Item {
        id: check

        anchors {
            left: parent.left
            leftMargin: hifi.dimensions.menuPadding.x + 15
            verticalCenter: label.verticalCenter
        }

        width: checkbox.visible ? checkbox.width : radiobutton.width
        height: checkbox.visible ? checkbox.height : radiobutton.height

        CheckBox {
            id: checkbox

            width: 20
            visible: source !== null ?
                         source.visible && source.type === 1 && source.checkable && !source.exclusiveGroup :
                         false

            Binding on checked {
                value: source.checked;
                when: source && source.type === 1 && source.checkable && !source.exclusiveGroup;
            }
        }

        RadioButton {
            id: radiobutton

            width: 20
            visible: source !== null ?
                         source.visible && source.type === 1 && source.checkable && source.exclusiveGroup :
                         false

            Binding on checked {
                value: source.checked;
                when: source && source.type === 1 && source.checkable && source.exclusiveGroup;
            }
        }
    }

    RalewaySemiBold {
        id: label
        size: 20
        //wrap will work only if width is set
        width: parent.width - (check.width + check.anchors.leftMargin) - tail.width
        font.capitalization: isSubMenu ? Font.MixedCase : Font.AllUppercase
        anchors.left: check.right
        anchors.verticalCenter: parent.verticalCenter
        verticalAlignment: Text.AlignVCenter
        color: source !== null ?
                   source.enabled ? hifi.colors.baseGrayShadow :
                                    hifi.colors.baseGrayShadow50 :
        "transparent"

        enabled: source !== null ? source.visible && (source.type !== 0 ? source.enabled : false) : false
        visible: source !== null ? source.visible : false
        wrapMode: Text.WordWrap
    }

    Item {
        id: separator
        anchors {
            fill: parent
            leftMargin: hifi.dimensions.menuPadding.x + check.width
            rightMargin: hifi.dimensions.menuPadding.x + tail.width
        }
        visible: source !== null ? source.type === MenuItemType.Separator : false

        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
                verticalCenter: parent.verticalCenter
            }
            height: 1
            color: hifi.colors.lightGray50
        }
    }

    Item {
        id: tail
        width: 48 + (shortcut.visible ? shortcut.width : 0)
        anchors {
            verticalCenter: parent.verticalCenter
            right: parent.right
            rightMargin: hifi.dimensions.menuPadding.x
        }

        RalewayLight {
            id: shortcut
            text: source !== null ? source.shortcut ? source.shortcut : "" : ""
            size: hifi.fontSizes.shortcutText
            color: hifi.colors.baseGrayShadow
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 15
            visible: source !== null ? source.visible && text != "" : false
        }

        HiFiGlyphs {
            text: hifi.glyphs.disclosureExpand
            color: source !== null ? source.enabled ? hifi.colors.baseGrayShadow : hifi.colors.baseGrayShadow25 : "transparent"
            size: 70
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            horizontalAlignment: Text.AlignRight
            visible: source !== null ? source.visible && (source.type === 2) : false
        }
    }
}
