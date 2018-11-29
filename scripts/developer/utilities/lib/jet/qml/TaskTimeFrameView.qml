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
    color: Qt.rgba(hifi.colors.baseGray.r, hifi.colors.baseGray.g, hifi.colors.baseGray.b, 0.8);
    id: root;
    
    property var rootConfig : Workload
    
    property var jobsTree
    property var jobsArray


    Component.onCompleted: {
        if (!jobsTree) { jobsTree = new Array(); }
        if (!jobsArray) { jobsArray = new Array(); }

        var tfunctor = Jet.job_tree_model_array_functor(jobsTree, function(node) {
            var job = { "fullpath": (node.path + "." + node.name), "cpuT": 0.0, "depth": node.level, "name": node.name }
            jobsArray.push(job)
        })
        Jet.task_traverseTree(rootConfig, tfunctor);
     
         for (var j = 0; j <jobsArray.length; j++) {
            jobsArray[j].cpuT = Render.getConfig(jobsArray[j].fullpath).cpuRunTime
         //   print("job" + j + ": " + jobsArray[j].cpuT)
        }
    }

    
    Component.onDestruction: {
        myCanvasTimer.stop();
        console.log("stopping timer!!!!");
    }

    Timer {
        id: myCanvasTimer
        interval: 100; running: true; repeat: true
        onTriggered: pullFreshValues()
    }

    function pullFreshValues() {

        for (var j = 0; j <jobsArray.length; j++) {
            jobsArray[j].cpuT = Render.getConfig(jobsArray[j].fullpath).cpuRunTime
           // jobsArray[j].cpuT = root.rootConfig.getConfig(jobsArray[j].fullpath).cpuRunTime 
        }
        mycanvas.requestPaint()
    }
    

    property var frameScale : 10
    Row {
        id: myHeaderRow
        anchors.top:parent.top 
        anchors.left:parent.left 
        anchors.right:parent.right 
 

        HifiControls.Button {
            id: myTimerPlayPause
            text: (myCanvasTimer.running ? "||" : "|>")
            height: 24
            width: 24
            onClicked: {
                print("list of highlight styles")
                myCanvasTimer.running = !myCanvasTimer.running
            }
        }
    }    
    Canvas {
        id: mycanvas
        anchors.top:myHeaderRow.bottom 
        anchors.bottom:parent.bottom 
        anchors.left:parent.left 
        anchors.right:parent.right
        
        property var frameDuration: 10
        property var frameViewBegin: 0
        property var frameViewRange: width

        function reset() {
            frameViewBegin = 0
            frameViewRange = width
        }

        function checkView() {
            if (frameViewBegin > width * 0.9) {
                frameViewBegin = width * 0.9
            } else if (frameViewBegin + frameViewRange < width * 0.1) {
                frameViewBegin =  width * 0.1 -frameViewRange
            }
        }

        function drag(deltaX) {  
            frameViewBegin -= deltaX
            checkView()
        }

        function pivotScale(pivotX, deltaX) {
            var newRange = frameViewRange + 2 * deltaX
            if (newRange <= 1) {
                newRange = 2;
            }
            frameViewBegin = pivotX - frameViewRange * (pivotX - frameViewBegin)  / newRange
            frameViewRange = newRange
            print( "pivot= " + pivotX + " deltaX= " + (deltaX))     
            checkView()    
        }


        onPaint: {
          //  print("mycanvasOnPaint " + jobsArray.length)
            var lineHeight = 12;

            function getXFromTime(t) {
                return (t / mycanvas.frameDuration) * mycanvas.frameViewRange  - (mycanvas.frameViewBegin)
            }
            function getWFromDuration(d) {
                return (d / mycanvas.frameDuration) * mycanvas.frameViewRange
            }
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
                
                var x0 = getXFromTime(0)
                ctx.moveTo(x0, 0); 
                ctx.lineTo(x0, height); 

                x0 = getXFromTime(5)
                ctx.moveTo(x0, 0); 
                ctx.lineTo(x0, height); 

                x0 = getXFromTime(10)
                ctx.moveTo(x0, 0); 
                ctx.lineTo(x0, height); 

                ctx.stroke();
            }

            function drawJob(ctx, depth, index, duration, timeOffset) {
               //print(root.jobsArray[index].cpuT)
                ctx.fillStyle = ( depth % 2 ? ( index % 2 ? "blue" : "yellow") : ( index % 2 ? "green" : "red"))
                ctx.fillRect(getXFromTime(timeOffset), lineHeight * 2 * depth,getWFromDuration(duration), lineHeight);

                if (depth,getWFromDuration(duration) >= width * 0.1) {
                    ctx.fillStyle = "grey";
                    ctx.textAlign = "center";
                    ctx.fillText( root.jobsArray[index].name, getXFromTime(timeOffset + duration * 0.5), lineHeight * 2 * depth);
                
                }
            }

            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            ctx.font="12px Verdana";

            displayBackground(ctx); 
            if (jobsArray.length > 0) {
                mycanvas.frameDuration = Math.max(jobsArray[0].cpuT, 1)
                var rangeStack =new Array()
                rangeStack.push( { "b": 0.0, "e": mycanvas.frameDuration } )
                 
                drawJob(ctx, 0, 0, jobsArray[0].cpuT, 0)
                
                for (var i = 1; i <jobsArray.length; i++) {
                    var lastDepth = rangeStack.length - 1;
                    var depth = jobsArray[i].depth;
                    var timeOffset = 0.0
                    var duration = jobsArray[i].cpuT;

                    if (depth > lastDepth) {                        
                        timeOffset = rangeStack[lastDepth].b
                        while(rangeStack.length <= depth) {
                            rangeStack.push( { "b": timeOffset, "e": timeOffset + duration } )
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
                    if (duration > 0.0) {
                        drawJob(ctx, depth, i, duration, timeOffset) 
                    }
                }
            }
        }
    }

    MouseArea {
        id: hitbox
        anchors.fill: mycanvas    
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        property var pivotX
        property var dragPos
        onPressed: {
            dragPos = { "x":mouse.x, "y":mouse.y }
            pivotX = mouse.x
        }
        onPositionChanged: {
            if (dragPos !== undefined) {
                var delta = mouse.x - dragPos.x
                
                if (mouse.buttons & Qt.LeftButton) {
                    mycanvas.drag(delta)
                }
                
                if (mouse.buttons & Qt.RightButton) {
                    mycanvas.pivotScale(pivotX, delta)
                }

                dragPos.x = mouse.x
                dragPos.y = mouse.y
                mycanvas.requestPaint()
            }
        }
        onReleased: {
            dragPos = undefined
        }

        onWheel: {
            mycanvas.pivotScale(wheel.x, mycanvas.frameViewRange * 0.02 * (wheel.angleDelta.y / 120.0))
            mycanvas.requestPaint()
        }

        onDoubleClicked: {
            mycanvas.reset()
            mycanvas.requestPaint()
        }
    }
}