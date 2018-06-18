//
//  jet/TaskTimeFrameView.qml
//
//  Created by Sam Gateau, 2018/06/15
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 1.4 as Original
import QtQuick.Controls.Styles 1.4

import "qrc:///qml/styles-uit"
import "qrc:///qml/controls-uit" as HifiControls

import "../jet.js" as Jet

Rectangle {
    HifiConstants { id: hifi;}
    color: hifi.colors.baseGray;
    id: root;
    
    property var rootConfig : Workload
    
    property var jobsTree
    property var jobsArray

    Component.onCompleted: {
        if (!jobsTree) { jobsTree = new Array(); }
        if (!jobsArray) { jobsArray = new Array(); }

        var tfunctor = Jet.job_tree_model_array_functor(jobsTree, function(node) {
            var job = { "fullpath": (node.path + "." + node.name), "cpuT": 0.0, "depth": node.level }
            jobsArray.push(job)
        })
        Jet.task_traverseTree(rootConfig, tfunctor);
     
         for (var j = 0; j <jobsArray.length; j++) {
            jobsArray[j].cpuT = Render.getConfig(jobsArray[j].fullpath).cpuRunTime
            print("job" + j + ": " + jobsArray[j].cpuT)
        }
    }

   


    Timer {
        interval: 500; running: true; repeat: true
        onTriggered: pullFreshValues()
    }

    function pullFreshValues() {

        for (var j = 0; j <jobsArray.length; j++) {
            jobsArray[j].cpuT = root.rootConfig.getConfig(jobsArray[j].fullpath).cpuRunTime
        }
        mycanvas.requestPaint()
    }
    Canvas {
        id: mycanvas
        anchors.fill:parent
        
        onPaint: {
            print("mycanvasOnPaint " + jobsArray.length)
            var lineHeight = 12;
            var frameWidth = width;

  
            function displayBackground(ctx) {
                ctx.fillStyle = Qt.rgba(0, 0, 0, root.backgroundOpacity);
                ctx.fillRect(0, 0, width, height);
                
                ctx.strokeStyle= "grey";
                ctx.lineWidth="2";

                ctx.beginPath();
                ctx.moveTo(0, lineHeight + 1); 
                ctx.lineTo(width, lineHeight + 1); 
                ctx.moveTo(0, height); 
                ctx.lineTo(width, height); 
                ctx.stroke();
            }

            function drawJob(ctx, depth, index, duration, timeOffset, timeScale) {
               //print(root.jobsArray[index].cpuT)
              //  ctx.fillStyle = Qt.rgba(255, 255, 0, root.backgroundOpacity);
                ctx.fillStyle = ( depth % 2 ? ( index % 2 ? "blue" : "yellow") : ( index % 2 ? "green" : "red"))
                ctx.fillRect(timeOffset * timeScale, lineHeight * depth, duration * timeScale, lineHeight * 0.6);
               
            }

            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            ctx.font="12px Verdana";

            displayBackground(ctx); 
            if (jobsArray.length > 0) {
            
                var rangeStack =new Array()
                var frameDuration = Math.max(jobsArray[0].cpuT, 1)
                rangeStack.push( { "b": 0.0, "e": frameDuration } )
                var timeScale = width * 0.9 / frameDuration;

                drawJob(ctx, 0, 0, jobsArray[0].cpuT, 0, timeScale)
                
                for (var i = 1; i <jobsArray.length; i++) {
                //for (var i = 1; i <20; i++) {
                    var lastDepth = rangeStack.length - 1;
                    var depth = jobsArray[i].depth;
                    var timeOffset = 0.0
                    var duration = jobsArray[i].cpuT;
                    if (depth < 4) {

                    if (depth > lastDepth) {                        
                        timeOffset = rangeStack[lastDepth].b
                        while(rangeStack.length <= depth) {
                            rangeStack.push( { "b": timeOffset, "e": duration } )
                        }

                    } else {
                        if (depth < lastDepth) {
                            while(rangeStack.length != (depth + 1)) {
                                rangeStack.pop()
                            }
                        }

                        timeOffset = rangeStack[depth].e
                        rangeStack[depth].b = timeOffset
                        rangeStack[depth].e = timeOffset + duration 
                    }
                    
                    print("j " + i + " depth " + depth + " lastDepth " + lastDepth + " off " + timeOffset + " dur " + duration)
                    drawJob(ctx, depth, i, duration, timeOffset, timeScale) 
                    }
                }
            }
        }
    }
}