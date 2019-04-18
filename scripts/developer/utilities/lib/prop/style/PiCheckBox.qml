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
import controlsUit 1.0 as HifiControls

HifiControls.CheckBox {
    Global { id: global }
    
    color: global.fontColor

    //anchors.left: root.splitter.right
    //anchors.verticalCenter: root.verticalCenter
    //width: root.width * global.valueAreaWidthScale
    height: global.slimHeight

    onCheckedChanged: { root.valueVarSetter(checked); }
}