//
//  ComboBoxPreference.qml
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
    property alias comboBox: comboBox
    height: comboBox.controlHeight

    Component.onCompleted: {
        comboBox.currentIndex = comboBox.find(preference.value);
    }

    function save() {
        preference.value = comboBox.currentText;
        preference.save();
    }

    Label {
        text: root.label + ":"
        colorScheme: hifi.colorSchemes.dark
        anchors.verticalCenter: comboBox.verticalCenter
    }

    ComboBox {
        id: comboBox
        model: preference.items
        width: 150
        anchors { right: parent.right }
        colorScheme: hifi.colorSchemes.dark
    }
}
