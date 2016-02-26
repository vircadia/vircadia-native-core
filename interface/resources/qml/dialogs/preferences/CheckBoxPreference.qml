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
    height: checkBox.implicitHeight

    Component.onCompleted: {
        checkBox.checked = preference.value;
        preference.value = Qt.binding(function(){ return checkBox.checked; });
    }

    function save() {
        preference.value = checkBox.checked;
        preference.save();
    }

    CheckBox {
        id: checkBox
        anchors.fill: parent
        text: root.label
        colorScheme: hifi.colorSchemes.dark
    }
}
