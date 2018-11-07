//
//  ConfigSlider.qml
//
//  Created by Zach Pomerantz on 2/8/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 1.4 as Original
import QtQuick.Controls.Styles 1.4

import stylesUit 1.0
import controlsUit 1.0 as HifiControls


Item {
    HifiConstants { id: hifi }
    id: root

    anchors.left: parent.left
    anchors.right: parent.right    
    height: 24

    property var labelAreaWidthScale: 0.5

    property bool integral: false
    property var config
    property string property
    property alias min: sliderControl.minimumValue
    property alias max: sliderControl.maximumValue

    property alias label: labelControl.text
    property bool showLabel: true  

    property bool showValue: true  

    
       
    signal valueChanged(real value)

    Component.onCompleted: {
        // Binding favors qml value, so set it first
        sliderControl.value = root.config[root.property];
        bindingControl.when = true;
    }

    HifiControls.Label {
        id: labelControl
        text: root.label
        enabled: root.showLabel
        anchors.left: root.left
        width: root.width * root.labelAreaWidthScale
        anchors.verticalCenter: root.verticalCenter
    }
    
    Binding {
        id: bindingControl
        target: root.config
        property: root.property
        value: sliderControl.value
        when: false
    }

    HifiControls.Slider {
        id: sliderControl
        stepSize: root.integral ? 1.0 : 0.0
        anchors.left: labelControl.right
        anchors.right: root.right
        anchors.rightMargin: 0
        anchors.top: root.top
        anchors.topMargin: 0

        onValueChanged: { root.valueChanged(value) }
    }

    HifiControls.Label {
        id: labelValue
        enabled: root.showValue
        text: sliderControl.value.toFixed(root.integral ? 0 : 2)
        anchors.right: labelControl.right
        anchors.rightMargin: 5
        anchors.verticalCenter: root.verticalCenter
    }


 

}
