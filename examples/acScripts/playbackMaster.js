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
Script.include("./AgentPoolController.js");

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

var masterController = new MasterController();

var input_text = null;

// Script. DO NOT MODIFY BEYOND THIS LINE.
//Script.include("../libraries/toolBars.js");
Script.include(HIFI_PUBLIC_BUCKET + "scripts/libraries/toolBars.js");
// We want small icons
Tool.IMAGE_HEIGHT /= 2;
Tool.IMAGE_WIDTH /= 2;

var PLAY = 1;
var PLAY_LOOP = 2;
var STOP = 3;
var LOAD = 6;

var windowDimensions = Controller.getViewportDimensions();
var TOOL_ICON_URL = HIFI_PUBLIC_BUCKET + "images/tools/";
var ALPHA_ON = 1.0;
var ALPHA_OFF = 0.7;
var COLOR_TOOL_BAR = { red: 0, green: 0, blue: 0 };
var COLOR_MASTER = { red: 0, green: 0, blue: 0 };
var TEXT_HEIGHT = 12;
var TEXT_MARGIN = 3;

// Add new features to Actor class:
Actor.prototype.destroy = function() { 
    print("Actor.prototype.destroy");
    print("Need to fire myself" + this.agentID);
    masterController.fireAgent(this);  
}


Actor.prototype.resetClip = function(clipURL, onLoadClip) {
    this.clipURL = clipURL;
    this.onLoadClip = onLoadClip;
    if (this.isConnected()) {
        this.onLoadClip(this);
    }
}


Actor.prototype.onMousePressEvent = function(clickedOverlay) {
    if (this.playIcon === this.toolbar.clicked(clickedOverlay, false)) {
        masterController.sendCommand(this.agentID, PLAY);
    } else if (this.playLoopIcon === this.toolbar.clicked(clickedOverlay, false)) {
        masterController.sendCommand(this.agentID, PLAY_LOOP);
    } else if (this.stopIcon === this.toolbar.clicked(clickedOverlay, false)) {
        masterController.sendCommand(this.agentID, STOP);
    } else if (this.nameOverlay === clickedOverlay) {                
        print("Actor: " + JSON.stringify(this));
    } else {
        return false;
    }

    return true;
}

Actor.prototype._buildUI = function() {
    print("Actor.prototype._buildUI = " + JSON.stringify(this));
    
    this.toolbar = new ToolBar(0, 0, ToolBar.HORIZONTAL);

    this.toolbar.setBack(COLOR_TOOL_BAR, ALPHA_OFF);
    
    this.playIcon = this.toolbar.addTool({
                                      imageURL: TOOL_ICON_URL + "play.svg",
                                      subImage: { x: 0, y: 0, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
                                      width: Tool.IMAGE_WIDTH,
                                      height: Tool.IMAGE_HEIGHT,
                                      alpha: ALPHA_OFF,
                                      visible: true
                                      }, false);
    
    var playLoopWidthFthis = 1.65;
    this.playLoopIcon = this.toolbar.addTool({
                                      imageURL: TOOL_ICON_URL + "play-and-loop.svg",
                                      subImage: { x: 0, y: 0, width: playLoopWidthFthis * Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
                                      width: playLoopWidthFthis * Tool.IMAGE_WIDTH,
                                      height: Tool.IMAGE_HEIGHT,
                                      alpha: ALPHA_OFF,
                                      visible: true
                                      }, false);
    
    this.stopIcon = this.toolbar.addTool({
                                      imageURL: TOOL_ICON_URL + "recording-stop.svg",
                                      width: Tool.IMAGE_WIDTH,
                                      height: Tool.IMAGE_HEIGHT,
                                      alpha: ALPHA_OFF,
                                      visible: true
                                      }, false);
    
    this.nameOverlay = Overlays.addOverlay("text", {
                                      backgroundColor: { red: 0, green: 0, blue: 0 },
                                      font: { size: TEXT_HEIGHT },
                                      text: "AC offline",
                                      x: 0, y: 0,
                                      width: this.toolbar.width + ToolBar.SPACING,
                                      height: TEXT_HEIGHT + TEXT_MARGIN,
                                      leftMargin: TEXT_MARGIN,
                                      topMargin: TEXT_MARGIN,
                                      alpha: ALPHA_OFF,
                                      backgroundAlpha: ALPHA_OFF,
                                      visible: true
                                      });
}

Actor.prototype._destroyUI = function() {
    this.toolbar.cleanup();
    Overlays.deleteOverlay(this.nameOverlay);   
}

Actor.prototype.moveUI = function(pos) {
    var textSize = TEXT_HEIGHT + 2 * TEXT_MARGIN;
    this.toolbar.move(pos.x, pos.y);

    Overlays.editOverlay(this.nameOverlay, {
                     x: this.toolbar.x - ToolBar.SPACING,
                     y: this.toolbar.y - textSize
                     });
}

Director = function() {
    this.actors = new Array();
    this.toolbar = null;
    this._buildUI();
    this.requestPerformanceLoad = false;
    this.performanceURL = "";
};

Director.prototype.destroy = function () {
    print("Director.prototype.destroy") 
    this.clearActors();
    this.toolbar.cleanup();
    Overlays.deleteOverlay(this.nameOverlay);
}

Director.prototype.clearActors = function () {
    print("Director.prototype.clearActors")
    while (this.actors.length > 0) {
        this.actors.pop().destroy();
    }

    this.actors = new Array();// Brand new actors    
}

Director.prototype._buildUI = function () {
    this.toolbar = new ToolBar(0, 0, ToolBar.HORIZONTAL);
    
    this.toolbar.setBack(COLOR_MASTER, ALPHA_OFF);
   
    this.onOffIcon = this.toolbar.addTool({
                                       imageURL: TOOL_ICON_URL + "ac-on-off.svg",
                                       subImage: { x: 0, y: 0, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
                                       x: 0, y: 0,
                                       width: Tool.IMAGE_WIDTH,
                                       height: Tool.IMAGE_HEIGHT,
                                       alpha: ALPHA_ON,
                                       visible: true
                                       }, true, true);
    
    this.playIcon = this.toolbar.addTool({
                                      imageURL: TOOL_ICON_URL + "play.svg",
                                      subImage: { x: 0, y: 0, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
                                      width: Tool.IMAGE_WIDTH,
                                      height: Tool.IMAGE_HEIGHT,
                                      alpha: ALPHA_OFF,
                                      visible: true
                                      }, false);
    
    var playLoopWidthFthis = 1.65;
    this.playLoopIcon = this.toolbar.addTool({
                                      imageURL: TOOL_ICON_URL + "play-and-loop.svg",
                                      subImage: { x: 0, y: 0, width: playLoopWidthFthis * Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
                                      width: playLoopWidthFthis * Tool.IMAGE_WIDTH,
                                      height: Tool.IMAGE_HEIGHT,
                                      alpha: ALPHA_OFF,
                                      visible: true
                                      }, false);
    
    this.stopIcon = this.toolbar.addTool({
                                      imageURL: TOOL_ICON_URL + "recording-stop.svg",
                                      width: Tool.IMAGE_WIDTH,
                                      height: Tool.IMAGE_HEIGHT,
                                      alpha: ALPHA_OFF,
                                      visible: true
                                      }, false);
                                      
    this.loadIcon = this.toolbar.addTool({
                                      imageURL: TOOL_ICON_URL + "recording-upload.svg",
                                      width: Tool.IMAGE_WIDTH,
                                      height: Tool.IMAGE_HEIGHT,
                                      alpha: ALPHA_OFF,
                                      visible: true
                                      }, false);
    
    this.nameOverlay = Overlays.addOverlay("text", {
                                      backgroundColor: { red: 0, green: 0, blue: 0 },
                                      font: { size: TEXT_HEIGHT },
                                      text: "Master",
                                      x: 0, y: 0,
                                      width: this.toolbar.width + ToolBar.SPACING,
                                      height: TEXT_HEIGHT + TEXT_MARGIN,
                                      leftMargin: TEXT_MARGIN,
                                      topMargin: TEXT_MARGIN,
                                      alpha: ALPHA_OFF,
                                      backgroundAlpha: ALPHA_OFF,
                                      visible: true
                                      }); 
}

Director.prototype.onMousePressEvent = function(clickedOverlay) {
    if (this.playIcon === this.toolbar.clicked(clickedOverlay, false)) {
        print("master play");
        masterController.sendCommand(BROADCAST_AGENTS, PLAY);
    } else if (this.onOffIcon === this.toolbar.clicked(clickedOverlay, false)) {
        this.clearActors();
        return true;
    } else if (this.playLoopIcon === this.toolbar.clicked(clickedOverlay, false)) {
        masterController.sendCommand(BROADCAST_AGENTS, PLAY_LOOP);
    } else if (this.stopIcon === this.toolbar.clicked(clickedOverlay, false)) {
        masterController.sendCommand(BROADCAST_AGENTS, STOP);
    } else if (this.loadIcon === this.toolbar.clicked(clickedOverlay, false)) {                
        input_text = Window.prompt("Insert the url of the clip: ","");
        if (!(input_text === "" || input_text === null)) {
            print("Performance file ready to be loaded url = " + input_text); 
            this.requestPerformanceLoad = true;
            this.performanceURL = input_text;
        }        
    } else if (this.nameOverlay === clickedOverlay) {                
        print("Director: " + JSON.stringify(this));
    } else {
        // Check individual controls
        for (var i = 0; i < this.actors.length; i++) {
            if (this.actors[i].onMousePressEvent(clickedOverlay)) {
                return true;
            }
        }

        return false; // nothing clicked from our known overlays
    }

    return true;
}

Director.prototype.moveUI = function(pos) {
    var textSize = TEXT_HEIGHT + 2 * TEXT_MARGIN;
    var relative = { x: pos.x, y: pos.y + (this.actors.length + 1) * (Tool.IMAGE_HEIGHT + ToolBar.SPACING + textSize) };
   
    this.toolbar.move(relative.x, windowDimensions.y - relative.y);
    Overlays.editOverlay(this.nameOverlay, {
                 x: this.toolbar.x - ToolBar.SPACING,
                 y: this.toolbar.y - textSize
                 });

    for (var i = 0; i < this.actors.length; i++) {
        this.actors[i].moveUI({x: relative.x, y: windowDimensions.y - relative.y +
                         (i + 1) * (Tool.IMAGE_HEIGHT + ToolBar.SPACING + textSize)});
    }
}


Director.prototype.reloadPerformance = function() {
    this.requestPerformanceLoad = false;

    if (this.performanceURL[0] == '{') {
        var jsonPerformance = JSON.parse(this.performanceURL);
        this.onPerformanceLoaded(jsonPerformance);
    } else {

        var urlpartition = this.performanceURL.split(".");
        print(urlpartition[0]);
        print(urlpartition[1]);

        if ((urlpartition.length > 1) && (urlpartition[urlpartition.length - 1] === "hfr")) {
            print("detected a unique clip url");
            var oneClipPerformance = new Object();
            oneClipPerformance.avatarClips = new Array();
            oneClipPerformance.avatarClips[0] = input_text;

            print(JSON.stringify(oneClipPerformance));

            // we make a local simple performance file with a single clip and pipe in directly
            this.onPerformanceLoaded(oneClipPerformance);
            return true;
        } else {
            // FIXME: I cannot pass directly this.onPerformanceLoaded, is that exepected ?
            var localThis = this;
            Assets.downloadData(input_text, function(data) { localThis.onPerformanceLoaded(JSON.parse(data)); });
        }
    }
}

Director.prototype.onPerformanceLoaded = function(performanceJSON) {
    // First fire all the current actors    
    this.clearActors();

    print("Director.prototype.onPerformanceLoaded = " + JSON.stringify(performanceJSON));
    if (performanceJSON.avatarClips != null) {
        var numClips = performanceJSON.avatarClips.length;
        print("Found " + numClips + "in the performance file, and currently using " + this.actors.length + " actor(s)");

        for (var i = 0; i < numClips; i++) {
            this.hireActor(performanceJSON.avatarClips[i]);
        }

    }
}


Director.prototype.hireActor = function(clipURL) {
    print("new actor = " + this.actors.length );
    var newActor = new Actor();
    newActor.clipURL = null;
    newActor.onLoadClip = function(clip) {};

    var localThis = this;
    newActor.onHired = function(actor) {
        print("agent hired from Director! " + actor.agentID)
        Overlays.editOverlay(actor.nameOverlay, {
                        text: "AC " + actor.agentID,
                        backgroundColor: { red: 0, green: 255, blue: 0 }
                     });

        if (actor.clipURL != null) {
            print("agent hired, calling load clip for url " + actor.clipURL); 
            actor.onLoadClip(actor);
        }
    };

    newActor.onFired = function(actor) { 
        print("agent fired from playbackMaster! " + actor.agentID);
        var index = localThis.actors.indexOf(actor);
        if (index >= 0) {
            localThis.actors.splice(index, 1); 
        }

        actor._destroyUI();
    
        actor.destroy();
        moveUI();
    }

    newActor.resetClip(clipURL,  function(actor) {
        print("Load clip for agent" + actor.agentID + " calling load clip for url " + actor.clipURL); 
        masterController.sendCommand(actor.agentID, LOAD, actor.clipURL);
    });


    masterController.hireAgent(newActor);        
    newActor._buildUI();

    this.actors.push(newActor);

    moveUI();
}


masterController.reset();
var director = new Director();

moveUI();


function mousePressEvent(event) {
    print("mousePressEvent");
    clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });
    
    // Check director and actors
    director.onMousePressEvent(clickedOverlay);
}

function moveUI() {
    director.moveUI({ x: 70, y: 75});

}

function update(deltaTime) {
    var newDimensions = Controller.getViewportDimensions();
    if (windowDimensions.x != newDimensions.x ||
            windowDimensions.y != newDimensions.y) {
        windowDimensions = newDimensions;
        moveUI();
    }

    if (director.requestPerformanceLoad) {
       print("reloadPerformance " + director.performanceURL);
       director.reloadPerformance();
    }

    masterController.update(deltaTime);
}

function scriptEnding() {
    print("cleanup")
    director.destroy(); 
    masterController.destroy();
}

Controller.mousePressEvent.connect(mousePressEvent);
Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);

