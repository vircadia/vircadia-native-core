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
import "styles-uit"
import "controls-uit" as HifiControlsUit

Item {
    id: root
    width: parent.width
    height: parent.height   
    
    property var _scopedata 
    property var _triggerdata
    property var _triggerValues
    property var _triggered
    property var _steps
    
    Component.onCompleted: {
        // createValues();   
        _triggerValues = { x: width/2, y: height/3 }
        _triggered = false
        _steps = 5
    }
    
    
    function pullFreshValues() {
        if (!AudioScope.getPause()){
            _scopedata = AudioScope.scopeInput
            if (AudioScope.getTriggered()) {
                _triggered = true
                _triggerdata = AudioScope.triggerInput
            }
        } 
        mycanvas.requestPaint()
    }

    Timer {
        interval: 10; running: true; repeat: true
        onTriggered: pullFreshValues()
    }

    Canvas {
        id: mycanvas
        anchors.fill:parent
        
        onPaint: {
            var lineHeight = 12;
            
            function displayTrigger(ctx) {
                var size = 3;
                ctx.lineWidth="3";
                ctx.strokeStyle="#EFB400";
                ctx.beginPath();
                ctx.moveTo(_triggerValues.x - (size + 2), _triggerValues.y); 
                ctx.lineTo(_triggerValues.x - 2, _triggerValues.y); 
                ctx.moveTo(_triggerValues.x + 2, _triggerValues.y); 
                ctx.lineTo(_triggerValues.x + (size + 2), _triggerValues.y); 
                
                ctx.moveTo(_triggerValues.x, _triggerValues.y - (size + 2)); 
                ctx.lineTo(_triggerValues.x, _triggerValues.y - 2); 
                ctx.moveTo(_triggerValues.x, _triggerValues.y + 2); 
                ctx.lineTo(_triggerValues.x, _triggerValues.y + (size + 2));

                ctx.stroke();
            }
            
            function displayBackground(ctx, datawidth, steps, color) {
                ctx.fillStyle = Qt.rgba(0, 0, 0, 1);
                ctx.fillRect(0, 0, width, height);
                
                ctx.strokeStyle= color;
                ctx.lineWidth="1";
 
                ctx.moveTo(0, height/2); 
                ctx.lineTo(datawidth, height/2);
                
                var gap = datawidth/steps;
                for (var i = 0; i < steps; i++) {
                    ctx.moveTo(i*gap + 1, 100); 
                    ctx.lineTo(i*gap + 1, height-100); 
                }
                ctx.moveTo(datawidth-1, 100); 
                ctx.lineTo(datawidth-1, height-100); 
                ctx.stroke();
                
            }
            
            function drawScope(ctx, data, width, color) {
                ctx.beginPath();
                ctx.strokeStyle = color; // Green path
                ctx.lineWidth=width;
                var x = 0;
                for (var i = 0; i < data.length-1; i++) {
                    ctx.moveTo(x, data[i] + height/2); 
                    ctx.lineTo(++x, data[i+1] + height/2);              
                }
                ctx.stroke();
            }
            
            
            
            var ctx = getContext("2d");

            displayBackground(ctx, _scopedata.length, _steps, "#555555");
                
            drawScope(ctx, _scopedata, "2", "#00B4EF");
            if (_triggered) {
                drawScope(ctx, _triggerdata, "1", "#EF0000");
            }
            if (AudioScope.getAutoTrigger()) {
                displayTrigger(ctx);
            }
        }
    }
    
    MouseArea {
        id: hitbox
        anchors.fill: mycanvas
        hoverEnabled: true
        onClicked: {
            _triggerValues.x = mouseX
            _triggerValues.y = mouseY
            AudioScope.setTriggerValues(mouseX, mouseY-height/2);
        }
    }
    
    HifiControlsUit.CheckBox {
        id: activated
        boxSize: 20
        anchors.top: parent.top;
        anchors.left: parent.left;
        anchors.topMargin: 20;
        anchors.leftMargin: 20;
        checked: AudioScope.getEnabled();
        onCheckedChanged: {
            AudioScope.setEnabled(checked);
            activelabel.text = AudioScope.getEnabled() ? "On" : "Off"
        }
    }
    
    HifiControlsUit.Label {
        id: activelabel
        text: AudioScope.getEnabled() ? "On" : "Off"
        anchors.top: activated.top;
        anchors.left: activated.right;
    }
    
    HifiControlsUit.Button {
        id: pauseButton;
        color: hifi.buttons.black;
        colorScheme: hifi.colorSchemes.dark;
        anchors.right: parent.right;
        anchors.bottom: parent.bottom;
        anchors.rightMargin: 30;
        anchors.bottomMargin: 8;
        height: 26;
        text: "Pause Scope";
        onClicked: {
            AudioScope.togglePause();
            pauseButton.text = AudioScope.getPause() ? "Continue Scope" : "Pause Scope";
        }
    }
    

    
    HifiControlsUit.CheckBox {
        id: twentyFrames
        boxSize: 20
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 8;
        onCheckedChanged: {
            if (checked){
                fiftyFrames.checked = false;
                fiveFrames.checked = false;
                AudioScope.selectAudioScopeTwentyFrames();
                _steps = 20;
                _triggered = false;
                AudioScope.setTriggered(false);
            }
        }
    }
    HifiControlsUit.Label {
        text: "20 Frames";
        anchors.horizontalCenter: twentyFrames.horizontalCenter;
        anchors.bottom: twentyFrames.top;
    }
    
    HifiControlsUit.CheckBox {
        id: fiveFrames
        boxSize: 20
        anchors.bottom: twentyFrames.bottom;
        anchors.right: twentyFrames.left;
        anchors.rightMargin: 80;
        checked: true;
        onCheckedChanged: {
            if (checked) {
                fiftyFrames.checked = false;
                twentyFrames.checked = false;
                AudioScope.selectAudioScopeFiveFrames();
                _steps = 5;
                _triggered = false;
                AudioScope.setTriggered(false);
            }
        }
    }
    
    HifiControlsUit.Label {
        text: "5 Frames";
        anchors.horizontalCenter: fiveFrames.horizontalCenter;
        anchors.bottom: fiveFrames.top;
    }
    
    HifiControlsUit.CheckBox {
        id: fiftyFrames
        boxSize: 20
        anchors.bottom: twentyFrames.bottom;
        anchors.left: twentyFrames.right;
        anchors.leftMargin: 80;
        onCheckedChanged: {
            if (checked) {
                twentyFrames.checked = false;
                fiveFrames.checked = false;
                AudioScope.selectAudioScopeFiftyFrames();
                _steps = 50;
                _triggered = false;
                AudioScope.setTriggered(false);
            }
        }
    }
    
    HifiControlsUit.Label {
        text: "50 Frames";
        anchors.horizontalCenter: fiftyFrames.horizontalCenter;
        anchors.bottom: fiftyFrames.top;
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
            _triggered = false;
            AudioScope.setTriggered(false);
            AudioScope.setAutoTrigger(checked);
        }
    }
    HifiControlsUit.Label {
        text: "Trigger";
        anchors.left: triggerSwitch.left;
        anchors.leftMargin: -15;
        anchors.bottom: triggerSwitch.top;
    }
}
