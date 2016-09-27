//
//  Stream.qml
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
    id: root
    property var stream 
    property bool showGraphs: false

    Label {
        Layout.alignment: Qt.AlignCenter
        text: "Ring Buffer"
        font.italic: true
    }
    MovingValue {
        label: "Desired"
        source: stream.framesDesired
        unit: "frames"
        showGraphs: root.showGraphs
    }
    MovingValue {
        label: "Unplayed"
        source: stream.unplayedMsMax
        showGraphs: root.showGraphs
    }
    Value {
        label: "Available (avg)"
        source: stream.framesAvailable + " (" + stream.framesAvailableAvg + ") frames"
    }

    Label {
        Layout.alignment: Qt.AlignCenter
        text: "Jitter"
        font.italic: true
    }
    Jitter {
        max: stream.timegapMsMax
        avg: stream.timegapMsAvg
        maxWindow: stream.timegapMsMaxWindow
        avgWindow: stream.timegapMsAvgWindow
        showGraphs: root.showGraphs
    }

    Label {
        Layout.alignment: Qt.AlignCenter
        text: "Packet Loss"
        font.italic: true
    }
    Value {
        label: "Overall"
        source: stream.lossRate.toFixed(2) + "% (" + stream.lossCount + " lost)"
    }
    Value {
        label: "Window"
        source: stream.lossRateWindow.toFixed(2) + "% (" + stream.lossCountWindow + " lost)"
    }
}

