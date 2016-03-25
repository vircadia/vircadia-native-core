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
        spacing: 4

        id: stats
        property var config: Render.getConfig("Stats")

        PlotPerf {
            config: stats.config
            parameters: "1::0:num Textures-numTextures-blue:num GPU Textures-numGPUTextures-green"
        }
        PlotPerf {
            config: stats.config
            parameters: "1048576:Mb:1:Sysmem-textureSysmemUsage-blue:Vidmem-textureVidmemUsage-green"
        }
    }
}
