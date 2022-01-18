//
//  flappyAvatars.js
//  examples/toybox
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
    function Avatar(DEFAULT_X, DEFAULT_Y, rotation, to3DPosition) {
        var DIMENSION = 0.15;
        var JUMP_VELOCITY = 1.0;
        var xPosition = DEFAULT_X;
        var color = { red: 0, green: 0, blue: 255 };
        
        var dimensionsSet = false;
        var dimensions = { x: DIMENSION, y: DIMENSION, z: DIMENSION };
        var yPosition = DEFAULT_Y;
        var yVelocity = 0.0;
        var yAcceleration = -G;

        var airSwipeSound = SoundCache.getSound("https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/flappyAvatars/Air%20Swipe%2005.wav");
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
                url: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/flappyAvatars/fly.fbx",
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

            airSwipeSound = SoundCache.getSound("https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/flappyAvatars/8bit%20Jump%2003.wav");
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
            modelURL: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/flappyAvatars/coin.fbx",
            angularVelocity: { x: 0, y: 20, z: 0 },
            position: to3DPosition(this.position()),
            dimensions:dimensions
        });

        this.update = function(deltaTime) {
            xPosition -= deltaTime * velocity;
        }
        this.isColliding = function(avatar) {
            var deltaX = Math.abs(this.position().x - avatar.position().x);
            var deltaY = Math.abs(this.position().Y - avatar.position().Y);
            if (deltaX < (avatar.dimensions().x + dimensions.x) / 2.0 &&
                deltaX < (avatar.dimensions().y + dimensions.y) / 2.0) {
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
            modelURL: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/flappyAvatars/greenPipe.fbx",
            rotation: Quat.fromPitchYawRollDegrees(180, 0, 0),
            position: to3DPosition({ x: xPosition, y: upYPosition }),
            dimensions: { x: width, y: upHeight, z: width }
        });
        var idDown = entityManager.add({
            type: "Model",
            modelURL: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/flappyAvatars/greenPipe.fbx",
            position: to3DPosition({ x: xPosition, y: height / 2.0 }),
            dimensions: { x: width, y: height, z: width }
        });

        this.update = function(deltaTime) {
            xPosition -= deltaTime * velocity;
        }
        this.isColliding = function(avatar) {
            var deltaX = Math.abs(this.position() - avatar.position().x);
            if (deltaX < (avatar.dimensions().z + width) / 2.0) {
                var factor = 0.8;
                var upDistance = (yPosition - upHeight) - (avatar.position().y + avatar.dimensions().y * factor);
                var downDistance = (avatar.position().y - avatar.dimensions().y * factor) - height;
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

        var coinsSound = SoundCache.getSound("https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/flappyAvatars/Coin.wav");
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
        this.isColliding = function(avatar) {
            // Check coin collection
            var collected = -1;
            coins.forEach(function(element, index) {
                if (element.isColliding(avatar)) {
                    element.clear();
                    collected = index;
                    moveScore(1);

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
                isColliding |= element.isColliding(avatar);
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


    function Score(space, bestScore) {
        var score = 0;
        var highScore = bestScore;

        var topOffset = Vec3.multiplyQbyV(space.orientation, { x: -0.1, y: 0.2, z: -0.2 });
        var topLeft = Vec3.sum(space.position, topOffset);
        var bottomOffset = Vec3.multiplyQbyV(space.orientation, { x: -0.1, y: 0.0, z: -0.2 });
        var bottomLeft = Vec3.sum(space.position, bottomOffset);

        var numberDimensions = { x: 0.0660, y: 0.1050, z: 0.0048 };

        function numberUrl(number) {
            return "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/flappyAvatars/" + number + ".fbx"
        }
        function digitPosition(digit) {
            return Vec3.multiplyQbyV(space.orientation, { x: 0.3778 + digit * (numberDimensions.x + 0.01), y: 0.0, z: 0.0 });
        }
        this.score = function() {
            return score;
        }
        this.highScore = function() {
            return highScore;
        }

        var numDigits = 3;

        var bestId = entityManager.add({
            type: "Model",
            modelURL: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/flappyAvatars/best.fbx",
            position: topLeft,
            rotation: Quat.multiply(space.orientation, Quat.fromPitchYawRollDegrees(90, 0, 0)),
            dimensions: { x: 0.2781, y: 0.0063, z: 0.1037 }
        });
        var bestDigitsId = []
        for (var i = 0; i < numDigits; i++) {
            bestDigitsId[i] = entityManager.add({
                type: "Model",
                modelURL: numberUrl(0),
                position: Vec3.sum(topLeft, digitPosition(i)),
                rotation: space.orientation,
                dimensions: numberDimensions
            });
        }

        var scoreId =  entityManager.add({
            type: "Model",
            modelURL: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/flappyAvatars/score.fbx",
            position: bottomLeft,
            rotation: Quat.multiply(space.orientation, Quat.fromPitchYawRollDegrees(90, 0, 0)),
            dimensions: { x: 0.3678, y: 0.0063, z: 0.1037 }
        });
        var scoreDigitsId = []
        for (var i = 0; i < numDigits; i++) {
            scoreDigitsId[i] = entityManager.add({
                type: "Model",
                modelURL: numberUrl(0),
                position: Vec3.sum(bottomLeft, digitPosition(i)),
                rotation: space.orientation,
                dimensions: numberDimensions
            });
        }

        this.moveScore = function(delta) {
            score += delta;
            if (score > highScore) {
                highScore = score;
            }
        }
        this.resetScore = function() {
            score = 0;
        }

        this.draw = function() {
            for (var i = 0; i < numDigits; i++) {
                Entities.editEntity(bestDigitsId[i], { modelURL: numberUrl(Math.floor((highScore / Math.pow(10, numDigits- i - 1)) % 10)) });
            }

            for (var i = 0; i < numDigits; i++) {
                Entities.editEntity(scoreDigitsId[i], { modelURL: numberUrl(Math.floor(score / Math.pow(10, numDigits - i - 1)) % 10) });
            }
        }
    }

    function Game(bestScore) {
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

        var space = null;
        var avatar = null;
        var pipes = null;
        var score = null;

        var gameOverSound = SoundCache.getSound("https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/flappyAvatars/Game%20Over.wav");
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
                avatar.changeModel("https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/flappyAvatars/mario.fbx");
                current = 0;
            }
        }

        var isBoardReset = true;

        function moveScore(delta) {
            score.moveScore(delta);
        }

        this.score = function() {
            return score.score();
        }
        this.highScore = function() {
            return score.highScore();
        }

        function setup() {
            space =    {
                position: getSpacePosition(),
                orientation: getSpaceOrientation(),
                dimensions: getSpaceDimensions()
            }

            var rotation = Quat.multiply(space.orientation, Quat.fromPitchYawRollDegrees(0, 90, 0)); 
            avatar = new Avatar(space.dimensions.x / 2.0, space.dimensions.y / 2.0, rotation, to3DPosition);
            pipes = new Pipes(space.dimensions.x, space.dimensions.y, to3DPosition, moveScore);
            score = new Score(space, bestScore);

            Controller.keyPressEvent.connect(keyPress);
        }
        function inputs(triggerValue) {
            if (!startedPlaying && !isBoardReset && (gameTime - lastLost) > coolDown) {
                score.resetScore();
                avatar.reset();
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
            // Keep entities alive
            entityManager.update(deltaTime);

            if (!startedPlaying && (gameTime - lastLost) < coolDown && !isBoardReset) {
                return;
            }

            // Update Avatar
            if (!startedPlaying && avatar.position().y < spaceDimensions.y / 2.0) {
                isJumping = true;
            }
            // Apply jumps
            if (isJumping) {
                avatar.jump();
                isJumping = false;
            }
            avatar.update(deltaTime);

            pipes.update(deltaTime, gameTime, startedPlaying);

            // Check lost
            var hasLost = avatar.position().y < 0.0 ||
                          avatar.position().y > space.dimensions.y ||
                          pipes.isColliding(avatar);


            // Cleanup
            if (hasLost) {
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
            avatar.draw();
            pipes.draw();
            score.draw();
        }
        function cleanup() {
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
        this.entityID = null;
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


              var bestScore = 0;
            var properties = Entities.getEntityProperties(this.entityID, ["userData"]);
            var userData = JSON.parse(properties.userData);
            if (userData.highScore) {
                bestScore = userData.highScore;
            }

            this.game = new Game(bestScore);
            this.game.start();
        },
        releaseEquip: function(id, params) {
            this.equipped = false;

            var properties = Entities.getEntityProperties(this.entityID, ["userData"]);
            var userData = JSON.parse(properties.userData);
            userData.highScore = this.game.highScore();
            properties.userData = JSON.stringify(userData);
            Entities.editEntity(this.entityID, properties);

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
