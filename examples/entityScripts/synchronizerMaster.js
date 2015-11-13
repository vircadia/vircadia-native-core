//
//  synchronizerMaster.js
//  examples/entityScripts
//
//  Created by Alessandro Signa on 11/12/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Run this script to spawn a box (synchronizer) and drive the start/end of the event for anyone who is inside the box
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var PARAMS_SCRIPT_URL = 'https://raw.githubusercontent.com/AlessandroSigna/hifi/05aa1d4ce49c719353007c245ae77ef2d2a8fc36/examples/entityScripts/synchronizerEntityScript.js';


HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
Script.include("../libraries/toolBars.js");
Script.include("../libraries/utils.js");



var rotation = Quat.safeEulerAngles(Camera.getOrientation());
rotation = Quat.fromPitchYawRollDegrees(0, rotation.y, 0);
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(1, Quat.getFront(rotation)));

var TOOL_ICON_URL = HIFI_PUBLIC_BUCKET + "images/tools/";
var ALPHA_ON = 1.0;
var ALPHA_OFF = 0.7;
var COLOR_TOOL_BAR = { red: 0, green: 0, blue: 0 };

var toolBar = null;
var recordIcon;



var isHappening = false;

var testEntity = Entities.addEntity({
    name: 'paramsTestEntity',
    dimensions: {
        x: 2,
        y: 1,
        z: 2
    },
    type: 'Box',
    position: center,
    color: {
        red: 255,
        green: 255,
        blue: 255
    },
    visible: true,
    ignoreForCollisions: true,
    script: PARAMS_SCRIPT_URL,
    
    userData: JSON.stringify({
        myKey: {
            valueToCheck: false
        }
    })
});


setupToolBar();

function setupToolBar() {
    if (toolBar != null) {
        print("Multiple calls to setupToolBar()");
        return;
    }
    Tool.IMAGE_HEIGHT /= 2;
    Tool.IMAGE_WIDTH /= 2;
    
    toolBar = new ToolBar(0, 0, ToolBar.HORIZONTAL);    //put the button in the up-left corner
    
    toolBar.setBack(COLOR_TOOL_BAR, ALPHA_OFF);
    
    recordIcon = toolBar.addTool({
        imageURL: TOOL_ICON_URL + "recording-record.svg",
        subImage: { x: 0, y: 0, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
        x: 0, y: 0,
        width: Tool.IMAGE_WIDTH,
        height: Tool.IMAGE_HEIGHT,
        alpha: MyAvatar.isPlaying() ? ALPHA_OFF : ALPHA_ON,
        visible: true
    }, true, isHappening);
    
}

function mousePressEvent(event) {
    clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });
    if (recordIcon === toolBar.clicked(clickedOverlay, false)) {
        if (!isHappening) {
            print("I'm the event master. I want the event starts");
            isHappening = true;
            setEntityCustomData("myKey", testEntity, {valueToCheck: true});
            
        } else {
            print("I want the event stops");
            isHappening = false;
            setEntityCustomData("myKey", testEntity, {valueToCheck: false});
            
        }
    } 
}


function cleanup() {
    toolBar.cleanup();
    Entities.callEntityMethod(testEntity, 'clean');      //have to call this before deleting to avoid the JSON warnings
    Entities.deleteEntity(testEntity);
}



 Script.scriptEnding.connect(cleanup);
 Controller.mousePressEvent.connect(mousePressEvent);