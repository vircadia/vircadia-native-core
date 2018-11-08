//
//  AudioScope.qml
//
//  Created by Luis Cuenca on 11/22/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit

Item {
    id: root
    width: parent.width
    height: parent.height   
    
    property var _scopeInputData 
    property var _scopeOutputLeftData
    property var _scopeOutputRightData
    
    property var _triggerInputData 
    property var _triggerOutputLeftData
    property var _triggerOutputRightData
    
    property var _triggerValues: QtObject{ 
        property int x: parent.width/2 
        property int y: parent.height/3
    }
    
    property var _triggered: false
    property var _steps
    property var _refreshMs: 32
    property var _framesPerSecond: AudioScope.getFramesPerSecond()
    property var _isFrameUnits: true
    
    property var _holdStart: QtObject{ 
        property int x: 0
        property int y: 0
    }
    
    property var _holdEnd: QtObject{ 
        property int x: 0 
        property int y: 0
    }
    
    property var _timeBeforeHold: 300
    property var _pressedTime: 0
    property var _isPressed: false

    property var _recOpacity : 0.0
    property var _recSign : 0.05

    property var _outputLeftState: false
    property var _outputRightState: false
    
    property var _wavFilePath: ""
    
    function isHolding() {
        return (_pressedTime > _timeBeforeHold); 
    }
    
    function updateMeasureUnits() {
        timeButton.text = _isFrameUnits ? "Display Frames" : "Milliseconds";
        fiveLabel.text = _isFrameUnits ? "5" : "" + (Math.round(1000 * 5.0/_framesPerSecond));
        twentyLabel.text = _isFrameUnits ? "20" : "" + (Math.round(1000 * 20.0/_framesPerSecond));
        fiftyLabel.text = _isFrameUnits ? "50" : "" + (Math.round(1000 * 50.0/_framesPerSecond));
    }
    
    function collectScopeData() {
        if (inputCh.checked) { 
            _scopeInputData  = AudioScope.scopeInput;
        }
        if (outputLeftCh.checked) {
            _scopeOutputLeftData  = AudioScope.scopeOutputLeft;
        }
        if (outputRightCh.checked) {
            _scopeOutputRightData  = AudioScope.scopeOutputRight;
        }
    }
    
    function collectTriggerData() {
        if (inputCh.checked) { 
            _triggerInputData  = AudioScope.triggerInput;
        }
        if (outputLeftCh.checked) {
            _triggerOutputLeftData  = AudioScope.triggerOutputLeft;
        }
        if (outputRightCh.checked) {
            _triggerOutputRightData  = AudioScope.triggerOutputRight;
        }
    }

    function setRecordingLabelOpacity(opacity) {
        _recOpacity = opacity;
        recCircle.opacity = _recOpacity;
        recText.opacity = _recOpacity;
    }

    function updateRecordingLabel() {
        _recOpacity += _recSign;
        if (_recOpacity > 1.0 || _recOpacity < 0.0) {
            _recOpacity = _recOpacity > 1.0 ? 1.0 : 0.0;
            _recSign *= -1;
        }
        setRecordingLabelOpacity(_recOpacity);
    }
        
    function pullFreshValues() {
        if (AudioScriptingInterface.getRecording()) {
            updateRecordingLabel();
        }

        if (!AudioScope.getPause()) {        
            if (!_triggered) {
                collectScopeData();
            }               
        } 
        if (inputCh.checked || outputLeftCh.checked || outputRightCh.checked) {
            mycanvas.requestPaint();
        }
    }
    
    function startRecording() {
        _wavFilePath = (new Date()).toISOString();  // yyyy-mm-ddThh:mm:ss.sssZ
        _wavFilePath = _wavFilePath.replace(/[\-:]|\.\d*Z$/g, "").replace("T", "-") + ".wav";
        // Using controller recording default directory
        _wavFilePath = Recording.getDefaultRecordingSaveDirectory() + _wavFilePath;
        if (!AudioScriptingInterface.startRecording(_wavFilePath)) {
            Messages.sendMessage("Hifi-Notifications", JSON.stringify({message:"Error creating: "+_wavFilePath}));
            updateRecordingUI(false);
        }
    }

    function stopRecording() {
        AudioScriptingInterface.stopRecording();
        setRecordingLabelOpacity(0.0);
        Messages.sendMessage("Hifi-Notifications", JSON.stringify({message:"Saved: "+_wavFilePath}));
    }

    function updateRecordingUI(isRecording) {
        if (!isRecording) {
            recordButton.text = "Record";
            recordButton.color = hifi.buttons.black;
            outputLeftCh.checked = _outputLeftState;
            outputRightCh.checked = _outputRightState;
        } else {
            recordButton.text = "Stop";
            recordButton.color = hifi.buttons.red;
            _outputLeftState = outputLeftCh.checked;
            _outputRightState = outputRightCh.checked;
            outputLeftCh.checked = true;
            outputRightCh.checked = true;
        }
    }
    
    function toggleRecording() {
        if (AudioScriptingInterface.getRecording()) {
            updateRecordingUI(false);
            stopRecording();
        } else {
            updateRecordingUI(true);
            startRecording();
        }
    }
    
    Timer {
        interval: _refreshMs; running: true; repeat: true
        onTriggered: pullFreshValues()
    }

    Canvas {
        id: mycanvas
        anchors.fill:parent
        
        onPaint: {
        
            function displayMeasureArea(ctx) {
                
                ctx.fillStyle = Qt.rgba(0.1, 0.1, 0.1, 1);
                ctx.fillRect(_holdStart.x, 0, _holdEnd.x - _holdStart.x, height);
                
                ctx.lineWidth = "2";
                ctx.strokeStyle = "#555555";
                
                ctx.beginPath();
                ctx.moveTo(_holdStart.x, 0); 
                ctx.lineTo(_holdStart.x, height);
                ctx.moveTo(_holdEnd.x, 0); 
                ctx.lineTo(_holdEnd.x, height); 
                                
                ctx.moveTo(_holdStart.x, _holdStart.y); 
                ctx.lineTo(_holdEnd.x, _holdStart.y); 
                ctx.moveTo(_holdEnd.x, _holdEnd.y); 
                ctx.lineTo(_holdStart.x, _holdEnd.y);
                
                ctx.stroke();
            }
            
            function displayTrigger(ctx, lineWidth, color) {
                var crossSize = 3;
                var holeSize = 2;
                
                ctx.lineWidth = lineWidth;
                ctx.strokeStyle = color;
                
                ctx.beginPath();
                ctx.moveTo(_triggerValues.x - (crossSize + holeSize), _triggerValues.y); 
                ctx.lineTo(_triggerValues.x - holeSize, _triggerValues.y); 
                ctx.moveTo(_triggerValues.x + holeSize, _triggerValues.y); 
                ctx.lineTo(_triggerValues.x + (crossSize + holeSize), _triggerValues.y); 
                
                ctx.moveTo(_triggerValues.x, _triggerValues.y - (crossSize + holeSize)); 
                ctx.lineTo(_triggerValues.x, _triggerValues.y - holeSize); 
                ctx.moveTo(_triggerValues.x, _triggerValues.y + holeSize); 
                ctx.lineTo(_triggerValues.x, _triggerValues.y + (crossSize + holeSize));

                ctx.stroke();
            }
            
            function displayBackground(ctx, datawidth, steps, lineWidth, color) {
                var verticalPadding = 100;
                
                ctx.strokeStyle = color;
                ctx.lineWidth = lineWidth;
 
                ctx.moveTo(0, height/2); 
                ctx.lineTo(datawidth, height/2);
                
                var gap = datawidth/steps;
                for (var i = 0; i < steps; i++) {
                    ctx.moveTo(i*gap + 1, verticalPadding); 
                    ctx.lineTo(i*gap + 1, height-verticalPadding); 
                }
                ctx.moveTo(datawidth-1, verticalPadding); 
                ctx.lineTo(datawidth-1, height-verticalPadding); 
                
                ctx.stroke();
            }
            
            function drawScope(ctx, data, width, color) {
                ctx.beginPath();
                ctx.strokeStyle = color;
                ctx.lineWidth = width;
                var x = 0;
                for (var i = 0; i < data.length-1; i++) {
                    ctx.moveTo(x, data[i] + height/2); 
                    ctx.lineTo(++x, data[i+1] + height/2);              
                }
                ctx.stroke();
            }
            
            function getMeasurementText(dist) {
                var datasize = _scopeInputData.length;
                var value = 0;
                if (fiveFrames.checked) {
                    value = (_isFrameUnits) ? 5.0*dist/datasize : (Math.round(1000 * 5.0/_framesPerSecond))*dist/datasize;
                } else if (twentyFrames.checked) {
                    value = (_isFrameUnits) ? 20.0*dist/datasize : (Math.round(1000 * 20.0/_framesPerSecond))*dist/datasize;
                } else if (fiftyFrames.checked) {
                    value = (_isFrameUnits) ? 50.0*dist/datasize : (Math.round(1000 * 50.0/_framesPerSecond))*dist/datasize;
                }
                value = Math.abs(Math.round(value*100)/100);
                var measureText = "" + value + (_isFrameUnits ? " frames" : " milliseconds");
                return measureText;
            }
            
            function drawMeasurements(ctx, color) {
                ctx.fillStyle = color;
                ctx.font = "normal 16px sans-serif";
                var fontwidth = 8;
                var measureText = getMeasurementText(_holdEnd.x - _holdStart.x);
                if (_holdStart.x < _holdEnd.x) {
                    ctx.fillText("" + height/2 - _holdStart.y, _holdStart.x-40, _holdStart.y);
                    ctx.fillText("" + height/2 - _holdEnd.y, _holdStart.x-40, _holdEnd.y);  
                    ctx.fillText(measureText, _holdEnd.x+10, _holdEnd.y);                   
                } else {
                    ctx.fillText("" + height/2 - _holdStart.y, _holdStart.x+10, _holdStart.y);
                    ctx.fillText("" + height/2 - _holdEnd.y, _holdStart.x+10, _holdEnd.y); 
                    ctx.fillText(measureText, _holdEnd.x-fontwidth*measureText.length, _holdEnd.y);  
                }                               
            }
            
            var ctx = getContext("2d");
            
            ctx.fillStyle = Qt.rgba(0, 0, 0, 1);
            ctx.fillRect(0, 0, width, height);
                
            if (isHolding()) {
                displayMeasureArea(ctx);
            }

            var guideLinesColor = "#555555"
            var guideLinesWidth = "1"
            
            displayBackground(ctx, _scopeInputData.length, _steps, guideLinesWidth, guideLinesColor);
                
            var triggerWidth = "3"
            var triggerColor = "#EFB400"
            
            if (AudioScope.getAutoTrigger()) {
                displayTrigger(ctx, triggerWidth, triggerColor);
            }
            
            var scopeWidth = "2"
            var scopeInputColor = "#00B4EF"
            var scopeOutputLeftColor = "#BB0000"
            var scopeOutputRightColor = "#00BB00"
            
            if (!_triggered) {
                if (inputCh.checked) {
                    drawScope(ctx, _scopeInputData, scopeWidth, scopeInputColor);
                }
                if (outputLeftCh.checked) {
                    drawScope(ctx, _scopeOutputLeftData, scopeWidth, scopeOutputLeftColor);
                }
                if (outputRightCh.checked) {
                    drawScope(ctx, _scopeOutputRightData, scopeWidth, scopeOutputRightColor);
                }   
            } else {
                if (inputCh.checked) {
                    drawScope(ctx, _triggerInputData, scopeWidth, scopeInputColor);
                }
                if (outputLeftCh.checked) {
                    drawScope(ctx, _triggerOutputLeftData, scopeWidth, scopeOutputLeftColor);
                }
                if (outputRightCh.checked) {
                    drawScope(ctx, _triggerOutputRightData, scopeWidth, scopeOutputRightColor);
                }
            }
            
            if (isHolding()) {
                drawMeasurements(ctx, "#eeeeee");
            }
            
            if (_isPressed) {
                _pressedTime += _refreshMs;
            }
        }
    }
    
    MouseArea {
        id: hitbox
        anchors.fill: mycanvas
        hoverEnabled: true
        onPressed: {
            _isPressed = true;
            _pressedTime = 0;
            _holdStart.x = mouseX;
            _holdStart.y = mouseY;
        }
        onPositionChanged: {
            _holdEnd.x = mouseX;
            _holdEnd.y = mouseY;
        }
        onReleased: {
            if (!isHolding() && AudioScope.getAutoTrigger()) {
                _triggerValues.x = mouseX
                _triggerValues.y = mouseY
                AudioScope.setTriggerValues(mouseX, mouseY-height/2);
            }
            _isPressed = false;
            _pressedTime = 0;
        }
    }
    
    HifiControlsUit.CheckBox {
        id: activated
        boxSize: 20
        anchors.top: parent.top;
        anchors.left: parent.left;
        anchors.topMargin: 8;
        anchors.leftMargin: 20;
        checked: AudioScope.getVisible();
        onCheckedChanged: {     
            AudioScope.setVisible(checked);
            activelabel.text = AudioScope.getVisible() ? "On" : "Off"
        }
    }
          
    HifiControlsUit.Label {
        id: activelabel
        text: AudioScope.getVisible() ? "On" : "Off"
        anchors.top: activated.top;
        anchors.left: activated.right;
    }
    
    HifiControlsUit.CheckBox {
        id: outputLeftCh
        boxSize: 20
        text: "Output L"
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.top: parent.top;
        anchors.topMargin: 8;
        onCheckedChanged: {
            AudioScope.setServerEcho(outputLeftCh.checked || outputRightCh.checked);
        }
    }
	
    HifiControlsUit.Label {
        text: "Channels";
        anchors.horizontalCenter: outputLeftCh.horizontalCenter;
        anchors.bottom: outputLeftCh.top;
        anchors.bottomMargin: 8;
    }
    
    HifiControlsUit.CheckBox {
        id: inputCh
        boxSize: 20
        text: "Input Mono"
        anchors.bottom: outputLeftCh.bottom;
        anchors.right: outputLeftCh.left;
        anchors.rightMargin: 40;
        onCheckedChanged: {
			AudioScope.setLocalEcho(checked);
        }
    }
    
    HifiControlsUit.CheckBox {
        id: outputRightCh
        boxSize: 20
        text: "Output R"
        anchors.bottom: outputLeftCh.bottom;
        anchors.left: outputLeftCh.right;
        anchors.leftMargin: 40;
        onCheckedChanged: {
            AudioScope.setServerEcho(outputLeftCh.checked || outputRightCh.checked);
        }
    }
    
    HifiControlsUit.Button {
        id: recordButton;
        text: "Record";
        color: hifi.buttons.black;
        colorScheme: hifi.colorSchemes.dark;
        anchors.right: parent.right;
        anchors.bottom: parent.bottom;
        anchors.rightMargin: 30;
        anchors.bottomMargin: 8;
        width: 95;
        height: 55;
        onClicked: {
            toggleRecording();
        }
    }
    
    HifiControlsUit.Button {
        id: pauseButton;
        color: hifi.buttons.black;
        colorScheme: hifi.colorSchemes.dark;
        anchors.right: recordButton.left;
        anchors.bottom: parent.bottom;
        anchors.rightMargin: 30;
        anchors.bottomMargin: 8;
        height: 55;
        width: 95;
        text: " Pause ";
        onClicked: {
            AudioScope.togglePause();
        }
    }
    
    HifiControlsUit.CheckBox {
        id: twentyFrames
        boxSize: 20
        anchors.left: parent.horizontalCenter;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 8;
        onCheckedChanged: {
            if (checked){
                fiftyFrames.checked = false;
                fiveFrames.checked = false;
                AudioScope.selectAudioScopeTwentyFrames();
                _steps = 20;
                AudioScope.setPause(false);
            }
        }
    }
    
    HifiControlsUit.Label {
        id:twentyLabel
        anchors.left: twentyFrames.right;
        anchors.verticalCenter: twentyFrames.verticalCenter;
    }
    
    HifiControlsUit.Button {
        id: timeButton;
        color: hifi.buttons.black;
        colorScheme: hifi.colorSchemes.dark;
        text: "Display Frames";
        anchors.horizontalCenter: twentyFrames.horizontalCenter;
        anchors.bottom: twentyFrames.top;
        anchors.bottomMargin: 8;
        height: 26;
        onClicked: {
            _isFrameUnits = !_isFrameUnits;
            updateMeasureUnits();
        }
    }
    
    HifiControlsUit.CheckBox {
        id: fiveFrames
        boxSize: 20
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 8;
        anchors.horizontalCenterOffset: -50;
        checked: true;
        onCheckedChanged: {
            if (checked) {
                fiftyFrames.checked = false;
                twentyFrames.checked = false;
                AudioScope.selectAudioScopeFiveFrames();
                _steps = 5;
                AudioScope.setPause(false);
            }
        }
    }
    
    HifiControlsUit.Label {
        id:fiveLabel
        anchors.left: fiveFrames.right;
        anchors.verticalCenter: fiveFrames.verticalCenter;
    }
    
    HifiControlsUit.CheckBox {
        id: fiftyFrames
        boxSize: 20
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 8;
        anchors.horizontalCenterOffset: 70;
        onCheckedChanged: {
            if (checked) {
                twentyFrames.checked = false;
                fiveFrames.checked = false;
                AudioScope.selectAudioScopeFiftyFrames();
                _steps = 50;
                AudioScope.setPause(false);            
            }
        }
    }
    
    HifiControlsUit.Label {
        id:fiftyLabel
        anchors.left: fiftyFrames.right;
        anchors.verticalCenter: fiftyFrames.verticalCenter;
    }
    
    HifiControlsUit.Switch {
        id: triggerSwitch;
        height: 26;
        anchors.left: parent.left;
        anchors.bottom: parent.bottom;
        anchors.leftMargin: 75;
        anchors.bottomMargin: 8;
        labelTextOff: "Off";
        labelTextOn: "On";
        onCheckedChanged: {
            if (!checked) AudioScope.setPause(false);
            AudioScope.setPause(false);
            AudioScope.setAutoTrigger(checked);
            AudioScope.setTriggerValues(_triggerValues.x, _triggerValues.y-root.height/2);
        }
    }
    
    HifiControlsUit.Label {
        text: "Trigger";
        anchors.left: triggerSwitch.left;
        anchors.leftMargin: -15;
        anchors.bottom: triggerSwitch.top;
    }
    
    Rectangle {
        id: recordIcon;
        width:110;
        height:40;
        anchors.right: parent.right;
        anchors.top: parent.top;
        anchors.topMargin: 8;
        color: "transparent"
        
        Text {
            id: recText
            text: "REC"
            color: "red"
            font.pixelSize: 30;
            anchors.left: recCircle.right;
            anchors.leftMargin: 10;
            opacity: _recOpacity;
            y: -8;
        }       
        
        Rectangle {
            id: recCircle;
            width: 25;
            height: 25;
            radius: width*0.5
            opacity: _recOpacity;
            color: "red";
        }
    }
    
    Component.onCompleted: {  
        _steps = AudioScope.getFramesPerScope();
        AudioScope.setTriggerValues(_triggerValues.x, _triggerValues.y-root.height/2);
        activated.checked = true;
		inputCh.checked = true;
        updateMeasureUnits();
    }
    
    Connections {
        target: AudioScope
        onPauseChanged: {
            if (!AudioScope.getPause()) {
                pauseButton.text = "Pause";
                pauseButton.color = hifi.buttons.black;
                AudioScope.setTriggered(false);
                _triggered = false;
            } else {
                pauseButton.text = "Continue";
                pauseButton.color = hifi.buttons.blue;
            }           
        }
        onTriggered: {
            _triggered = true;
            collectTriggerData();
            AudioScope.setPause(true);
        }
    }
}
