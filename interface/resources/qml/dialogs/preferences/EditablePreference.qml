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
import "../../controls" as Controls

Preference {
    id: root
    height: dataTextField.controlHeight + hifi.dimensions.controlInterlineHeight + (editablePreferenceColumn.keyboardRaised ? 200 : 0)

    Component.onCompleted: {
        dataTextField.text = preference.value;
    }

    function save() {
        preference.value = dataTextField.text;
        preference.save();
    }

    Column {
        id: editablePreferenceColumn
        height: dataTextField.height + (keyboardRaised ? 200 : 0)

        property bool keyboardRaised: false
        property bool punctuationMode: false

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        TextField {
            id: dataTextField
            placeholderText: preference.placeholderText
            label: root.label
            colorScheme: hifi.colorSchemes.dark

            anchors {
                left: parent.left
                right: parent.right
                // bottom: keyboard1.bottom
            }
        }

        // virtual keyboard, letters
        Controls.Keyboard {
            id: keyboard1
            // y: parent.keyboardRaised ? parent.height : 0
            height: parent.keyboardRaised ? 200 : 0
            visible: parent.keyboardRaised && !parent.punctuationMode
            enabled: parent.keyboardRaised && !parent.punctuationMode
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            // anchors.bottom: parent.bottom
            // anchors.bottomMargin: 0
            // anchors.bottomMargin: 2 * hifi.dimensions.contentSpacing.y
        }

        Controls.KeyboardPunctuation {
            id: keyboard2
            // y: parent.keyboardRaised ? parent.height : 0
            height: parent.keyboardRaised ? 200 : 0
            visible: parent.keyboardRaised && parent.punctuationMode
            enabled: parent.keyboardRaised && parent.punctuationMode
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            // anchors.bottom: parent.bottom
            // anchors.bottomMargin: 0
            // anchors.bottomMargin: 2 * hifi.dimensions.contentSpacing.y
        }
    }
}
