//
//  Jitter.qml
//  scripts/developer/utilities/audio
//
//  Created by Zach Pomerantz on 9/22/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3

ColumnLayout {
    property var max
    property var avg
    property var maxWindow
    property var avgWindow

    MovingValuePair {
        label: "Total"
        label1: "Average"
        label2: "Maximum"
        source1: avg
        source2: max
    }
    MovingValuePair {
        label: "Window"
        label1: "Average"
        label2: "Maximum"
        source1: avgWindow
        source2: maxWindow
    }
}

