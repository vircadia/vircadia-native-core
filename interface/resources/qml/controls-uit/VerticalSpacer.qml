//
//  VerticalSpacer.qml
//
//  Created by David Rowe on 16 Feb 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

import "../styles-uit"

Item {
    id: root
    property alias size: root.height

    width: 1  // Must be non-zero
    height: hifi.dimensions.controlInterlineHeight
}
