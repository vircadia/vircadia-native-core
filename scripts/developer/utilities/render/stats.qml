//
//  stats.qml
//  examples/utilities/render
//
//  Created by Zach Pomerantz on 2/8/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import "../lib/plotperf"

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
            height: parent.evalEvenHeight()
            object: stats.config
            plots: [
                {
                    prop: "bufferCPUCount",
                    label: "CPU",
                    color: "#00B4EF"
                },
                {
                    prop: "bufferGPUCount",
                    label: "GPU",
                    color: "#1AC567"
                }
            ]
        }
        PlotPerf {
            title: "gpu::Buffer Memory"
            height: parent.evalEvenHeight()
            object: stats.config
            valueScale: 1048576
            valueUnit: "Mb"
            valueNumDigits: "1"
            plots: [
                {
                    prop: "bufferCPUMemoryUsage",
                    label: "CPU",
                    color: "#00B4EF"
                },
                {
                    prop: "bufferGPUMemoryUsage",
                    label: "GPU",
                    color: "#1AC567"
                }
            ]
        }
        PlotPerf {
            title: "Num Textures"
            height: parent.evalEvenHeight()
            object: stats.config
            plots: [
                {
                    prop: "textureCPUCount",
                    label: "CPU",
                    color: "#00B4EF"
                },
                {
                    prop: "textureGPUCount",
                    label: "GPU",
                    color: "#1AC567"
                },
                {
                    prop: "textureGPUTransferCount",
                    label: "Transfer",
                    color: "#9495FF"
                }
            ]
        }
        PlotPerf {
            title: "gpu::Texture Memory"
            height: parent.evalEvenHeight()
            object: stats.config
            valueScale: 1048576
            valueUnit: "Mb"
            valueNumDigits: "1"
            plots: [
                {
                    prop: "textureCPUMemoryUsage",
                    label: "CPU",
                    color: "#00B4EF"
                },
                {
                    prop: "textureGPUMemoryUsage",
                    label: "GPU",
                    color: "#1AC567"
                },
                {
                    prop: "textureGPUVirtualMemoryUsage",
                    label: "GPU Virtual",
                    color: "#9495FF"
                }
            ]
        }

        PlotPerf {
            title: "Triangles"
            height: parent.evalEvenHeight()
            object: stats.config
            valueScale: 1000
            valueUnit: "K"
            plots: [
                {
                    prop: "frameTriangleCount",
                    label: "Triangles",
                    color: "#1AC567"
                },
                {
                    prop: "frameTriangleRate",
                    label: "rate",
                    color: "#E2334D",
                    scale: 0.001,
                    unit: "MT/s"
                }
            ]
        }
        PlotPerf {
            title: "Drawcalls"
            height: parent.evalEvenHeight()
            object: stats.config
            plots: [
                {
                    prop: "frameAPIDrawcallCount",
                    label: "API Drawcalls",
                    color: "#00B4EF"
                },
                {
                    prop: "frameDrawcallCount",
                    label: "GPU Drawcalls",
                    color: "#1AC567"
                },
                {
                    prop: "frameDrawcallRate",
                    label: "rate",
                    color: "#E2334D",
                    scale: 0.001,
                    unit: "K/s"
                }
            ]
        }
 
        PlotPerf {
            title: "State Changes"
            height: parent.evalEvenHeight()
            object: stats.config
            plots: [
                {
                    prop: "frameTextureCount",
                    label: "Textures",
                    color: "#00B4EF"
                },
                {
                    prop: "frameSetPipelineCount",
                    label: "Pipelines",
                    color: "#E2334D"
                },
                {
                    prop: "frameSetInputFormatCount",
                    label: "Input Formats",
                    color: "#1AC567"
                }
            ]
        }  

        property var drawOpaqueConfig: Render.getConfig("DrawOpaqueDeferred")
        property var drawTransparentConfig: Render.getConfig("DrawTransparentDeferred")
        property var drawLightConfig: Render.getConfig("DrawLight")

        PlotPerf {
            title: "Items"
            height: parent.evalEvenHeight()
            object: parent.drawOpaqueConfig

            plots: [
                {
                    object: parent.drawOpaqueConfig,
                    prop: "numDrawn",
                    label: "Opaques",
                    color: "#1AC567"
                },
                {
                    object: Render.getConfig("DrawTransparentDeferred"),
                    prop: "numDrawn",
                    label: "Translucents",
                    color: "#00B4EF"
                },
                {
                    object: Render.getConfig("DrawLight"),
                    prop: "numDrawn",
                    label: "Lights",
                    color: "#FED959"
                }
            ]
        } 

        PlotPerf {
           title: "Timing"
           height: parent.evalEvenHeight()
           object: parent.drawOpaqueConfig
           valueUnit: "ms"
           valueScale: 1
           valueNumDigits: "2"
           plots: [
               {
                   object: Render.getConfig("DrawOpaqueDeferred"),
                   prop: "cpuRunTime",
                   label: "Opaques",
                   color: "#1AC567"
               },
               {
                   object: Render.getConfig("DrawTransparentDeferred"),
                   prop: "cpuRunTime",
                   label: "Translucents",
                   color: "#00B4EF"
               },
               {
                   object: Render.getConfig("RenderDeferred"),
                   prop: "cpuRunTime",
                   label: "Lighting",
                   color: "#FED959"
               },
               {
                   object: Render.getConfig("RenderDeferredTask"),
                   prop: "cpuRunTime",
                   label: "RenderFrame",
                   color: "#E2334D"
               }
           ]
        }
    }

}

