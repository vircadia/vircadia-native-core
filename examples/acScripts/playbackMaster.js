//
//  playbackMaster.js
//  acScripts
//
//  Created by Edgar Pironti on 11/17/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";


var ac_number = 1; // This is the default number of ACs. Their ID need to be unique and between 0 (included) and ac_number (excluded)
var names = new Array();  // It is possible to specify the name of the ACs in this array. ACs names ordered by IDs (Default name is "ACx", x = ID + 1))
var channel = "PlaybackChannel1";
var subscribed = false;
var clip_url = null;
var input_text = null;

// Script. DO NOT MODIFY BEYOND THIS LINE.
Script.include("../libraries/toolBars.js");

var DO_NOTHING = 0;
var PLAY = 1;
var PLAY_LOOP = 2;
var STOP = 3;
var SHOW = 4;
var HIDE = 5;
var LOAD = 6;

var windowDimensions = Controller.getViewportDimensions();
var TOOL_ICON_URL = HIFI_PUBLIC_BUCKET + "images/tools/";
var ALPHA_ON = 1.0;
var ALPHA_OFF = 0.7;
var COLOR_TOOL_BAR = { red: 0, green: 0, blue: 0 };
var COLOR_MASTER = { red: 0, green: 0, blue: 0 };
var TEXT_HEIGHT = 12;
var TEXT_MARGIN = 3;

var toolBars = new Array();
var nameOverlays = new Array();
var onOffIcon = new Array();
var playIcon = new Array();
var playLoopIcon = new Array();
var stopIcon = new Array();
var loadIcon = new Array();

setupPlayback();

function setupPlayback() {
    ac_number = Window.prompt("Insert number of agents: ","1");
    if (ac_number === "" || ac_number === null)
        ac_number = 1;
    Messages.subscribe(channel);
    subscribed = true;
    setupToolBars();
}

function setupToolBars() {      
    if (toolBars.length > 0) {
        print("Multiple calls to Recorder.js:setupToolBars()");
        return;
    }
    Tool.IMAGE_HEIGHT /= 2;
    Tool.IMAGE_WIDTH /= 2;
    
    for (i = 0; i <= ac_number; i++) {
        toolBars.push(new ToolBar(0, 0, ToolBar.HORIZONTAL));
        toolBars[i].setBack((i == ac_number) ? COLOR_MASTER : COLOR_TOOL_BAR, ALPHA_OFF);
        
        onOffIcon.push(toolBars[i].addTool({
                                           imageURL: TOOL_ICON_URL + "ac-on-off.svg",
                                           subImage: { x: 0, y: 0, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
                                           x: 0, y: 0,
                                           width: Tool.IMAGE_WIDTH,
                                           height: Tool.IMAGE_HEIGHT,
                                           alpha: ALPHA_ON,
                                           visible: true
                                           }, true, true));
        
        playIcon[i] = toolBars[i].addTool({
                                          imageURL: TOOL_ICON_URL + "play.svg",
                                          subImage: { x: 0, y: 0, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
                                          width: Tool.IMAGE_WIDTH,
                                          height: Tool.IMAGE_HEIGHT,
                                          alpha: ALPHA_OFF,
                                          visible: true
                                          }, false);
        
        var playLoopWidthFactor = 1.65;
        playLoopIcon[i] = toolBars[i].addTool({
                                              imageURL: TOOL_ICON_URL + "play-and-loop.svg",
                                              subImage: { x: 0, y: 0, width: playLoopWidthFactor * Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
                                              width: playLoopWidthFactor * Tool.IMAGE_WIDTH,
                                              height: Tool.IMAGE_HEIGHT,
                                              alpha: ALPHA_OFF,
                                              visible: true
                                              }, false);
        
        stopIcon[i] = toolBars[i].addTool({
                                          imageURL: TOOL_ICON_URL + "recording-stop.svg",
                                          width: Tool.IMAGE_WIDTH,
                                          height: Tool.IMAGE_HEIGHT,
                                          alpha: ALPHA_OFF,
                                          visible: true
                                          }, false);
                                          
        loadIcon[i] = toolBars[i].addTool({
                                          imageURL: TOOL_ICON_URL + "recording-upload.svg",
                                          width: Tool.IMAGE_WIDTH,
                                          height: Tool.IMAGE_HEIGHT,
                                          alpha: ALPHA_OFF,
                                          visible: true
                                          }, false);
        
        nameOverlays.push(Overlays.addOverlay("text", {
                                              backgroundColor: { red: 0, green: 0, blue: 0 },
                                              font: { size: TEXT_HEIGHT },
                                              text: (i == ac_number) ? "Master" : i + ". " +
                                              ((i < names.length) ? names[i] :
                                               "AC" + i),
                                              x: 0, y: 0,
                                              width: toolBars[i].width + ToolBar.SPACING,
                                              height: TEXT_HEIGHT + TEXT_MARGIN,
                                              leftMargin: TEXT_MARGIN,
                                              topMargin: TEXT_MARGIN,
                                              alpha: ALPHA_OFF,
                                              backgroundAlpha: ALPHA_OFF,
                                              visible: true
                                              }));
    }
}

function sendCommand(id, action) {
    
    if (action === SHOW) {
        toolBars[id].selectTool(onOffIcon[id], false);
        toolBars[id].setAlpha(ALPHA_ON, playIcon[id]);
        toolBars[id].setAlpha(ALPHA_ON, playLoopIcon[id]);
        toolBars[id].setAlpha(ALPHA_ON, stopIcon[id]);
        toolBars[id].setAlpha(ALPHA_ON, loadIcon[id]);
    } else if (action === HIDE) {
        toolBars[id].selectTool(onOffIcon[id], true);
        toolBars[id].setAlpha(ALPHA_OFF, playIcon[id]);
        toolBars[id].setAlpha(ALPHA_OFF, playLoopIcon[id]);
        toolBars[id].setAlpha(ALPHA_OFF, stopIcon[id]);
        toolBars[id].setAlpha(ALPHA_OFF, loadIcon[id]);
    } else if (toolBars[id].toolSelected(onOffIcon[id])) {
        return;
    }
    
    if (id == (toolBars.length - 1))
        id = -1; // Master command becomes broadcast.
    
    var message = {
        id_key: id,
        action_key: action,
        clip_url_key: clip_url
    };
    
    if(subscribed){
        Messages.sendMessage(channel, JSON.stringify(message));
        print("Message sent!");
    }
}

function mousePressEvent(event) {
    clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });
    
    // Check master control
    var i = toolBars.length - 1;
    if (onOffIcon[i] === toolBars[i].clicked(clickedOverlay, false)) {
        if (toolBars[i].toolSelected(onOffIcon[i])) {
            sendCommand(i, SHOW);
        } else {
            sendCommand(i, HIDE);
        }
    } else if (playIcon[i] === toolBars[i].clicked(clickedOverlay, false)) {
        sendCommand(i, PLAY);
    } else if (playLoopIcon[i] === toolBars[i].clicked(clickedOverlay, false)) {
        sendCommand(i, PLAY_LOOP);
    } else if (stopIcon[i] === toolBars[i].clicked(clickedOverlay, false)) {
        sendCommand(i, STOP);
    } else if (loadIcon[i] === toolBars[i].clicked(clickedOverlay, false)) {
        input_text = Window.prompt("Insert the url of the clip: ","");
        if (!(input_text === "" || input_text === null)) {
            clip_url = input_text;
            sendCommand(i, LOAD);
        }
    } else {
        // Check individual controls
        for (i = 0; i < ac_number; i++) {
            if (onOffIcon[i] === toolBars[i].clicked(clickedOverlay, false)) {
                if (toolBars[i].toolSelected(onOffIcon[i], false)) {
                    sendCommand(i, SHOW);
                } else {
                    sendCommand(i, HIDE);
                }
            } else if (playIcon[i] === toolBars[i].clicked(clickedOverlay, false)) {
                sendCommand(i, PLAY);
            } else if (playLoopIcon[i] === toolBars[i].clicked(clickedOverlay, false)) {
                sendCommand(i, PLAY_LOOP);
            } else if (stopIcon[i] === toolBars[i].clicked(clickedOverlay, false)) {
                sendCommand(i, STOP);
            } else if (loadIcon[i] === toolBars[i].clicked(clickedOverlay, false)) {                
                input_text = Window.prompt("Insert the url of the clip: ","");
                if (!(input_text === "" || input_text === null)) {
                    clip_url = input_text;
                    sendCommand(i, LOAD);
                }                
            } else {
                
            }
        }
    }
}

function moveUI() {
    var textSize = TEXT_HEIGHT + 2 * TEXT_MARGIN;
    var relative = { x: 70, y: 75 + (ac_number) * (Tool.IMAGE_HEIGHT + ToolBar.SPACING + textSize) };
    
    for (i = 0; i <= ac_number; i++) {
        toolBars[i].move(relative.x,
                         windowDimensions.y - relative.y +
                         i * (Tool.IMAGE_HEIGHT + ToolBar.SPACING + textSize));
        
        Overlays.editOverlay(nameOverlays[i], {
                             x: toolBars[i].x - ToolBar.SPACING,
                             y: toolBars[i].y - textSize
                             });
    }
}

function update() {
    var newDimensions = Controller.getViewportDimensions();
    if (windowDimensions.x != newDimensions.x ||
            windowDimensions.y != newDimensions.y) {
        windowDimensions = newDimensions;
        moveUI();
    }
}

function scriptEnding() {
    for (i = 0; i <= ac_number; i++) {
        toolBars[i].cleanup();
        Overlays.deleteOverlay(nameOverlays[i]);
    }
    
    if(subscribed)
        Messages.unsubscribe(channel);
}

Controller.mousePressEvent.connect(mousePressEvent);
Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);

moveUI();