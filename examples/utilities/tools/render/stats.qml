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
    id: statsUI
    width: 300 
    spacing: 8
    Column {
        spacing: 8

        property var config: Render.getConfig("Stats")
        id: stats

        PlotPerf {
            title: "Num Buffers"
            width:statsUI.width
            config: stats.config
            parameters: "1::0:CPU-numBuffers-#00B4EF:GPU-numGPUBuffers-#1AC567"
        }
        PlotPerf {
            title: "Memory Usage"
            width:statsUI.width       
            config: stats.config
            parameters: "1048576:Mb:1:CPU-bufferSysmemUsage-#00B4EF:GPU-bufferVidmemUsage-#1AC567"
        }

        PlotPerf {
            title: "Num Textures"
            width:statsUI.width
            config: stats.config
            parameters: "1::0:CPU-numTextures-#00B4EF:GPU-numGPUTextures-#1AC567:Frame-numFrameTextures-#E2334D"
        }
        PlotPerf {
            title: "Memory Usage"
            width:statsUI.width       
            config: stats.config
            parameters: "1048576:Mb:1:CPU-textureSysmemUsage-#00B4EF:GPU-textureVidmemUsage-#1AC567"
        }
    }
}
