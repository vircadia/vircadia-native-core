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
    height: slider.controlHeight

    Component.onCompleted: {
        slider.value = preference.value;
    }

    function save() {
        preference.value = slider.value;
        preference.save();
    }

    Label {
        text: root.label + ":"
        colorScheme: hifi.colorSchemes.dark
        anchors {
            left: parent.left
            right: slider.left
            rightMargin: hifi.dimensions.labelPadding
            verticalCenter: slider.verticalCenter
        }
        horizontalAlignment: Text.AlignRight
        wrapMode: Text.Wrap
    }

    Slider {
        id: slider
        value: preference.value
        width: 130
        anchors { right: parent.right }
        colorScheme: hifi.colorSchemes.dark
    }
}
