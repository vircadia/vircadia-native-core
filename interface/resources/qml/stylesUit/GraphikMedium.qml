//
//  GraphikMedium.qml
//
//  Created by Wayne Chen on 3 May 2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7

Text {
    id: root
    property real size: 32
    font.pixelSize: size
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft
    font.family: "Graphik"
    font.weight: Font.Medium
}
