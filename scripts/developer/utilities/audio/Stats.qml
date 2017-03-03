//
//  Stats.qml
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

import "../../../../resources/qml/controls-uit" as HifiControls

Column {
    id: stats
    width: parent.width
    property bool showGraphs: toggleGraphs.checked

    Item {
        width: parent.width
        height: 30

        HifiControls.Button {
            id: toggleGraphs
            property bool checked: false
            anchors.horizontalCenter: parent.horizontalCenter
            text: checked ? "Hide graphs" : "Show graphs"
            onClicked: function() { checked = !checked; }
        }
    }

    Grid {
        width: parent.width

        Column {
            width: parent.width / 2

            Section {
                label: "Latency"
                description: "Audio pipeline latency, broken out and summed"
                control: ColumnLayout {
                    MovingValue { label: "Input Read"; source: AudioStats.inputReadMsMax; showGraphs: stats.showGraphs }
                    MovingValue { label: "Input Ring"; source: AudioStats.inputUnplayedMsMax; showGraphs: stats.showGraphs }
                    MovingValue { label: "Network (up)"; source: AudioStats.pingMs / 2; showGraphs: stats.showGraphs; decimals: 1 }
                    MovingValue { label: "Mixer Ring"; source: AudioStats.mixerStream.unplayedMsMax; showGraphs: stats.showGraphs }
                    MovingValue { label: "Network (down)"; source: AudioStats.pingMs / 2; showGraphs: stats.showGraphs; decimals: 1 }
                    MovingValue { label: "Output Ring"; source: AudioStats.clientStream.unplayedMsMax; showGraphs: stats.showGraphs }
                    MovingValue { label: "Output Read"; source: AudioStats.outputUnplayedMsMax; showGraphs: stats.showGraphs }
                    MovingValue { label: "TOTAL"; color: "black"; showGraphs: stats.showGraphs
                        source: AudioStats.inputReadMsMax +
                            AudioStats.inputUnplayedMsMax +
                            AudioStats.outputUnplayedMsMax +
                            AudioStats.mixerStream.unplayedMsMax +
                            AudioStats.clientStream.unplayedMsMax +
                            AudioStats.pingMs
                    }
                }
            }

            Section {
                label: "Upstream Jitter"
                description: "Timegaps in packets sent to the mixer"
                control: Jitter {
                    max: AudioStats.sentTimegapMsMaxWindow
                    avg: AudioStats.sentTimegapMsAvgWindow
                    showGraphs: stats.showGraphs
                }
            }
        }

        Column {
            width: parent.width / 2

            Section {
                label: "Mixer (upstream)"
                description: "This client's remote audio stream, as seen by the server's mixer"
                control: Stream { stream: AudioStats.mixerStream; showGraphs: stats.showGraphs }
            }

            Section {
                label: "Client (downstream)"
                description: "This client's received audio stream, between the network and the OS"
                control: Stream { stream: AudioStats.clientStream; showGraphs: stats.showGraphs }
            }
        }
    }
}
