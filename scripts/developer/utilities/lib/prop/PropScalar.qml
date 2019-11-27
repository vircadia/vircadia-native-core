//
//  PropScalar.qml
//
//  Created by Sam Gateau on 3/2/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7

import controlsUit 1.0 as HifiControls

PropItem {
    Global { id: global }
    id: root

    // Scalar Prop
    property bool integral: false
    property var numDigits: 2


    property alias min: sliderControl.minimumValue
    property alias max: sliderControl.maximumValue

    property bool showValue: true  
    
    Component.onCompleted: {
    }  

    property var sourceValueVar: root.valueVarGetter()

    function applyValueVarFromWidgets(value) {
        root.valueVarSetter(value)
    }

    PropLabel {
        id: valueLabel
        enabled: root.showValue

        anchors.left: root.splitter.right
        anchors.right: (root.readOnly ? root.right : sliderControl.left)
        anchors.verticalCenter: root.verticalCenter
        horizontalAlignment: global.valueTextAlign
        height: global.slimHeight
        
        text: root.sourceValueVar.toFixed(root.integral ? 0 : root.numDigits)

        background: Rectangle {
            color: global.color
            border.color: global.colorBorderLight
            border.width: global.valueBorderWidth
            radius: global.valueBorderRadius
        }

        MouseArea{
            id: mousearea
            enabled: !root.readOnly
            anchors.fill: parent
            onDoubleClicked: { sliderControl.visible = !sliderControl.visible }
        }
    }

    HifiControls.Slider {
        id: sliderControl
        visible: !root.readOnly

        stepSize: root.integral ? 1.0 : 0.0
        value: root.sourceValueVar
        onValueChanged: { applyValueVarFromWidgets(value) }

        width: root.width * (root.readOnly ? 0.0 : global.handleAreaWidthScale)
        anchors.right: root.right
        anchors.verticalCenter: root.verticalCnter
    }

    
}
