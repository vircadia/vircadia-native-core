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

import controlsUit 1.0

Preference {
    id: root
    property alias spinner: spinner
    height: control.height + hifi.dimensions.controlInterlineHeight

    Component.onCompleted: {
        spinner.realValue = preference.value;
    }

    function save() {
        preference.value = spinner.realValue;
        preference.save();
    }

    Row {
        id: control
        anchors {
            bottom: parent.bottom
        }
        height: Math.max(spinnerLabel.height, spinner.controlHeight)

        Label {
            id: spinnerLabel
            text: root.label + ":"
            colorScheme: hifi.colorSchemes.dark
            anchors {
                verticalCenter: parent.verticalCenter
            }
            horizontalAlignment: Text.AlignRight
            wrapMode: Text.Wrap
        }

        spacing: hifi.dimensions.labelPadding

        SpinBox {
            id: spinner
            decimals: preference.decimals
            minimumValue: preference.min
            maximumValue: preference.max
            realStepSize: preference.step
            width: 100
            anchors {
                verticalCenter: parent.verticalCenter
            }
            colorScheme: hifi.colorSchemes.dark
        }
    }
}
