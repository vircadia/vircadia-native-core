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
 
        property var mainViewTask: Render.getConfig("RenderMainView");
        property var config: mainViewTask.getConfig("Stats")
 
        function evalEvenHeight() {
            // Why do we have to do that manually ? cannot seem to find a qml / anchor / layout mode that does that ?
            return (height - spacing * (children.length - 1)) / children.length
        }

      
        PlotPerf {
           title: "GPU Timing"
           height: parent.evalEvenHeight()
           object: parent.drawOpaqueConfig
           valueUnit: "ms"
           valueScale: 1
           valueNumDigits: "4"
           plots: [
            {
                   object: mainViewTask.getConfig("OpaqueRangeTimer"),
                   prop: "gpuRunTime",
                   label: "Opaque",
                   color: "#FFFFFF"
               }, 
               {
                   object: mainViewTask.getConfig("LinearDepth"),
                   prop: "gpuRunTime",
                   label: "LinearDepth",
                   color: "#00FF00"
               },{
                   object: mainViewTask.getConfig("SurfaceGeometry"),
                   prop: "gpuRunTime",
                   label: "SurfaceGeometry",
                   color: "#00FFFF"
               },
               {
                   object: mainViewTask.getConfig("RenderDeferred"),
                   prop: "gpuRunTime",
                   label: "DeferredLighting",
                   color: "#FF00FF"
               }
               ,
               {
                   object: mainViewTask.getConfig("ToneAndPostRangeTimer"),
                   prop: "gpuRunTime",
                   label: "tone and post",
                   color: "#FF0000"
               }
           ]
        }
        PlotPerf {
           title: "Batch Timing"
           height: parent.evalEvenHeight()
           object: parent.drawOpaqueConfig
           valueUnit: "ms"
           valueScale: 1
           valueNumDigits: "3"
           plots: [
            {
                   object: mainViewTask.getConfig("OpaqueRangeTimer"),
                   prop: "batchRunTime",
                   label: "Opaque",
                   color: "#FFFFFF"
               }, 
               {
                   object: mainViewTask.getConfig("LinearDepth"),
                   prop: "batchRunTime",
                   label: "LinearDepth",
                   color: "#00FF00"
               },{
                   object: mainViewTask.getConfig("SurfaceGeometry"),
                   prop: "batchRunTime",
                   label: "SurfaceGeometry",
                   color: "#00FFFF"
               },
               {
                   object: mainViewTask.getConfig("RenderDeferred"),
                   prop: "batchRunTime",
                   label: "DeferredLighting",
                   color: "#FF00FF"
               }
               ,
               {
                   object: mainViewTask.getConfig("ToneAndPostRangeTimer"),
                   prop: "batchRunTime",
                   label: "tone and post",
                   color: "#FF0000"
               }
           ]
        }
    }

}

