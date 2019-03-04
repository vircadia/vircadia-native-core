//
//  PropSplitter.qml
//
//  Created by Sam Gateau on 3/2/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7

Item {
    id: root
    property real size

    width: size  // Must be non-zero
    height: size

    anchors.verticalCenter: parent.verticalCenter
}
