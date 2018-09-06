//
//  RichSlider.qml
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

import "qrc:///qml/styles-uit"
import "qrc:///qml/controls-uit" as HifiControls


Item {
    HifiConstants { id: hifi }
    id: root

    anchors.left: parent.left
    anchors.right: parent.right    
    height: 24

    function defaultGet() { return 0 }
    function defaultSet(value) { }

    property var labelAreaWidthScale: 0.5
    property bool integral: false
    property var numDigits: 2

    property var valueVarSetter: defaultSet
    property alias valueVar : sliderControl.value

    property alias min: sliderControl.minimumValue
    property alias max: sliderControl.maximumValue

    property alias label: labelControl.text
    property bool showLabel: true  

    property bool showValue: true  

    
       
    signal valueChanged(real value)

    Component.onCompleted: {
    }

    HifiControls.Label {
        id: labelControl
        text: root.label
        enabled: root.showLabel
        anchors.left: root.left
        width: root.width * root.labelAreaWidthScale
        anchors.verticalCenter: root.verticalCenter
    }

    HifiControls.Slider {
        id: sliderControl
        stepSize: root.integral ? 1.0 : 0.0
        anchors.left: labelControl.right
        anchors.right: root.right
        anchors.rightMargin: 0
        anchors.top: root.top
        anchors.topMargin: 0
        onValueChanged: { root.valueVarSetter(value) }
    }

    HifiControls.Label {
        id: labelValue
        enabled: root.showValue
        text: sliderControl.value.toFixed(root.integral ? 0 : root.numDigits)
        anchors.right: labelControl.right
        anchors.rightMargin: 5
        anchors.verticalCenter: root.verticalCenter
    }

}
