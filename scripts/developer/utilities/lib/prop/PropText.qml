//
//  Prop/Text.qml
//
//  Created by Sam Gateau on 3/2/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7

Text {
    Global { id: global }
    
    color: global.fontColor
    font.pixelSize: global.fontSize
    font.family: global.fontFamily
    font.weight: global.fontWeight
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: global.labelTextAlign
    elide: global.labelTextElide
}

