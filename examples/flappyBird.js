//
//  flappyBird.js
//
//  Created by Clement 3/2/16
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Constants



// Class definitions
function Game() {
	// public methods
	this.start = function() {
		if (!isRunning) {
			setup();
			Script.update.connect(idle);
		}
	};
	
	this.stop = function() {
		if (isRunning) {
			Script.update.disconnect(idle);
			cleanup();
		}
	};

	// Private game state
	var that = this;
	var isRunning = false;

	// Game loop setup
	function idle(deltaTime) {
		inputs();
		update(deltaTime);
		draw();
	};

	// Private methods
	function setup() {
		print("setup");
	};
	function inputs() {
		print("inputs");
	};
	function update(deltaTime) {
		print("update: " + deltaTime);
	};
	function draw() {
		print("draw");
	};
	function cleanup() {
		print("cleanup");
	};
}

// Script logic
function scriptStarting() {
	var game = new Game();

	Script.scriptEnding.connect(function() {
		game.stop();
	});
	game.start();
}

scriptStarting();