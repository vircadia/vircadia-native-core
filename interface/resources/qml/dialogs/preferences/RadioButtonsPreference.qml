//
//  RadioButtonsPreference.qml
//
//  Created by Cain Kilgore on 20th July 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

import "../../controls-uit"

Preference {
    id: root
    
    height: control.height + hifi.dimensions.controlInterlineHeight

    Component.onCompleted: {
        repeater.itemAt(preference.value).checked = true
    }

    function save() {
        var value = 0;
        for (var i = 0; i < repeater.count; i++) {
            if (repeater.itemAt(i).checked) {
                value = i;
                break;
            }
        }
        preference.value = value;
        preference.save();
    }

    Row {
        id: control
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        spacing: 5

        Repeater {
            id: repeater
            model: preference.items.length
            delegate: RadioButton {
                text: preference.items[index]
                anchors {
                    verticalCenter: parent.verticalCenter
                }
                colorScheme: hifi.colorSchemes.dark
            }
        }
    }
}
