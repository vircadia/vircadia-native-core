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

    property var caches: [["Animation", AnimationCache], ["Model", ModelCache], ["Texture", TextureCache], ["Sound", SoundCache]]

    Grid {
        id: grid
        rows: root.caches.length; columns: 1; spacing: 8
        anchors.fill: parent
 
        Repeater {
            id: repeater

            model: root.caches

            Row {
            PlotPerf {
                title: modelData[0] + " Count"
                anchors.left: parent
                height: (grid.height - (grid.spacing * (root.caches.length + 1))) / root.caches.length
                width: grid.width / 2 - grid.spacing * 1.5
                object: modelData[1]
                valueNumDigits: "1"
                plots: [
                    {
                        prop: "numTotal",
                        label: "total",
                        color: "#00B4EF"
                    },
                    {
                        prop: "numCached",
                        label: "cached",
                        color: "#1AC567"
                    }
                ]
            }
            PlotPerf {
                title: modelData[0] + " Size"
                anchors.right: parent
                height: (grid.height - (grid.spacing * (root.caches.length + 1))) / root.caches.length
                width: grid.width / 2 - grid.spacing * 1.5
                object: modelData[1]
                valueScale: 1048576
                valueUnit: "Mb"
                valueNumDigits: "1"
                plots: [
                    {
                        prop: "sizeTotal",
                        label: "total",
                        color: "#00B4EF"
                    },
                    {
                        prop: "sizeCached",
                        label: "cached",
                        color: "#1AC567"
                    }
                ]
            }
            }
        }
    }
}
