//
//  stats.qml
//  examples/utilities/cache
//
//  Created by Zach Pomerantz on 4/1/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import "../lib/plotperf"

Item {
    id: root 
    anchors.fill: parent

    property var caches: [ ["Present", "present"], ["Present", "present"], ["New", "newFrame"], ["Dropped", "dropped"], ["Simulation", "simulation"], ["Avatar", "avatar"] ]
    property var colors: [ "#1AC567", "#00B4EF" ]

    Grid {
        id: grid
        rows: (root.caches.length / 2); columns: 2; spacing: 8
        anchors.fill: parent
 
        Repeater {
            id: repeater

            model: root.caches

            Row {
                PlotPerf {
                    title: modelData[0] + " Rate"
                    height: (grid.height - (grid.spacing * ((root.caches.length / 2) + 1))) / (root.caches.length / 2)
                    width: grid.width / 2 - grid.spacing * 1.5
                    object: Rates
                    valueScale: 1
                    valueUnit: "fps"
                    valueNumDigits: "2"
                    plots: [{
                        prop: modelData[1],
                        color: root.colors[index % 2]
                    }]
                }
            }
        }
    }
}
