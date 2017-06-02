//
//  Created by Dante Ruiz on 6/1/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtGraphicalEffects 1.0

Item {
    Rectangle {
        id: shadows 
        width: parent.width
        height: 2
        color: hifi.colors.baseGrayShadow
    }
    
    Rectangle {
        width: parent.width
        height: 1
        anchors.top: shadows.bottom
        color: hifi.colors.baseGrayHighlight
    }
}
