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

var timer = null;
setupTimer();

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
}

function setupTimer() {
	timer = Overlays.addOverlay("text", {
		font: { size: 20 },
		text: (0.00).toFixed(3),
		backgroundColor: COLOR_OFF,
		x: 0, y: 0,
		width: 100,
		height: 100,
		alpha: 1.0,
		visible: true
	});
}

function updateTimer() {
	var text = "";
	if (MyAvatar.isRecording()) {
		text = formatTime(MyAvatar.recorderElapsed())
	} else {
		text = formatTime(MyAvatar.playerElapsed()) + " / " +
					 formatTime(MyAvatar.playerLength());
	}

	Overlays.editOverlay(timer, {
		text: text
	})
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
	var relative = { x: 30, y: 90 };
	toolBar.move(relative.x,
							 windowDimensions.y - relative.y);
	Overlays.editOverlay(timer, {
		x: relative.x - 10,
		y: windowDimensions.y - relative.y - 35,
		width: 0,
		height: 0
	});
}

function mousePressEvent(event) {
	clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });

	print("Status: isPlaying=" + MyAvatar.isPlaying() + ", isRecording=" + MyAvatar.isRecording());

  if (recordIcon === toolBar.clicked(clickedOverlay) && !MyAvatar.isPlaying()) {
  	if (!MyAvatar.isRecording()) {
  		MyAvatar.startRecording();
			toolBar.setBack(COLOR_ON, ALPHA_ON);
  	} else {
  		MyAvatar.stopRecording();
  		MyAvatar.loadLastRecording();
			toolBar.setBack(COLOR_OFF, ALPHA_OFF);
  	}
  } else if (playIcon === toolBar.clicked(clickedOverlay) && !MyAvatar.isRecording()) {
		if (MyAvatar.isPlaying()) {
			MyAvatar.stopPlaying();
		} else {
			MyAvatar.setPlayFromCurrentLocation(true);
			MyAvatar.setPlayerLoop(true);
	  	MyAvatar.startPlaying(true);
	  }
  } else if (saveIcon === toolBar.clicked(clickedOverlay)) {
  	if (!MyAvatar.isRecording()) {
  		recordingFile = Window.save("Save recording to file", ".", "*.rec");
	  	MyAvatar.saveRecording(recordingFile);
	  }
  } else if (loadIcon === toolBar.clicked(clickedOverlay)) {
  	if (!MyAvatar.isRecording()) {
  		recordingFile = Window.browse("Load recorcding from file", ".", "*.rec");
	  	MyAvatar.loadRecording(recordingFile);
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


