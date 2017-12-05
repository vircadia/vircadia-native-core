//
//  XColor.qml
//
//  Created by Sam Gateau 12/4/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 1.4 as Original
import QtQuick.Controls.Styles 1.4

import "qrc:///qml/styles-uit"
import "qrc:///qml/controls-uit" as HifiControls


Item {
    HifiConstants { id: hifi }
    id: root

    anchors.left: parent.left
    anchors.right: parent.right    
    height: 24
    property var color
    property alias label: labelControl.text

    function getColor() {
        return Qt.rgba(color.red / 255.0, color.green / 255.0, color.blue / 255.0, 1.0 );
    }

    Rectangle {
        id: current
        HifiConstants { id: hifi;}
        color: root.getColor();
    }

    Component.onCompleted: {
        // Binding favors qml value, so set it first
        bindingControl.when = true;
    }

    HifiControls.Label {
        id: labelControl
        text: root.label
        enabled: true
        anchors.left: root.left
        anchors.right: root.horizontalCenter
        anchors.verticalCenter: root.verticalCenter
    }

    Binding {
        id: bindingControl
        target: root.color
        property: root.property
        when: false
    }
}
