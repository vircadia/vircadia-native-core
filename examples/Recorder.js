//
//  Recorder.js
//  examples
//
//  Created by Clément Brisset on 8/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("toolBars.js");

var recordingFile = "recording.rec";
var playFromCurrentLocation = true;

var windowDimensions = Controller.getViewportDimensions();
var TOOL_ICON_URL = "http://s3-us-west-1.amazonaws.com/highfidelity-public/images/tools/";
var ALPHA_ON = 1.0;
var ALPHA_OFF = 0.7;
var COLOR_ON = { red: 128, green: 0, blue: 0 };
var COLOR_OFF = { red: 128, green: 128, blue: 128 };
var COLOR_TOOL_BAR = { red: 0, green: 0, blue: 0 };

var toolBar = null;
var recordIcon;
var playIcon;
var playLoopIcon;
var saveIcon;
var loadIcon;
var spacing;
var timerOffset;
setupToolBar();

var timer = null;
setupTimer();

var watchStop = false;

function setupToolBar() {
	if (toolBar != null) {
		print("Multiple calls to Recorder.js:setupToolBar()");
		return;
	}
    Tool.IMAGE_HEIGHT /= 2;
    Tool.IMAGE_WIDTH /= 2;
    
	toolBar = new ToolBar(0, 0, ToolBar.HORIZONTAL);
    
    toolBar.setBack(COLOR_TOOL_BAR, ALPHA_OFF);
    
    recordIcon = toolBar.addTool({
                                 imageURL: TOOL_ICON_URL + "recording-record.svg",
                                 subImage: { x: 0, y: 0, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
                                 x: 0, y: 0,
                                 width: Tool.IMAGE_WIDTH,
                                 height: Tool.IMAGE_HEIGHT,
                                 alpha: MyAvatar.isPlaying() ? ALPHA_OFF : ALPHA_ON,
                                 visible: true
                                 }, true, !MyAvatar.isRecording());
    
    var playLoopWidthFactor = 1.65;
    playIcon = toolBar.addTool({
                               imageURL: TOOL_ICON_URL + "play-pause.svg",
                               width: playLoopWidthFactor * Tool.IMAGE_WIDTH,
                               height: Tool.IMAGE_HEIGHT,
                               alpha: (MyAvatar.isRecording() || MyAvatar.playerLength() === 0) ? ALPHA_OFF : ALPHA_ON,
                               visible: true
                               }, false);
    
    playLoopIcon = toolBar.addTool({
                                   imageURL: TOOL_ICON_URL + "play-and-loop.svg",
                                   subImage: { x: 0, y: 0, width: playLoopWidthFactor * Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
                                   width: playLoopWidthFactor * Tool.IMAGE_WIDTH,
                                   height: Tool.IMAGE_HEIGHT,
                                   alpha: (MyAvatar.isRecording() || MyAvatar.playerLength() === 0) ? ALPHA_OFF : ALPHA_ON,
                                   visible: true
                                   }, false);
    
    timerOffset = toolBar.width;
    spacing = toolBar.addSpacing(0);
    
    saveIcon = toolBar.addTool({
                               imageURL: TOOL_ICON_URL + "recording-save.svg",
                               width: Tool.IMAGE_WIDTH,
                               height: Tool.IMAGE_HEIGHT,
                               alpha: (MyAvatar.isRecording() || MyAvatar.isPlaying() || MyAvatar.playerLength() === 0) ? ALPHA_OFF : ALPHA_ON,
                               visible: true
                               }, false);
    
    loadIcon = toolBar.addTool({
                               imageURL: TOOL_ICON_URL + "recording-upload.svg",
                               width: Tool.IMAGE_WIDTH,
                               height: Tool.IMAGE_HEIGHT,
                               alpha: (MyAvatar.isRecording() || MyAvatar.isPlaying()) ? ALPHA_OFF : ALPHA_ON,
                               visible: true
                               }, false);
}

function setupTimer() {
	timer = Overlays.addOverlay("text", {
		font: { size: 15 },
		text: (0.00).toFixed(3),
		backgroundColor: COLOR_OFF,
		x: 0, y: 0,
		width: 0,
		height: 0,
		alpha: 1.0,
		visible: true
	});
}

function updateTimer() {
	var text = "";
	if (MyAvatar.isRecording()) {
		text = formatTime(MyAvatar.recorderElapsed());
        
	} else {
		text = formatTime(MyAvatar.playerElapsed()) + " / " +
					 formatTime(MyAvatar.playerLength());
	}

	Overlays.editOverlay(timer, {
		text: text
	})
    toolBar.changeSpacing(text.length * 8 + ((MyAvatar.isRecording()) ? 15 : 0), spacing);
}

function formatTime(time) {
	var MIN_PER_HOUR = 60;
	var SEC_PER_MIN = 60;
	var MSEC_PER_SEC = 1000;

	var hours = Math.floor(time / (MSEC_PER_SEC * SEC_PER_MIN * MIN_PER_HOUR));
	time -= hours * (MSEC_PER_SEC * SEC_PER_MIN * MIN_PER_HOUR);

	var minutes = Math.floor(time / (MSEC_PER_SEC * SEC_PER_MIN));
	time -= minutes * (MSEC_PER_SEC * SEC_PER_MIN);

	var seconds = Math.floor(time / MSEC_PER_SEC);
	seconds = time / MSEC_PER_SEC;

	var text = "";
	text += (hours > 0) ? hours + ":" :
												"";
	text += (minutes > 0) ? ((minutes < 10 && text != "") ? "0" : "") + minutes + ":" :
													"";
	text += ((seconds < 10 && text != "") ? "0" : "") + seconds.toFixed(3);
	return text;
}

function moveUI() {
	var relative = { x: 70, y: 40 };
	toolBar.move(relative.x,
							 windowDimensions.y - relative.y);
	Overlays.editOverlay(timer, {
		x: relative.x + timerOffset - ToolBar.SPACING,
		y: windowDimensions.y - relative.y - ToolBar.SPACING
	});
}

function mousePressEvent(event) {
	clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });
    
    if (recordIcon === toolBar.clicked(clickedOverlay, false) && !MyAvatar.isPlaying()) {
        if (!MyAvatar.isRecording()) {
            MyAvatar.startRecording();
            toolBar.selectTool(recordIcon, false);
            toolBar.setAlpha(ALPHA_OFF, playIcon);
            toolBar.setAlpha(ALPHA_OFF, playLoopIcon);
            toolBar.setAlpha(ALPHA_OFF, saveIcon);
            toolBar.setAlpha(ALPHA_OFF, loadIcon);
        } else {
            MyAvatar.stopRecording();
            toolBar.selectTool(recordIcon, true );
            MyAvatar.loadLastRecording();
            toolBar.setAlpha(ALPHA_ON, playIcon);
            toolBar.setAlpha(ALPHA_ON, playLoopIcon);
            toolBar.setAlpha(ALPHA_ON, saveIcon);
            toolBar.setAlpha(ALPHA_ON, loadIcon);
        }
    } else if (playIcon === toolBar.clicked(clickedOverlay) && !MyAvatar.isRecording()) {
        if (MyAvatar.isPlaying()) {
            MyAvatar.stopPlaying();
            toolBar.setAlpha(ALPHA_ON, recordIcon);
            toolBar.setAlpha(ALPHA_ON, saveIcon);
            toolBar.setAlpha(ALPHA_ON, loadIcon);
        } else if (MyAvatar.playerLength() > 0) {
            MyAvatar.setPlayFromCurrentLocation(playFromCurrentLocation);
            MyAvatar.setPlayerLoop(false);
            MyAvatar.startPlaying();
            toolBar.setAlpha(ALPHA_OFF, recordIcon);
            toolBar.setAlpha(ALPHA_OFF, saveIcon);
            toolBar.setAlpha(ALPHA_OFF, loadIcon);
            watchStop = true;
        }
    } else if (playLoopIcon === toolBar.clicked(clickedOverlay) && !MyAvatar.isRecording()) {
        if (MyAvatar.isPlaying()) {
            MyAvatar.stopPlaying();
            toolBar.setAlpha(ALPHA_ON, recordIcon);
            toolBar.setAlpha(ALPHA_ON, saveIcon);
            toolBar.setAlpha(ALPHA_ON, loadIcon);
        } else if (MyAvatar.playerLength() > 0) {
            MyAvatar.setPlayFromCurrentLocation(playFromCurrentLocation);
            MyAvatar.setPlayerLoop(true);
            MyAvatar.startPlaying();
            toolBar.setAlpha(ALPHA_OFF, recordIcon);
            toolBar.setAlpha(ALPHA_OFF, saveIcon);
            toolBar.setAlpha(ALPHA_OFF, loadIcon);
        }
    } else if (saveIcon === toolBar.clicked(clickedOverlay)) {
        if (!MyAvatar.isRecording() && !MyAvatar.isPlaying() && MyAvatar.playerLength() != 0) {
            recordingFile = Window.save("Save recording to file", ".", "Recordings (*.hfr)");
            if (!(recordingFile === "null" || recordingFile === null || recordingFile === "")) {
				MyAvatar.saveRecording(recordingFile);
            }
        }
    } else if (loadIcon === toolBar.clicked(clickedOverlay)) {
        if (!MyAvatar.isRecording() && !MyAvatar.isPlaying()) {
            recordingFile = Window.browse("Load recorcding from file", ".", "Recordings (*.hfr *.rec *.HFR *.REC)");
            if (!(recordingFile === "null" || recordingFile === null || recordingFile === "")) {
                MyAvatar.loadRecording(recordingFile);
            }
            if (MyAvatar.playerLength() > 0) {
                toolBar.setAlpha(ALPHA_ON, playIcon);
                toolBar.setAlpha(ALPHA_ON, playLoopIcon);
                toolBar.setAlpha(ALPHA_ON, saveIcon);
            }
        }
    } else {
        
    }
}

function update() {
	var newDimensions = Controller.getViewportDimensions();
	if (windowDimensions.x != newDimensions.x ||
			windowDimensions.y != newDimensions.y) {
		windowDimensions = newDimensions;
		moveUI();
	}

	updateTimer();
    
    if (watchStop && !MyAvatar.isPlaying()) {
        watchStop = false;
        toolBar.setAlpha(ALPHA_ON, recordIcon);
        toolBar.setAlpha(ALPHA_ON, saveIcon);
        toolBar.setAlpha(ALPHA_ON, loadIcon);
    }
}

function scriptEnding() {
	if (MyAvatar.isRecording()) {
		MyAvatar.stopRecording();
	}
	if (MyAvatar.isPlaying()) {
		MyAvatar.stopPlaying();
	}
	toolBar.cleanup();
	Overlays.deleteOverlay(timer);
}

Controller.mousePressEvent.connect(mousePressEvent);
Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);

// Should be called last to put everything into position
moveUI();


