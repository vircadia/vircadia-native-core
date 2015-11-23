//
//  recordingMaster.js
//  examples/entityScripts
//
//  Created by Alessandro Signa on 11/12/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Run this script to spawn a box (recorder) and drive the start/end of the recording for anyone who is inside the box
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
Script.include(HIFI_PUBLIC_BUCKET + "scripts/libraries/toolBars.js");
Script.include(HIFI_PUBLIC_BUCKET + "scripts/libraries/utils.js");

var rotation = Quat.safeEulerAngles(Camera.getOrientation());
rotation = Quat.fromPitchYawRollDegrees(0, rotation.y, 0);
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(1, Quat.getFront(rotation)));

var TOOL_ICON_URL = HIFI_PUBLIC_BUCKET + "images/tools/";
var ALPHA_ON = 1.0;
var ALPHA_OFF = 0.7;
var COLOR_TOOL_BAR = { red: 0, green: 0, blue: 0 };
var MASTER_TO_CLIENTS_CHANNEL = "startStopChannel";
var CLIENTS_TO_MASTER_CHANNEL = "resultsChannel";
var START_MESSAGE = "recordingStarted";
var STOP_MESSAGE = "recordingEnded";
var PARTICIPATING_MESSAGE = "participatingToRecording";
var TIMEOUT = 20;

var toolBar = null;
var recordIcon;
var isRecording = false;
var performanceJSON = { "avatarClips" : [] };
var responsesExpected = 0;
var waitingForPerformanceFile = true;
var totalWaitingTime = 0;
var extension = "txt";


Messages.subscribe(CLIENTS_TO_MASTER_CHANNEL);
setupToolBar();

function setupToolBar() {
    if (toolBar != null) {
        print("Multiple calls to setupToolBar()");
        return;
    }
    Tool.IMAGE_HEIGHT /= 2;
    Tool.IMAGE_WIDTH /= 2;
    toolBar = new ToolBar(0, 100, ToolBar.HORIZONTAL);    //put the button in the up-left corner
    toolBar.setBack(COLOR_TOOL_BAR, ALPHA_OFF);
    
    recordIcon = toolBar.addTool({
        imageURL: TOOL_ICON_URL + "recording-record.svg",
        subImage: { x: 0, y: 0, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
        x: 0, y: 0,
        width: Tool.IMAGE_WIDTH,
        height: Tool.IMAGE_HEIGHT,
        alpha: Recording.isPlaying() ? ALPHA_OFF : ALPHA_ON,
        visible: true,
    }, true, isRecording);
}

function mousePressEvent(event) {
    clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });
    if (recordIcon === toolBar.clicked(clickedOverlay, false)) {
        if (!isRecording) {
            print("I'm the master. I want to start recording");
            Messages.sendMessage(MASTER_TO_CLIENTS_CHANNEL, START_MESSAGE);
            isRecording = true;
        } else {
            print("I want to stop recording");
            waitingForPerformanceFile = true;
            Script.update.connect(update);
            Messages.sendMessage(MASTER_TO_CLIENTS_CHANNEL, STOP_MESSAGE);
            isRecording = false;
        }
    }
}

function masterReceivingMessage(channel, message, senderID) {
    if (channel === CLIENTS_TO_MASTER_CHANNEL) {
        print("MASTER received message:" + message );
        if (message === PARTICIPATING_MESSAGE) {
            //increment the counter of all the participants
            responsesExpected++;
        } else if (waitingForPerformanceFile) {
            //I get an atp url from one participant
            performanceJSON.avatarClips[performanceJSON.avatarClips.length] = message;
        }
    }
}

function update(deltaTime) {
    if (waitingForPerformanceFile) {
        totalWaitingTime += deltaTime;
        if (totalWaitingTime > TIMEOUT || performanceJSON.avatarClips.length === responsesExpected) {
            print("UPLOADING PERFORMANCE FILE");
            if (performanceJSON.avatarClips.length !== 0) {
                //I can upload the performance file on the asset
                Assets.uploadData(JSON.stringify(performanceJSON), extension, uploadFinished);
            }
            //clean things after upload performance file to asset
            waitingForPerformanceFile = false;
            responsesExpected = 0;
            totalWaitingTime = 0;
            Script.update.disconnect(update);
            performanceJSON = { "avatarClips" : [] };
        }
    }
}

function uploadFinished(url){
    //need to print somehow the url here this way the master can copy the url
    print("PERFORMANCE FILE URL: " + url);
    Assets.downloadData(url, function (data) {
        printPerformanceJSON(JSON.parse(data));
    });
}

function printPerformanceJSON(obj) {
    print("some info:");
    print("downloaded performance file from asset and examinating its content...");
    var avatarClips = obj.avatarClips;
    avatarClips.forEach(function(param) {
        print("clip url obtained: " + param);
    });
}

function cleanup() {
    toolBar.cleanup();
    Messages.unsubscribe(CLIENTS_TO_MASTER_CHANNEL);
}

Script.scriptEnding.connect(cleanup);
Controller.mousePressEvent.connect(mousePressEvent);
Messages.messageReceived.connect(masterReceivingMessage);