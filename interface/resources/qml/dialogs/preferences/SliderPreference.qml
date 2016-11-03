//
//  SpinBoxPreference.qml
//
//  Created by Bradley Austin Davis on 18 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

import "../../dialogs"
import "../../controls-uit"

Preference {
    id: root
    property alias slider: slider
    height: control.height + hifi.dimensions.controlInterlineHeight

    Component.onCompleted: {
        slider.value = preference.value;
    }

    function save() {
        preference.value = slider.value;
        preference.save();
    }

    Item {
        id: control
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: Math.max(labelText.height, slider.height)

        Label {
            id: labelText
            text: root.label + ":"
            colorScheme: hifi.colorSchemes.dark
            anchors {
                left: parent.left
                right: slider.left
                rightMargin: hifi.dimensions.labelPadding
                verticalCenter: parent.verticalCenter
            }
            horizontalAlignment: Text.AlignRight
            wrapMode: Text.Wrap
        }

        Slider {
            id: slider
            value: preference.value
            width: 130
            anchors {
                right: parent.right
                verticalCenter: parent.verticalCenter
            }
            colorScheme: hifi.colorSchemes.dark
        }
    }
}
