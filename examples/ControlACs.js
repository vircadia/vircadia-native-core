//
//  ControlACs.js
//  examples
//
//  Created by Clément Brisset on 8/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Set the following variables to the right value
var NUM_AC = 3; // This is the number of AC. Their ID need to be unique and between 0 (included) and NUM_AC (excluded)
var NAMES = new Array("Arnold", "Jeff"); // ACs names ordered by IDs (Default name is "ACx", x = ID + 1)) 

// Those variables MUST be common to every scripts
var controlVoxelSize = 0.25;
var controlVoxelPosition = { x: 2000 , y: 0, z: 0 };

// Script. DO NOT MODIFY BEYOND THIS LINE.
Script.include("toolBars.js");

var DO_NOTHING = 0;
var PLAY = 1;
var PLAY_LOOP = 2;
var STOP = 3;
var SHOW = 4;
var HIDE = 5;

var COLORS = [];
COLORS[PLAY] = { red: PLAY, green: 0,  blue: 0 };
COLORS[PLAY_LOOP] = { red: PLAY_LOOP, green: 0,  blue: 0 };
COLORS[STOP] = { red: STOP, green: 0,  blue: 0 };
COLORS[SHOW] = { red: SHOW, green: 0,  blue: 0 };
COLORS[HIDE] = { red: HIDE, green: 0,  blue: 0 };



var windowDimensions = Controller.getViewportDimensions();
var TOOL_ICON_URL = "http://s3-us-west-1.amazonaws.com/highfidelity-public/images/tools/";
var ALPHA_ON = 1.0;
var ALPHA_OFF = 0.7;
var COLOR_TOOL_BAR = { red: 128, green: 128, blue: 128 };
var COLOR_MASTER = { red: 200, green: 200, blue: 200 };
var TEXT_HEIGHT = 10;

var toolBars = new Array();
var nameOverlays = new Array();
var onOffIcon = new Array();
var playIcon = new Array();
var playLoopIcon = new Array();
var stopIcon = new Array();
setupToolBars();

function setupToolBars() {
	if (toolBars.length > 0) {
		print("Multiple calls to Recorder.js:setupToolBars()");
		return;
	}

	for (i = 0; i <= NUM_AC; i++) {
		toolBars.push(new ToolBar(0, 0, ToolBar.HORIZONTAL));
		nameOverlays.push(Overlays.addOverlay("text", {
																					font: { size: TEXT_HEIGHT },
																					text: (i === NUM_AC) ? "Master" :
																								 ((i < NAMES.length) ? NAMES[i] :
																																			 "AC" + (i + 1)),
																					x: 0, y: 0,
																					width: 0,
																					height: 0,
																					alpha: 1.0,
																					visible: true
																				}));

		onOffIcon.push(toolBars[i].addTool({
		    imageURL: TOOL_ICON_URL + "models-tool.svg",
	      subImage: { x: 0, y: Tool.IMAGE_WIDTH, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
		    width: Tool.IMAGE_WIDTH,
		    height: Tool.IMAGE_HEIGHT,
		    alpha: ALPHA_ON,
		    visible: true
		}, true, false));
		playIcon.push(null);
		playLoopIcon.push(null);
		stopIcon.push(null);
	}
}

function sendCommand(id, action) {
	if (action === SHOW && toolBars[id].numberOfTools() === 1) {
		toolBars[id].selectTool(onOffIcon[id], true);

		playIcon[id] = toolBars[id].addTool({
		    imageURL: TOOL_ICON_URL + "play.svg",
		    width: Tool.IMAGE_WIDTH,
		    height: Tool.IMAGE_HEIGHT,
		    alpha: ALPHA_ON,
		    visible: true
		}, false);

		playLoopIcon[id] = toolBars[id].addTool({
		    imageURL: TOOL_ICON_URL + "play-loop.svg",
		    width: Tool.IMAGE_WIDTH,
		    height: Tool.IMAGE_HEIGHT,
		    alpha: ALPHA_ON,
		    visible: true
		}, false);

		stopIcon[id] = toolBars[id].addTool({
		    imageURL: TOOL_ICON_URL + "stop.svg",
		    width: Tool.IMAGE_WIDTH,
		    height: Tool.IMAGE_HEIGHT,
		    alpha: ALPHA_ON,
		    visible: true
		}, false);

		toolBars[id].setBack((id === NUM_AC) ? COLOR_MASTER : COLOR_TOOL_BAR, ALPHA_OFF);
	} else if (action === HIDE && toolBars[id].numberOfTools() != 1) {
		toolBars[id].selectTool(onOffIcon[id], false);
		toolBars[id].removeLastTool();
		toolBars[id].removeLastTool();
		toolBars[id].removeLastTool();
		toolBars[id].setBack(null);
	}
	
	if (id === toolBars.length - 1) {
		for (i = 0; i < NUM_AC; i++) {
			sendCommand(i, action);
		}
		return;
	}

	Voxels.setVoxel(controlVoxelPosition.x + id * controlVoxelSize,
									controlVoxelPosition.y,
									controlVoxelPosition.z,
									controlVoxelSize,
									COLORS[action].red,
									COLORS[action].green,
									COLORS[action].blue);
}

function mousePressEvent(event) {
	clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });

	// Check master control
	var i = toolBars.length - 1;
	if (onOffIcon[i] === toolBars[i].clicked(clickedOverlay)) {
  	if (toolBars[i].toolSelected(onOffIcon[i])) {
	  	sendCommand(i, SHOW);
  	} else {
	  	sendCommand(i, HIDE);
  	}
  } else if (playIcon[i] === toolBars[i].clicked(clickedOverlay)) {
  	sendCommand(i, PLAY);
  } else if (playLoopIcon[i] === toolBars[i].clicked(clickedOverlay)) {
  	sendCommand(i, PLAY_LOOP);
  } else if (stopIcon[i] === toolBars[i].clicked(clickedOverlay)) {
  	sendCommand(i, STOP);
  } else {
  	// Check individual controls
		for (i = 0; i < NUM_AC; i++) {
			if (onOffIcon[i] === toolBars[i].clicked(clickedOverlay)) {
		  	if (toolBars[i].toolSelected(onOffIcon[i])) {
			  	sendCommand(i, SHOW);
		  	} else {
			  	sendCommand(i, HIDE);
		  	}
		  } else if (playIcon[i] === toolBars[i].clicked(clickedOverlay)) {
		  	sendCommand(i, PLAY);
		  } else if (playLoopIcon[i] === toolBars[i].clicked(clickedOverlay)) {
		  	sendCommand(i, PLAY_LOOP);
		  } else if (stopIcon[i] === toolBars[i].clicked(clickedOverlay)) {
		  	sendCommand(i, STOP);
		  } else {

		  }
		}
  }
}

function moveUI() {
	var relative = { x: 70, y: 400 };
	for (i = 0; i <= NUM_AC; i++) {
		toolBars[i].move(relative.x,
								 		 windowDimensions.y - relative.y +
								 		 i * (Tool.IMAGE_HEIGHT + 2 * ToolBar.SPACING + TEXT_HEIGHT));
		Overlays.editOverlay(nameOverlays[i], {
			x: relative.x,
			y: windowDimensions.y - relative.y +
				 i * (Tool.IMAGE_HEIGHT + 2 * ToolBar.SPACING + TEXT_HEIGHT) -
				 ToolBar.SPACING - 2 * TEXT_HEIGHT
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
	for (i = 0; i <= NUM_AC; i++) {
		toolBars[i].cleanup();
		Overlays.deleteOverlay(nameOverlays[i]);
	}
}

Controller.mousePressEvent.connect(mousePressEvent);
Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);

moveUI();
