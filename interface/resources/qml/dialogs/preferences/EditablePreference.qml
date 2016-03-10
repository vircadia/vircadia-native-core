//
//  EditablePreference.qml
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
    height: dataTextField.controlHeight + hifi.dimensions.controlInterlineHeight

    Component.onCompleted: {
        dataTextField.text = preference.value;
    }

    function save() {
        preference.value = dataTextField.text;
        preference.save();
    }

    TextField {
        id: dataTextField
        placeholderText: preference.placeholderText
        label: root.label
        colorScheme: hifi.colorSchemes.dark

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }
}
