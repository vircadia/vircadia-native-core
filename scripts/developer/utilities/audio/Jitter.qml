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
    id: jitter
    property var max
    property var avg
    property var maxWindow
    property var avgWindow
    property bool showGraphs: false

    MovingValuePair {
        label: "Window"
        label1: "Maximum"
        label2: "Average"
        source1: maxWindow
        source2: avgWindow
        color1: "red"
        color2: "dimgrey"
        showGraphs: jitter.showGraphs
    }
    MovingValuePair {
        label: "Overall"
        label1: "Maximum"
        label2: "Average"
        source1: max
        source2: avg
        color1: "firebrick"
        color2: "dimgrey"
        showGraphs: jitter.showGraphs
    }
}

