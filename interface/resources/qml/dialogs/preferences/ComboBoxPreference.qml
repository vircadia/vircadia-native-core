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

import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import "../../controls-uit" as HiFiControls
import "../../styles-uit"

Preference {
    id: root
    height: dataComboBox.controlHeight

    Component.onCompleted: {
        dataComboBox.currentIndex = dataComboBox.comboBox.find(preference.value);
    }

    function save() {
        preference.value = dataComboBox.currentText;
        //preference.value = comboBox.currentText;
        preference.save();
    }

    HiFiControls.Label {
        text: root.label + ":"
        colorScheme: hifi.colorSchemes.dark
        anchors {
            left: parent.left
            right: dataComboBox.left
            rightMargin: hifi.dimensions.contentSpacing.x
            verticalCenter: dataComboBox.verticalCenter
        }
        wrapMode: Text.Wrap
    }

    HiFiControls.ComboBox {
        id: dataComboBox
        model: preference.items
        width: 150
        anchors { right: parent.right }
        colorScheme: hifi.colorSchemes.dark
    }
}
