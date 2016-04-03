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

import "../../controls-uit"

Preference {
    id: root
    property alias spinner: spinner
    height: control.height + hifi.dimensions.controlInterlineHeight

    Component.onCompleted: {
        spinner.value = preference.value;
    }

    function save() {
        preference.value = spinner.value;
        preference.save();
    }

    Item {
        id: control
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: Math.max(spinnerLabel.height, spinner.controlHeight)

        Label {
            id: spinnerLabel
            text: root.label + ":"
            colorScheme: hifi.colorSchemes.dark
            anchors {
                left: parent.left
                right: spinner.left
                rightMargin: hifi.dimensions.labelPadding
                verticalCenter: parent.verticalCenter
            }
            horizontalAlignment: Text.AlignRight
            wrapMode: Text.Wrap
        }

        SpinBox {
            id: spinner
            decimals: preference.decimals
            minimumValue: preference.min
            maximumValue: preference.max
            width: 100
            anchors {
                right: parent.right
                verticalCenter: parent.verticalCenter
            }
            colorScheme: hifi.colorSchemes.dark
        }
    }
}
