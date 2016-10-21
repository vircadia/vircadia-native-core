//
//  MovingValue.qml
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
import "../lib/plotperf"

RowLayout {
    id: value
    property string label
    property var source
    property string unit: "ms"
    property bool showGraphs: false
    property color color: "darkslategrey"
    property int decimals: 0

    width: parent.width

    Label {
        Layout.preferredWidth: 100
        color: value.color
        text: value.label
    }
    Label {
        visible: !value.showGraphs
        Layout.preferredWidth: 50
        horizontalAlignment: Text.AlignRight
        color: value.color
        text: value.source.toFixed(decimals) + ' ' + unit
    }
    PlotPerf {
        visible: value.showGraphs
        Layout.fillWidth: true
        height: 70

        valueUnit: value.unit
        valueNumDigits: 0
        backgroundOpacity: 0.2

        plots: [{ binding: "source", color: value.color }]
    }
}

