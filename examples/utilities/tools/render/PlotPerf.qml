//
//  PlotPerf.qml
//  examples/utilities/tools/render
//
//  Created by Sam Gateau on 3//2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4

Item {
    id: root
    height: 100
    property string title
    property var config
    property string parameters

    property var trigger: config["numTextures"]

    property var inputs: parameters.split(":")
    property var valueScale: +inputs[0]
    property var valueUnit: inputs[1]
    property var valueNumDigits: inputs[2]
    property var input_VALUE_OFFSET: 3
    property var valueMax : 1

    property var _values : new Array()
    property var tick : 0

    function createValues() {
        if (inputs.length > input_VALUE_OFFSET) {
                for (var i = input_VALUE_OFFSET; i < inputs.length; i++) {
                    var varProps = inputs[i].split("-")
                    _values.push( {
                        value: varProps[1],
                        valueMax: 1,
                        valueHistory: new Array(),
                        label: varProps[0],
                        color: varProps[2]
                    })
                }
        } 
        print("in creator" + JSON.stringify(_values));

    }

    Component.onCompleted: {
        createValues();   
        print(JSON.stringify(_values));
                    
    }

    function pullFreshValues() {
        //print("pullFreshValues");
        var VALUE_HISTORY_SIZE = 100;
        var UPDATE_CANVAS_RATE = 20;
        tick++;
        
        valueMax = 0
        for (var i = 0; i < _values.length; i++) {
            var currentVal = stats.config[_values[i].value];
            if (_values[i].valueMax < currentVal) {
                _values[i].valueMax = currentVal;
            }                    
            _values[i].valueHistory.push(currentVal)
            if (_values[i].valueHistory.length > VALUE_HISTORY_SIZE) {
                var lostValue = _values[i].valueHistory.shift();
                if (lostValue >= _values[i].valueMax) {
                    _values[i].valueMax *= 0.99
                }
            }

            if (valueMax < _values[i].valueMax) {
                valueMax = _values[i].valueMax
            }
        }


        if (tick % UPDATE_CANVAS_RATE == 0) {
            mycanvas.requestPaint()
        }
    }
    onTriggerChanged: pullFreshValues() 

    Canvas {
        id: mycanvas
        anchors.fill:parent
        onPaint: {
            var lineHeight = 12;

            function displayValue(val) {
                 return (val / root.valueScale).toFixed(root.valueNumDigits) + " " + root.valueUnit
            }

            function pixelFromVal(val) {
                return lineHeight + (height - lineHeight) * (1 - (0.9) * val / valueMax);
            }
            function plotValueHistory(ctx, valHistory, color) {
                var widthStep= width / (valHistory.length - 1);

                ctx.beginPath();
                ctx.strokeStyle= color; // Green path
                ctx.lineWidth="2";
                ctx.moveTo(0, pixelFromVal(valHistory[i])); 
                   
                for (var i = 1; i < valHistory.length; i++) { 
                    ctx.lineTo(i * widthStep, pixelFromVal(valHistory[i])); 
                }

                ctx.stroke();
            }
            function displayValueLegend(ctx, val, num) {
                ctx.fillStyle = val.color;
                var bestValue = val.valueHistory[val.valueHistory.length -1];             
                ctx.textAlign = "right";
                ctx.fillText(displayValue(bestValue), width, height - num * lineHeight);
                ctx.textAlign = "left";
                ctx.fillText(val.label, 0, height - num * lineHeight);
            }

            function displayTitle(ctx, text, maxVal) {
                ctx.fillStyle = "grey";
                ctx.textAlign = "right";
                ctx.fillText(displayValue(maxVal), width, lineHeight);
                
                ctx.fillStyle = "white";
                ctx.textAlign = "left";
                ctx.fillText(text, 0, lineHeight);
            }
            
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            ctx.fillStyle = Qt.rgba(0, 0, 0, 0.4);
            ctx.fillRect(0, 0, width, height);

            ctx.font="12px Verdana";
                
            for (var i = 0; i < _values.length; i++) {
                plotValueHistory(ctx, _values[i].valueHistory, _values[i].color)
                displayValueLegend(ctx, _values[i], i)
            }

            displayTitle(ctx, title, valueMax)
        }
    }
}
