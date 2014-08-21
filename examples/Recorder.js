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

var windowDimensions = Controller.getViewportDimensions();
var TOOL_ICON_URL = "http://s3-us-west-1.amazonaws.com/highfidelity-public/images/tools/";
var ALPHA_ON = 1.0;
var ALPHA_OFF = 0.7;
var COLOR_ON = { red: 128, green: 0, blue: 0 };
var COLOR_OFF = { red: 128, green: 128, blue: 128 };
Tool.IMAGE_WIDTH *= 0.7;
Tool.IMAGE_HEIGHT *= 0.7;

var toolBar = null;
var recordIcon;
var playIcon;
var saveIcon;
var loadIcon;
setupToolBar();

function setupToolBar() {
	if (toolBar != null) {
		print("Multiple calls to Recorder.js:setupToolBar()");
		return;
	}

	toolBar = new ToolBar(0, 0, ToolBar.HORIZONTAL);
	toolBar.setBack(COLOR_OFF, ALPHA_OFF);

	recordIcon = toolBar.addTool({
	    imageURL: TOOL_ICON_URL + "record.svg",
	    width: Tool.IMAGE_WIDTH,
	    height: Tool.IMAGE_HEIGHT,
	    alpha: ALPHA_ON,
	    visible: true
	}, false);

	playIcon = toolBar.addTool({
	    imageURL: TOOL_ICON_URL + "play.svg",
	    width: Tool.IMAGE_WIDTH,
	    height: Tool.IMAGE_HEIGHT,
	    alpha: ALPHA_ON,
	    visible: true
	}, false, false);

	saveIcon = toolBar.addTool({
	    imageURL: TOOL_ICON_URL + "save.svg",
	    width: Tool.IMAGE_WIDTH,
	    height: Tool.IMAGE_HEIGHT,
	    alpha: ALPHA_ON,
	    visible: true
	}, false, false);

	loadIcon = toolBar.addTool({
	    imageURL: TOOL_ICON_URL + "load.svg",
	    width: Tool.IMAGE_WIDTH,
	    height: Tool.IMAGE_HEIGHT,
	    alpha: ALPHA_ON,
	    visible: true
	}, false, false);

	moveUI();
}
 
function moveUI() {
	var relative = { x: 30, y: 90 };
	toolBar.move(relative.x,
							 windowDimensions.y - relative.y);
}

function mousePressEvent(event) {
	clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });

  if (recordIcon === toolBar.clicked(clickedOverlay)) {
  	if (!MyAvatar.isRecording()) {
  		MyAvatar.startRecording();
			toolBar.setBack(COLOR_ON, ALPHA_ON);
  	} else {
  		MyAvatar.stopRecording();
  		MyAvatar.loadLastRecording();
			toolBar.setBack(COLOR_OFF, ALPHA_OFF);
  	}
  } else if (playIcon === toolBar.clicked(clickedOverlay)) {
  	if (!MyAvatar.isRecording()) {
  		if (MyAvatar.isPlaying()) {
  			MyAvatar.stopPlaying();
  		} else {
		  	MyAvatar.startPlaying();
		  }
  	}
  } else if (saveIcon === toolBar.clicked(clickedOverlay)) {
  	recordingFile = Window.save("Save recording to file", ".", "*.rec");
  	MyAvatar.saveRecording(recordingFile);
  } else if (loadIcon === toolBar.clicked(clickedOverlay)) {
  	recordingFile = Window.browse("Load recorcding from file", ".", "*.rec");
  	MyAvatar.loadRecording(recordingFile);
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
}

function scriptEnding() {
	if (MyAvatar.isRecording()) {
		MyAvatar.stopRecording();
	}
	if (MyAvatar.isPlaying()) {
		MyAvatar.stopPlaying();
	}
	toolBar.cleanup();
}

Controller.mousePressEvent.connect(mousePressEvent);
Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);




