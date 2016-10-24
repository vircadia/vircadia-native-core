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
    property bool showGraphs: false

    MovingValue {
        label: "Jitter"
        color: "red"
        source: max - avg
        showGraphs: jitter.showGraphs
    }
    Value {
        label: "Average"
        source: avg
    }
}

