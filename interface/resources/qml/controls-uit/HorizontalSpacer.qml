//
//  HorizontalSpacer.qml
//
//  Created by Howard Stearns on 12 July 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

import "../styles-uit"

Item {
    HifiConstants { id: hifi }
    height: 1  // Must be non-zero
    width: hifi.dimensions.controlInterlineHeight
}
