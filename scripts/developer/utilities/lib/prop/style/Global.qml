//
//  Prop/style/Global.qml
//
//  Created by Sam Gateau on 3/2/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7

import stylesUit 1.0
import controlsUit 1.0 as HifiControls


Item {
    HifiConstants { id: hifi }
    id: root

    readonly property real lineHeight: 32
    readonly property real slimHeight: 24  

    readonly property real horizontalMargin: 4

    readonly property color color: hifi.colors.baseGray
    readonly property color colorBackHighlight: hifi.colors.baseGrayHighlight
    readonly property color colorBorderLight: hifi.colors.lightGray
    readonly property color colorBorderHighight: hifi.colors.blueHighlight

    readonly property color colorOrangeAccent: "#FF6309"
    readonly property color colorRedAccent: "#C62147"
    readonly property color colorGreenHighlight: "#1ac567"

    readonly property real fontSize: 12
    readonly property var fontFamily: "Raleway"
    readonly property var fontWeight: Font.DemiBold
    readonly property color fontColor: hifi.colors.faintGray

    readonly property var splitterRightWidthScale: 0.45
    readonly property real splitterWidth: 8

    readonly property real iconWidth: fontSize
    readonly property real iconHeight: fontSize
    
    readonly property var labelTextAlign: Text.AlignRight
    readonly property var labelTextElide: Text.ElideMiddle

    readonly property var valueAreaWidthScale: 0.3 * (1.0 - splitterRightWidthScale)
    readonly property var valueTextAlign: Text.AlignHCenter 
    readonly property real valueBorderWidth: 1
    readonly property real valueBorderRadius: 2
}