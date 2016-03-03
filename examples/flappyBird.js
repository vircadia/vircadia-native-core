//
//  flappyBird.js
//
//  Created by Clement 3/2/16
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
	// Constants
    var TRIGGER_CONTROLS = [
        Controller.Standard.LT,
        Controller.Standard.RT,
    ];

	var G = 4.0;

	Number.prototype.clamp = function(min, max) {
	  return Math.min(Math.max(this, min), max);
	};
	
	var entityManager = new EntityManager();

	// Class definitions
	function Bird(DEFAULT_X, DEFAULT_Y, rotation, to3DPosition) {
		var DIMENSION = 0.15;
		var JUMP_VELOCITY = 1.0;
		var xPosition = DEFAULT_X;
		var color = { red: 0, green: 0, blue: 255 };
		
		var dimensionsSet = false;
		var dimensions = { x: DIMENSION, y: DIMENSION, z: DIMENSION };
		var yPosition = DEFAULT_Y;
		var yVelocity = 0.0;
		var yAcceleration = -G;

	    var airSwipeSound = SoundCache.getSound("https://s3-us-west-1.amazonaws.com/hifi-content/clement/production/Air Swipe 05.wav");
	    var injector = null;

		this.position = function() {
			return { x: xPosition, y: yPosition };
		}
		this.dimensions = function() {
			return dimensions;
		}

		var id = entityManager.add({
			type: "Model",
			modelURL: MyAvatar.skeletonModelURL,
			animation: {
				url: "http://hifi-content.s3.amazonaws.com/ozan/dev/anim/standard_anims_160127/fly.fbx",
				running: true,
				fps: 30,
				firstFrame: 1.0,
				lastFrame: 80.0,
				currentFrame: 1.0,
				loop: true,
				hold: false
			},
			position: to3DPosition(this.position()),
			rotation: rotation,
			dimensions: dimensions
		});

		this.changeModel = function(modelURL) {
			dimensionsSet = false;
			dimensions = { x: 0.10, y: 0.10, z: 0.01 };

			Entities.editEntity(id, {
				modelURL: modelURL,
				rotation: Quat.multiply(rotation, Quat.fromPitchYawRollDegrees(0, -90, 0)),
				dimensions: dimensions,
				animation: {running: false}
			});

	    	airSwipeSound = SoundCache.getSound("https://s3-us-west-1.amazonaws.com/hifi-content/clement/production/8bit Jump 03.wav");
	    	injector = null;
		}

		this.jump = function() {
			yVelocity = JUMP_VELOCITY;

            if (airSwipeSound.downloaded && !injector) {
                injector = Audio.playSound(airSwipeSound, { position: to3DPosition(this.position()), volume: 0.05 });
            } else if (injector) {
            	injector.restart();
            }
		}
		this.update = function(deltaTime) {
			if (!dimensionsSet) {
				var properties = Entities.getEntityProperties(id, ["naturalDimensions"]);
				var naturalDimensions = properties.naturalDimensions;
				if (naturalDimensions.x != 1 || naturalDimensions.y != 1 || naturalDimensions.z != 1) {
					var max = Math.max(naturalDimensions.x, Math.max(naturalDimensions.y, naturalDimensions.z));
					dimensions.x = naturalDimensions.x / max * dimensions.x;
					dimensions.y = naturalDimensions.y / max * dimensions.y;
					dimensions.z = naturalDimensions.z / max * dimensions.z;
					dimensionsSet = true;
					
					Entities.editEntity(id, {
						dimensions: dimensions
					});
				}
			} else {
				dimensions = Entities.getEntityProperties(id, ["dimensions"]).dimensions;
			}

			yPosition += deltaTime * (yVelocity + deltaTime * yAcceleration / 2.0);
			yVelocity += deltaTime * yAcceleration;
		}
		this.draw = function() {
			Entities.editEntity(id, {
				position: to3DPosition(this.position())
			});
		}
		this.reset = function() {
			yPosition = DEFAULT_Y;
			yVelocity = 0.0;
		}
	}

	function Coin(xPosition, yPosition, to3DPosition) {
		var velocity = 0.4;
		var dimensions = { x: 0.0625, y: 0.0625, z: 0.0088 };

		this.position = function() {
			return { x: xPosition, y: yPosition };
		}

		var id = entityManager.add({
			type: "Model",
			modelURL: "https://s3-us-west-1.amazonaws.com/hifi-content/clement/production/coin.fbx",
			angularVelocity: { x: 0, y: 20, z: 0 },
			position: to3DPosition(this.position()),
			dimensions:dimensions
		});

		this.update = function(deltaTime) {
			xPosition -= deltaTime * velocity;
		}
		this.isColliding = function(bird) {
			var deltaX = Math.abs(this.position().x - bird.position().x);
			var deltaY = Math.abs(this.position().Y - bird.position().Y);
			if (deltaX < (bird.dimensions().x + dimensions.x) / 2.0 &&
				deltaX < (bird.dimensions().y + dimensions.y) / 2.0) {
				return true;
			}
			
			return false;
		}
		this.draw = function() {
			Entities.editEntity(id, { position: to3DPosition({ x: xPosition, y: yPosition }) });
		}
		this.clear = function() {
			entityManager.remove(id);
		}
	}

	function Pipe(xPosition, yPosition, height, gap, to3DPosition) {
		var velocity = 0.4;
		var width = 0.1;

		this.position = function() {
			return xPosition;
		}

		var upHeight = yPosition - (height + gap);
		var upYPosition = height + gap + upHeight / 2.0;

		var idUp = entityManager.add({
			type: "Model",
			modelURL: "https://s3-us-west-1.amazonaws.com/hifi-content/clement/production/greenPipe.fbx",
			rotation: Quat.fromPitchYawRollDegrees(180, 0, 0),
			position: to3DPosition({ x: xPosition, y: upYPosition }),
			dimensions: { x: width, y: upHeight, z: width }
		});
		var idDown = entityManager.add({
			type: "Model",
			modelURL: "https://s3-us-west-1.amazonaws.com/hifi-content/clement/production/greenPipe.fbx",
			position: to3DPosition({ x: xPosition, y: height / 2.0 }),
			dimensions: { x: width, y: height, z: width }
		});

		this.update = function(deltaTime) {
			xPosition -= deltaTime * velocity;
		}
		this.isColliding = function(bird) {
			var deltaX = Math.abs(this.position() - bird.position().x);
			if (deltaX < (bird.dimensions().z + width) / 2.0) {
				var upDistance = (yPosition - upHeight) - (bird.position().y + bird.dimensions().y);
				var downDistance = (bird.position().y - bird.dimensions().y) - height;
				if (upDistance <= 0 || downDistance <= 0) {
					return true;
				}
			}
			
			return false;
		}
		this.draw = function() {
			Entities.editEntity(idUp, { position: to3DPosition({ x: xPosition, y: upYPosition }) });
			Entities.editEntity(idDown, { position: to3DPosition({ x: xPosition, y: height / 2.0 }) });
		}
		this.clear = function() {
			entityManager.remove(idUp);
			entityManager.remove(idDown);
		}
	}

	function Pipes(newPipesPosition, newPipesHeight, to3DPosition, moveScore) {
		var lastPipe = 0;
		var pipesInterval = 2.0;

		var pipes = new Array();
		var coins = new Array();

	    var coinsSound = SoundCache.getSound("https://s3-us-west-1.amazonaws.com/hifi-content/clement/production/Coin.wav");
	    var injector = null;

		this.update = function(deltaTime, gameTime, startedPlaying) {
			// Move pipes forward
			pipes.forEach(function(element) {
				element.update(deltaTime);
			});
			// Move coins forward
			coins.forEach(function(element) {
				element.update(deltaTime);
			});
			// Delete pipes over the end
			var count = 0;
			while(count < pipes.length && pipes[count].position() <= 0.0) {
				pipes[count].clear();
				count++;
			}
			if (count > 0) {
				pipes = pipes.splice(count);
			}
			// Delete coins over the end
			count = 0;
			while(count < coins.length && coins[count].position() <= 0.0) {
				coins[count].clear();
				count++;
			}
			if (count > 0) {
				coins = coins.splice(count);
			}
			// Make new pipes and coins
			if (startedPlaying && gameTime - lastPipe > pipesInterval) {
				var min = 0.4;
				var max = 0.7;
				var height = Math.random() * (max - min) + min;
				pipes.push(new Pipe(newPipesPosition, newPipesHeight, height, 0.5, to3DPosition));
				coins.push(new Coin(newPipesPosition, height + 0.5 / 2.0, to3DPosition));
				lastPipe = gameTime;
			}
		}
		this.isColliding = function(bird) {
			// Check coin collection
			var collected = -1;
			coins.forEach(function(element, index) {
				if (element.isColliding(bird)) {
					element.clear();
					collected = index;
					moveScore(10);

		            if (coinsSound.downloaded && !injector) {
		                injector = Audio.playSound(coinsSound, { position: to3DPosition({ x: newPipesPosition, y: newPipesHeight }), volume: 0.1 });
		            } else if (injector) {
		            	injector.restart();
		            }
				}
			});
			if (collected > -1) {
				coins.splice(collected, 1);
			}


			// Check collisions
			var isColliding = false;

			pipes.forEach(function(element) {
				isColliding |= element.isColliding(bird);
			});

			return isColliding;
		}
		this.draw = function() {
			// Drawing pipes
			pipes.forEach(function(element) {
				element.draw();
			});
			// Drawing coins
			coins.forEach(function(element) {
				element.draw();
			});
		}
		this.clear = function() {
			// Clearing pipes
			pipes.forEach(function(element) {
				element.clear();
			});
			pipes = new Array();

			// Clearing coins
			coins.forEach(function(element) {
				element.clear();
			});
			coins = new Array();
		}
	}

	function Game() {
		// public methods
		this.start = function() {
			if (!isRunning) {
				isRunning = true;
				setup();
			}
		}
		
		this.stop = function() {
			if (isRunning) {
				cleanup();
				isRunning = false;
			}
		}

		// Game loop setup
		var timestamp = 0;
		this.idle = function(triggerValue) {
			var now = Date.now();
			var deltaTime = (now - timestamp) / 1000.0;
			if (timestamp === 0) {
				deltaTime = 0;
			}
			gameTime += deltaTime;

			inputs(triggerValue);
			update(deltaTime);
			draw();
			timestamp = now;
		}
		// this.keyPressed = function(event) {
		// 	if (event.text === "SPACE" && (gameTime - lastLost) > coolDown) {
		// 		isJumping = true;
		// 		startedPlaying = true;
		// 	}
		// }

		// Constants
		var spaceDimensions = { x: 2.0, y: 1.5, z: 0.01 };
		var spaceDistance = 1.5;
		var spaceYOffset = 0.6;

		// Private game state
		var that = this;
		var isRunning = false;
		var startedPlaying = false;

		var coolDown = 1.5;
		var lastLost = -coolDown;

		var gameTime = 0;

		var isJumping = false;
		var lastJumpValue = 0.0;
		var lastTriggerValue = 0.0;
		var TRIGGER_THRESHOLD = 0.9;

		var space = null
		var board = null;
		var bird = null;
		var pipes = null;

	    var gameOverSound = SoundCache.getSound("https://s3-us-west-1.amazonaws.com/hifi-content/clement/production/Game Over.wav");
	    var injector = null;

		var directions = ["UP", "DOWN", "LEFT", "RIGHT"];
		var sequence = [directions[0], directions[0], directions[1], directions[1], directions[2], directions[3], directions[2], directions[3], "b", "a"];
		var current = 0;
		function keyPress(event) {
			if (event.text === sequence[current]) {
				++current;
			} else {
				current = 0;
			}
			if (current === sequence.length) {
				print("KONAMI CODE!!!");
				bird.changeModel("https://s3-us-west-1.amazonaws.com/hifi-content/clement/production/mario.fbx");
				current = 0;
			}
		}

		var isBoardReset = true;

		var score = 0;
		function moveScore(delta) {
			score += delta;
		}

		function setup() {
			print("setup");

			space =	{
				position: getSpacePosition(),
				orientation: getSpaceOrientation(),
				dimensions: getSpaceDimensions()
			}

			// board = entityManager.add({
			// 	type: "Box",
			// 	position: space.position,
			// 	rotation: space.orientation,
			// 	dimensions: space.dimensions,
			// 	color: { red: 100, green: 200, blue: 200 }
			// });

			var rotation = Quat.multiply(space.orientation, Quat.fromPitchYawRollDegrees(0, 90, 0)); 
			bird = new Bird(space.dimensions.x / 2.0, space.dimensions.y / 2.0, rotation, to3DPosition);
			pipes = new Pipes(space.dimensions.x, space.dimensions.y, to3DPosition, moveScore);

			Controller.keyPressEvent.connect(keyPress);
		}
		function inputs(triggerValue) {
			if (!startedPlaying && !isBoardReset && (gameTime - lastLost) > coolDown) {
				score = 0;
				bird.reset();
				pipes.clear();

				isBoardReset = true;
			}

			if (triggerValue > TRIGGER_THRESHOLD &&
				lastTriggerValue < TRIGGER_THRESHOLD &&
				 (gameTime - lastLost) > coolDown) {
				isJumping = true;
				startedPlaying = true;
			}
			lastTriggerValue = triggerValue;
		}
		function update(deltaTime) {
			//print("update: " + deltaTime);

			// Keep entities alive
			entityManager.update(deltaTime);

			if (!startedPlaying && (gameTime - lastLost) < coolDown && !isBoardReset) {
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

			pipes.update(deltaTime, gameTime, startedPlaying);

			// Check lost
			var hasLost = bird.position().y < 0.0 ||
						  bird.position().y > space.dimensions.y ||
						  pipes.isColliding(bird);


			// Cleanup
			if (hasLost) {
				print("Game Over!");

	            if (gameOverSound.downloaded && !injector) {
	                injector = Audio.playSound(gameOverSound, { position: space.position, volume: 0.4 });
	            } else if (injector) {
	            	injector.restart();
	            }

				isBoardReset = false;
				startedPlaying = false;
				lastLost = gameTime;
			}
		}
		function draw() {
			//print("draw");
			bird.draw();
			pipes.draw();
		}
		function cleanup() {
			print("cleanup");
			entityManager.removeAll();

			Controller.keyPressEvent.disconnect(keyPress);
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

		function to3DPosition(position) {
			var position2D = {
				x: position.x - space.dimensions.x / 2.0,
				y: position.y - space.dimensions.y / 2.0,
				z: 0.0
			}
			return Vec3.sum(space.position, Vec3.multiplyQbyV(space.orientation, position2D));
		}
	}

	function EntityManager() {
		var OBJECTS_LIFETIME = 1;
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

    PartableGame = function() {
        this.equipped = false;
        this.triggerValue = 0.0;
        this.hand = 0;
        this.game = null;
    };

    PartableGame.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
        },
        unload: function() {
        },
        startEquip: function(id, params) {
            this.equipped = true;
            this.hand = params[0] == "left" ? 0 : 1;

            this.game = new Game();
            this.game.start();
        },
        releaseEquip: function(id, params) {
            this.equipped = false;

            this.game.stop();
            delete this.game;
        },
        continueEquip: function(id, params) {
            if (!this.equipped) {
                return;
            }
            this.triggerValue = Controller.getValue(TRIGGER_CONTROLS[this.hand]);
			this.game.idle(this.triggerValue);
        },
    };

    // entity scripts always need to return a newly constructed object of our type
    return new PartableGame();
});
