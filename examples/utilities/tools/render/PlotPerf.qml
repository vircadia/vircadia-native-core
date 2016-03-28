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
    width: parent.width
    height: 100
    property string title
    property var config
    property string parameters

    // THis is my hack to get the name of the first property and assign it to a trigger var in order to get
    // a signal called whenever the value changed
    property var trigger: config[parameters.split(":")[3].split("-")[0]]

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
                        value: varProps[0],
                        valueMax: 1,
                        numSamplesConstantMax: 0,
                        valueHistory: new Array(),
                        label: varProps[1],
                        color: varProps[2],
                        scale: (varProps.length > 3 ? varProps[3] : 1),
                        unit: (varProps.length > 4 ? varProps[4] : valueUnit)
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
        

        var currentValueMax = 0
        for (var i = 0; i < _values.length; i++) {

            var currentVal = config[_values[i].value] * _values[i].scale;
            _values[i].valueHistory.push(currentVal)
            _values[i].numSamplesConstantMax++;

            if (_values[i].valueHistory.length > VALUE_HISTORY_SIZE) {
                var lostValue = _values[i].valueHistory.shift();
                if (lostValue >= _values[i].valueMax) {
                    _values[i].valueMax *= 0.99
                    _values[i].numSamplesConstantMax = 0
                }
            }

            if (_values[i].valueMax < currentVal) {
                _values[i].valueMax = currentVal;
                _values[i].numSamplesConstantMax = 0 
            }                    

            if (_values[i].numSamplesConstantMax > VALUE_HISTORY_SIZE) {
                _values[i].numSamplesConstantMax = 0     
                _values[i].valueMax *= 0.95 // lower slowly the current max if no new above max since a while                      
            }
 
            if (currentValueMax < _values[i].valueMax) {
                currentValueMax = _values[i].valueMax
            }
        }

        if ((valueMax < currentValueMax) || (tick % VALUE_HISTORY_SIZE == 0)) {
            valueMax = currentValueMax;
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

            function displayValue(val, unit) {
                 return (val / root.valueScale).toFixed(root.valueNumDigits) + " " + unit
            }

            function pixelFromVal(val, valScale) {
                return lineHeight + (height - lineHeight) * (1 - (0.9) * val / valueMax);
            }
            function valueFromPixel(pixY) {
                return ((pixY - lineHeight) / (height - lineHeight) - 1) * valueMax / (-0.9);
            }
            function plotValueHistory(ctx, valHistory, color) {
                var widthStep= width / (valHistory.length - 1);

                ctx.beginPath();
                ctx.strokeStyle= color; // Green path
                ctx.lineWidth="2";
                ctx.moveTo(0, pixelFromVal(valHistory[0])); 
                   
                for (var i = 1; i < valHistory.length; i++) { 
                    ctx.lineTo(i * widthStep, pixelFromVal(valHistory[i])); 
                }

                ctx.stroke();
            }
            function displayValueLegend(ctx, val, num) {
                ctx.fillStyle = val.color;
                var bestValue = val.valueHistory[val.valueHistory.length -1];             
                ctx.textAlign = "right";
                ctx.fillText(displayValue(bestValue, val.unit), width, (num + 2) * lineHeight * 1.5);
                ctx.textAlign = "left";
                ctx.fillText(val.label, 0, (num + 2) * lineHeight * 1.5);
            }

            function displayTitle(ctx, text, maxVal) {
                ctx.fillStyle = "grey";
                ctx.textAlign = "right";
                ctx.fillText(displayValue(valueFromPixel(lineHeight), root.valueUnit), width, lineHeight);
                
                ctx.fillStyle = "white";
                ctx.textAlign = "left";
                ctx.fillText(text, 0, lineHeight);
            }
            function displayBackground(ctx) {
                ctx.fillStyle = Qt.rgba(0, 0, 0, 0.6);
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
            
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            ctx.font="12px Verdana";

            displayBackground(ctx);
                
            for (var i = 0; i < _values.length; i++) {
                plotValueHistory(ctx, _values[i].valueHistory, _values[i].color)
                displayValueLegend(ctx, _values[i], i)
            }

            displayTitle(ctx, title, valueMax)
        }
    }
}
