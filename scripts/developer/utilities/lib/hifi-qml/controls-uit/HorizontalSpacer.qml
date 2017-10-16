//
//  HorizontalSpacer.qml
//
//  Created by Clement on 7/18/16
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

import "../styles-uit"

Item {
    id: root
    property alias size: root.width

    width: hifi.dimensions.controlInterlineHeight
    height: 1  // Must be non-zero
}
