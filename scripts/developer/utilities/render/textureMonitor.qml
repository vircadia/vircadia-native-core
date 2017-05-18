//
//  texture monitor.qml
//  examples/utilities/render
//
//  Created by Sam Gateau on 5/17/2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import "../lib/plotperf"


Item {
    id: texMex
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
                    prop: "texturePendingGPUTransferCount",
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
                    prop: "textureCPUMemSize",
                    label: "CPU",
                    color: "#00B4EF"
                },
                {
                    prop: "textureGPUMemSize",
                    label: "GPU",
                    color: "#1AC567"
                },
                {
                    prop: "textureTransferPendingSize",
                    label: "Pending Transfer",
                    color: "#9495FF"
                },
                {
                    prop: "textureResourceGPUMemSize",
                    label: "Resource",
                    color: "#1FC6A6"
                },
                {
                    prop: "textureResidentGPUMemSize",
                    label: "Resident",
                    color: "#FF6309"
                },
                {
                    prop: "textureFramebufferGPUMemSize",
                    label: "Framebuffer",
                    color: "#EF93D1"
                }
            ]
        }
    }

}