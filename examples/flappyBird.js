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
var OBJECTS_LIFETIME = 1;
var G = 4.0;

var entityManager = new EntityManager();

Number.prototype.clamp = function(min, max) {
  return Math.min(Math.max(this, min), max);
};

// Class definitions
function Bird(DEFAULT_X, DEFAULT_Y, to3DPosition) {
	var DIMENSION = 0.05;
	var JUMP_VELOCITY = 1.0;
	var xPosition = DEFAULT_X;
	var dimensions = { x: DIMENSION, y: DIMENSION, z: DIMENSION };
	var color = { red: 0, green: 0, blue: 255 };
	
	var yPosition = DEFAULT_Y;
	var yVelocity = 0.0;
	var yAcceleration = -G;

	this.position = function() {
		return { x: xPosition, y: yPosition };
	}
	this.size = function() {
		return DIMENSION;
	}

	var id = entityManager.add({
		type: "Sphere",
		position: to3DPosition(this.position()),
		dimensions: dimensions,
		color: color
	});


	this.jump = function() {
		yVelocity = JUMP_VELOCITY;
	}
	this.update = function(deltaTime) {
		yPosition += deltaTime * (yVelocity + deltaTime * yAcceleration / 2.0);
		yVelocity += deltaTime * yAcceleration;
	}
	this.draw = function() {
		Entities.editEntity(id, { position: to3DPosition(this.position()) });
	}
	this.reset = function() {
		yPosition = DEFAULT_Y;
		yVelocity = 0.0;
	}
}

function Pipe() {

}


function Game() {
	// public methods
	this.start = function() {
		if (!isRunning) {
			isRunning = true;
			setup();
			Script.update.connect(idle);
		}
	}
	
	this.stop = function() {
		if (isRunning) {
			Script.update.disconnect(idle);
			cleanup();
			isRunning = false;
		}
	}
	this.keyPressed = function(event) {
		if (event.text === "SPACE" && (gameTime - lastLost) > coolDown) {
			isJumping = true;
			startedPlaying = true;
		}
	}

	// Constants
	var spaceDimensions = { x: 1.5, y: 0.8, z: 0.01 };
	var spaceDistance = 1.5;
	var spaceYOffset = 0.6;

	var jumpVelocity = 1.0;

	// Private game state
	var that = this;
	var isRunning = false;
	var startedPlaying = false;

	var coolDown = 1;
	var lastLost = -coolDown;

	var gameTime = 0;

	var isJumping = false;

	var space = null
	var board = null;

	var bird = null;

	var pipes = new Array();
	var lastPipe = 0;
	var pipesInterval = 0.5;
	var pipesVelocity = 1.0;
	var pipeWidth = 0.05;
	var pipeLength = 0.4;

	// Game loop setup
	function idle(deltaTime) {
		inputs();
		update(deltaTime);
		draw();
	}

	function setup() {
		print("setup");

		space =	{
			position: getSpacePosition(),
			orientation: getSpaceOrientation(),
			dimensions: getSpaceDimensions()
		}

		board = entityManager.add({
			type: "Box",
			position: space.position,
			rotation: space.orientation,
			dimensions: space.dimensions,
			color: { red: 100, green: 200, blue: 200 }
		});

		bird = new Bird(space.dimensions.x / 2.0, space.dimensions.y / 2.0, to3DPosition);
	}
	function inputs() {
		//print("inputs");
	}
	function update(deltaTime) {
		//print("update: " + deltaTime);

		// Keep entities alive
		gameTime += deltaTime;
		entityManager.update(deltaTime);

		if (!startedPlaying && (gameTime - lastLost) < coolDown) {
			return;
		}

		// Update Bird
		if (!startedPlaying && bird.position().y < spaceDimensions.y / 2.0) {
			isJumping = true;
		}
		// Apply jumps
		if (isJumping) {
			bird.jump();
			isJumping = false;
		}
		bird.update(deltaTime);


		// Move pipes forward
		pipes.forEach(function(element) {
			element.position -= deltaTime * pipesVelocity;
		});
		// Delete pipes over the end
		var count = 0;
		while(count < pipes.length && pipes[count].position <= 0.0) {
			entityManager.remove(pipes[count].id);
			count++;
		}
		if (count > 0) {
			pipes = pipes.splice(count);
		}
		// Move pipes forward
		if (startedPlaying && gameTime - lastPipe > pipesInterval) {
			print("New pipe");
			var newPipe = entityManager.add({
				type: "Box",
				position: to3DPosition({ x: space.dimensions.x, y: 0.2 }),
				dimensions: { x: pipeWidth, y: pipeLength, z: pipeWidth },
				color: { red: 0, green: 255, blue: 0 }
			});
			pipes.push({ id: newPipe, position: space.dimensions.x });
			lastPipe = gameTime;
		}


		// Check lost
		var hasLost = bird.position().y < 0.0 || bird.position().y > space.dimensions.y;
		if (!hasLost) {
			pipes.forEach(function(element) {
				var deltaX = Math.abs(element.position - bird.position().x);
				if (deltaX < (bird.size() + pipeWidth) / 2.0) {
					var deltaY = bird.position().y - pipeLength;
					if (deltaY < 0 || deltaY < bird.size() / 2.0) {
						hasLost = true;
					}
				}
			});
		}

		// Cleanup
		if (hasLost) {
			bird.reset()
			startedPlaying = false;
			lastLost = gameTime;


			// Clearing pipes
			print("Clearing pipes: " + pipes.length);
			pipes.forEach(function(element) {
				entityManager.remove(element.id);
			});
			pipes = new Array();
		}
	}
	function draw() {
		//print("draw");
		bird.draw();

		pipes.forEach(function(element) {
			Entities.editEntity(element.id, { position: to3DPosition({ x: element.position, y: 0.2 }) });
		});
	}
	function cleanup() {
		print("cleanup");
		entityManager.removeAll();
	}

	// Private methods
	function getSpacePosition() {
		var forward = Vec3.multiplyQbyV(MyAvatar.orientation, Vec3.FRONT);
		var spacePosition = Vec3.sum(MyAvatar.position, Vec3.multiply(spaceDistance, forward));
		return Vec3.sum(spacePosition, Vec3.multiply(spaceYOffset, Vec3.UP));
	}
	function getSpaceOrientation() {
		return MyAvatar.orientation;
	}
	function getSpaceDimensions() {
		return spaceDimensions;
	}



	function project(point, plane) {
		var v = Vec3.subtract(point, plane.origin);
		var dist = Vec3.dot(v, plane.normal);
		return Vec3.subtract(point, Vec3.multiply(dist, v));
	}
	function to3DPosition(position) {
		var position2D = {
			x: position.x - space.dimensions.x / 2.0,
			y: position.y - space.dimensions.y / 2.0,
			z: 0.0
		}
		return Vec3.sum(space.position, Vec3.multiplyQbyV(space.orientation, position2D));
	}
	function to2DPosition(position) {
		var position3D = project(position, {
			origin: Vec3.subtract(space.position, {
				x: space.dimensions.x / 2.0,
				y: space.dimensions.y / 2.0,
				z: 0.0
			}),
			normal: Vec3.multiplyQbyV(space.orientation, Vec3.FRONT)
		});

		var position2D = {
			x: position3D.x.clamp(0.0, space.dimensions.x),
			y: position3D.y.clamp(0.0, space.dimensions.y)
		}
		return position2D;
	}

}

function EntityManager() {
	var entities = new Array();
	var lifetime = OBJECTS_LIFETIME;

	this.setLifetime = function(newLifetime) {
		lifetime = newLifetime;
		this.update();
	}
	this.add = function(properties) {
		// Add to scene
		properties.lifetime = lifetime;
		var entityID = Entities.addEntity(properties);
		// Add to array
		entities.push({ id: entityID, properties: properties });

		return entityID;
	}
	this.update = function(deltaTime) {
		entities.forEach(function(element) {
			// Get entity's age
			var properties = Entities.getEntityProperties(element.id, ["age"]);
			// Update entity's lifetime
			Entities.editEntity(element.id, { lifetime: properties.age + lifetime });
		});
	}
	this.remove = function(entityID) {
		// Remove from scene
		Entities.deleteEntity(entityID);

		// Remove from array
		entities = entities.filter(function(element) {
			return element.id !== entityID;
		});
	}
	this.removeAll = function() {
		// Remove all from scene
		entities.forEach(function(element) {
			Entities.deleteEntity(element.id);
		});
		// Remove all from array
		entities = new Array();
	}
}

// Script logic
function scriptStarting() {
	var game = new Game();

	Controller.keyPressEvent.connect(function(event) {
		game.keyPressed(event);
	});
	Script.scriptEnding.connect(function() {
		game.stop();
	});
	game.start();
}

scriptStarting();