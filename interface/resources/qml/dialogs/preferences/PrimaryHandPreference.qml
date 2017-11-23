//
//  PrimaryHandPreference.qml
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
    property alias box1: box1
    property alias box2: box2
    
    height: control.height + hifi.dimensions.controlInterlineHeight

    Component.onCompleted: {
        if (preference.value == "left") {
            box1.checked = true;
        } else {
            box2.checked = true;
        }
    }

    function save() {
        if (box1.checked && !box2.checked) {
            preference.value = "left";
        }
        if (!box1.checked && box2.checked) {
            preference.value = "right";
        }
        preference.save();
    }

    Item {
        id: control
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: Math.max(labelName.height, box1.height, box2.height)

        Label {
            id: labelName
            text: root.label + ":"
            colorScheme: hifi.colorSchemes.dark
            anchors {
                left: parent.left
                right: box1.left
                rightMargin: hifi.dimensions.labelPadding
                verticalCenter: parent.verticalCenter
            }
            horizontalAlignment: Text.AlignRight
            wrapMode: Text.Wrap
        }

        RadioButton {
            id: box1
            text: "Left"
            width: 60
            anchors {
                right: box2.left
                verticalCenter: parent.verticalCenter
            }
            onClicked: {
                if (box2.checked) {
                    box2.checked = false;
                }
                if (!box1.checked && !box2.checked) {
                    box1.checked = true;
                }
            }
            colorScheme: hifi.colorSchemes.dark
        }
        
        RadioButton {
            id: box2
            text: "Right"
            width: 60
            anchors {
                right: parent.right
                verticalCenter: parent.verticalCenter
            }
            onClicked: {
                if (box1.checked) {
                    box1.checked = false;
                }
                if (!box1.checked && !box2.checked) {
                    box2.checked = true;
                }
            }
            colorScheme: hifi.colorSchemes.dark
        }
    }
}
