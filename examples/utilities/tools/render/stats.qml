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


Item {
    id: statsUI
    anchors.fill:parent

    Column {
        id: stats
        spacing: 8
        anchors.fill:parent
 
        property var config: Render.getConfig("Stats")

        function evalEvenHeight() {
            // Why do we have to do that manually ? cannot seem to find a qml / anchor / layout mode that does that ?
            return (height - spacing * (children.length - 1)) / children.length
        }

        PlotPerf {
            title: "Num Buffers"
            config: stats.config
            height: parent.evalEvenHeight()
            parameters: "1::0:bufferCPUCount-CPU-#00B4EF:bufferGPUCount-GPU-#1AC567"
        }
        PlotPerf {
            title: "gpu::Buffer Memory"
            config: stats.config          
            height: parent.evalEvenHeight()
            parameters: "1048576:Mb:1:bufferCPUMemoryUsage-CPU-#00B4EF:bufferGPUMemoryUsage-GPU-#1AC567"
        }

        PlotPerf {
            title: "Num Textures"
            config: stats.config
            height: parent.evalEvenHeight()
            parameters: "1::0:textureCPUCount-CPU-#00B4EF:textureGPUCount-GPU-#1AC567:frameTextureCount-Frame-#E2334D"
        }
        PlotPerf {
            title: "gpu::Texture Memory"
            config: stats.config
            height: parent.evalEvenHeight()
            parameters: "1048576:Mb:1:textureCPUMemoryUsage-CPU-#00B4EF:textureGPUMemoryUsage-GPU-#1AC567"
        }
        PlotPerf {
            title: "Drawcalls"
            config: stats.config
            height: parent.evalEvenHeight()
            parameters: "1::0:frameDrawcallCount-frame-#E2334D:frameDrawcallRate-rate-#1AC567-0.001-K/s"
        }
        PlotPerf {
            title: "Triangles"
            config: stats.config
            height: parent.evalEvenHeight()
            parameters: "1000:K:0:frameTriangleCount-frame-#E2334D:frameTriangleRate-rate-#1AC567-0.001-MT/s"
        }
    }
}
