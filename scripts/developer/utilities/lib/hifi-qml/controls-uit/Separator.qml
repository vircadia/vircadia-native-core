//
//  Separator.qml
//
//  Created by Zach Fox on 2017-06-06
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import "../styles-uit"

Item {
    // Size
    height: 2;
    Rectangle {
        // Size
        width: parent.width;
        height: 1;
        // Anchors
        anchors.left: parent.left;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: height;
        // Style
        color: hifi.colors.baseGrayShadow;
    }
    Rectangle {
        // Size
        width: parent.width;
        height: 1;
        // Anchors
        anchors.left: parent.left;
        anchors.bottom: parent.bottom;
        // Style
        color: hifi.colors.baseGrayHighlight;
    }
}
