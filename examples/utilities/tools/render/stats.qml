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
                "num Textures:numTextures:1",
                "Sysmem Usage:textureSysmemUsage",
                "num GPU Textures:numGPUTextures",
                "Vidmem Usage:textureVidmemUsage"
            ]
            Row {
                Label {
                    text: qsTr(modelData.split(":")[0])
                }
                property var value: stats.config[modelData.split(":")[1]] 
                property var valueHistory : new Array()
                property var valueMax : 1000
                property var tick : 0
                Label {
                    text: value
                    horizontalAlignment: AlignRight
                }
                Canvas {
                    id: mycanvas
                    width: 100
                    height: 200
                    onPaint: {
                        tick++;
                        valueHistory.push(value)
                        if (valueHistory.length > 100) {
                            valueHistory.shift();
                        }
                        var ctx = getContext("2d");
                        if (tick % 2) {
                            ctx.fillStyle = Qt.rgba(0, 1, 0, 0.5);
                        } else {
                            ctx.fillStyle = Qt.rgba(0, 0, 0, 0.5);
                        }
                        ctx.fillRect(0, 0, width, height);
                        var widthStep= width / valueHistory.length;
                        ctx.lineWidth="5";
                        ctx.beginPath();
                        ctx.strokeStyle="green"; // Green path
                        ctx.moveTo(0,height);

                        for (var i = 0; i < valueHistory.length; i++) { 
                            ctx.lineTo(i * widthStep, height * (1 - valueHistory[i] / valueMax) ); 
                        }
                        ctx.lineTo(width, height); 
                        ctx.stroke(); // Draw it           
                    }
                }
            }
        }
    }
}
