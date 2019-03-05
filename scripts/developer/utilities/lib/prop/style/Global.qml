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

    property real lineHeight: 32
    property real slimHeight: 24  

    property real horizontalMargin: 2

    property var color: hifi.colors.baseGray
    property var colorBackHighlight: hifi.colors.baseGrayHighlight
    property var colorBorderLight: hifi.colors.lightGray
    property var colorBorderHighight: hifi.colors.blueHighlight

    property real fontSize: 12
    property var fontFamily: "Raleway"
    property var fontWeight: Font.DemiBold
    property var fontColor: hifi.colors.faintGray

    property var splitterRightWidthScale: 0.45
    property real splitterWidth: 8
    
    property var labelTextAlign: Text.AlignRight
    property var labelTextElide: Text.ElideMiddle

    property var valueAreaWidthScale: 0.3 * (1.0 - splitterRightWidthScale)
    property var valueTextAlign: Text.AlignHCenter 
    property real valueBorderWidth: 1
    property real valueBorderRadius: 2
}