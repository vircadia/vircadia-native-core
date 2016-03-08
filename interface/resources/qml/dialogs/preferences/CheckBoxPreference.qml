//
//  CheckBoxPreference.qml
//
//  Created by Bradley Austin Davis on 18 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

import "../../controls-uit"

Preference {
    id: root
    height: spacer.height + Math.max(hifi.dimensions.controlLineHeight, checkBox.implicitHeight)

    Component.onCompleted: {
        checkBox.checked = preference.value;
        preference.value = Qt.binding(function(){ return checkBox.checked; });
    }

    function save() {
        preference.value = checkBox.checked;
        preference.save();
    }

    Item {
        id: spacer
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: isFirstCheckBox ? hifi.dimensions.controlInterlineHeight : 0
    }

    CheckBox {
        id: checkBox
        anchors {
            top: spacer.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        text: root.label
        colorScheme: hifi.colorSchemes.dark
    }
}
