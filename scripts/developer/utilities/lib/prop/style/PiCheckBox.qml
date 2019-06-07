//
//  Prop/style/PiCheckBox.qml
//
//  Created by Sam Gateau on 3/2/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 2.2

CheckBox {
    Global { id: global }
    id: control
    text: ""
    checked: true
    spacing: 0

    indicator: Rectangle {
        color: global.colorBack  
        border.color: control.down ? global.colorBorderLighter : global.colorBorderLight
        border.width: global.valueBorderWidth
        radius: global.valueBorderRadius / 2

         anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter   
        implicitWidth: global.iconWidth
        implicitHeight: global.iconWidth

         Rectangle {
            visible: control.checked

             color: global.colorBorderHighight           
            radius: global.valueBorderRadius / 4

             anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter

             implicitWidth: global.iconWidth - 2
            implicitHeight: global.iconHeight - 2
        }      
    }

    contentItem: PiText {
        text: control.text
        anchors.verticalCenter: parent.verticalCenter
        horizontalAlignment: Text.AlignHLeft
        anchors.left: control.indicator.right
        leftPadding: global.horizontalMargin
    }    
    
    height: global.slimHeight
}