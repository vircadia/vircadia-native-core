//
//  stats.qml
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

Column {
    width: parent.width
    height: parent.height

    Section {
        label: "Latency"
        description: "Audio pipeline latency, broken out and summed"
        control: ColumnLayout {
            MovingValue { label: "Input Read"; source: AudioStats.inputReadMsMax }
            MovingValue { label: "Input Ring"; source: AudioStats.inputUnplayedMsMax }
            MovingValue { label: "Network (client->mixer)"; source: AudioStats.pingMs / 2 }
            MovingValue { label: "Mixer Ring"; source: AudioStats.mixerStream.unplayedMsMax }
            MovingValue { label: "Network (mixer->client)"; source: AudioStats.pingMs / 2 }
            MovingValue { label: "Output Ring"; source: AudioStats.clientStream.unplayedMsMax }
            MovingValue { label: "Output Read"; source: AudioStats.outputUnplayedMsMax }
            MovingValue { label: "TOTAL"
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
            max: AudioStats.sentTimegapMsMax
            avg: AudioStats.sentTimegapMsAvg
            maxWindow: AudioStats.sentTimegapMsMaxWindow
            avgWindow: AudioStats.sentTimegapMsAvgWindow
        }
    }

    Section {
        label: "Mixer (upstream)"
        description: "This client's remote audio stream, as seen by the server's mixer"
        control: Stream { stream: AudioStats.mixerStream }
    }

    Section {
        label: "Client (downstream)"
        description: "This client's received audio stream, between the network and the OS"
        control: Stream { stream: AudioStats.clientStream }
    }
}

