//
//  stats.qml
//  examples/utilities/tools/render
//
//  Created by Zach Pomerantz on 2/8/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4

Column {
    spacing: 8
    Column {
        id: stats
        property var config: Render.getConfig("Stats")

        Repeater {
            model: [
                "num Textures:numTextures",
                "Sysmem Usage:textureSysmemUsage",
                "num GPU Textures:numGPUTextures",
                "Vidmem Usage:textureVidmemUsage"
            ]
            Row {
                Label {
                    text: qsTr(modelData.split(":")[0])
                }
                property var value: stats.config[modelData.split(":")[1]]
                Label {
                    text: value
                    horizontalAlignment: AlignRight
                }
                
            }
        }
    }
}
