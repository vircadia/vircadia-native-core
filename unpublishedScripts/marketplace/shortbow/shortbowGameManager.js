//
//  Created by Ryan Huffman on 1/10/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* globals ShortbowGameManager:true, utils */

Script.include('utils.js');

// +--------+      +-----------+      +-----------------+
// |        |      |           |<-----+                 |
// |  IDLE  +----->|  PLAYING  |      |  BETWEEN_WAVES  |
// |        |      |           +----->|                 |
// +--------+      +-----+-----+      +-----------------+
//      ^                |
//      |                v
//      |         +-------------+
//      |         |             |
//      +---------+  GAME_OVER  |
//                |             |
//                +-------------+
var GAME_STATES = {
    IDLE: 0,
    PLAYING: 1,
    BETWEEN_WAVES: 2,
    GAME_OVER: 3
};

// Load the sounds that we will be using in the game so they are ready to be
// used when we need them.
var BEGIN_BUILDING_SOUND = SoundCache.getSound(Script.resolvePath("sounds/gameOn.wav"));
var GAME_OVER_SOUND = SoundCache.getSound(Script.resolvePath("sounds/gameOver.wav"));
var WAVE_COMPLETE_SOUND = SoundCache.getSound(Script.resolvePath("sounds/waveComplete.wav"));
var EXPLOSION_SOUND = SoundCache.getSound(Script.resolvePath("sounds/explosion.wav"));
var TARGET_HIT_SOUND = SoundCache.getSound(Script.resolvePath("sounds/targetHit.wav"));
var ESCAPE_SOUND = SoundCache.getSound(Script.resolvePath("sounds/escape.wav"));

const STARTING_NUMBER_OF_LIVES = 6;
const ENEMIES_PER_WAVE_MULTIPLIER = 2;
const POINTS_PER_KILL = 100;
const ENEMY_SPEED = 3.0;

// Encode a set of key-value pairs into a param string. Does NOT do any URL escaping.
function encodeURLParams(params) {
    var paramPairs = [];
    for (var key in params) {
        paramPairs.push(key + "=" + params[key]);
    }
    return paramPairs.join("&");
}

function sendAndUpdateHighScore(entityID, score, wave, numPlayers, onResposeReceived) {
    const URL = 'https://script.google.com/macros/s/AKfycbwbjCm9mGd1d5BzfAHmVT_XKmWyUYRkjCEqDOKm1368oM8nqWni/exec';
    print("Sending high score");

    const paramString = encodeURLParams({
        entityID: entityID,
        score: score,
        wave: wave,
        numPlayers: numPlayers
    });

    var request = new XMLHttpRequest();
    request.onreadystatechange = function() {
        print("ready state: ", request.readyState, request.status, request.readyState === request.DONE, request.response);
        if (request.readyState === request.DONE && request.status === 200) {
            print("Got response for high score: ", request.response);
            var response = JSON.parse(request.responseText);
            if (response.highScore !== undefined) {
                onResposeReceived(response.highScore);
            }
        }
    };
    request.open('GET', URL + "?" + paramString);
    request.timeout = 10000;
    request.send();
}

function findChildrenWithName(parentID, name) {
    var childrenIDs = Entities.getChildrenIDs(parentID);
    var matchingIDs = [];
    for (var i = 0; i < childrenIDs.length; ++i) {
        var id = childrenIDs[i];
        var childName = Entities.getEntityProperties(id, 'name').name;
        if (childName === name) {
            matchingIDs.push(id);
        }
    }
    return matchingIDs;
}

function getPropertiesForEntities(entityIDs, desiredProperties) {
    var properties = [];
    for (var i = 0; i < entityIDs.length; ++i) {
        properties.push(Entities.getEntityProperties(entityIDs[i], desiredProperties));
    }
    return properties;
}


var baseEnemyProperties = {
    "name": "SB.Enemy",
    "damping": 0,
    "linearDamping": 0,
    "angularDamping": 0,
    "acceleration": {
        "x": 0,
        "y": -9,
        "z": 0
    },
    "angularVelocity": {
        "x": -0.058330666273832321,
        "y": -0.77943277359008789,
        "z": -2.1163818836212158
    },
    "clientOnly": 0,
    "collisionsWillMove": 1,
    "dimensions": {
        "x": 0.63503998517990112,
        "y": 0.63503998517990112,
        "z": 0.63503998517990112
    },
    "dynamic": 1,
    "gravity": {
        "x": 0,
        "y": -15,
        "z": 0
    },
    "lifetime": 30,
    "id": "{ed8f7339-8bbd-4750-968e-c3ceb9d64721}",
    "modelURL": Script.resolvePath("models/Amber.baked.fbx"),
    "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
    "queryAACube": {
        "scale": 1.0999215841293335,
        "x": -0.54996079206466675,
        "y": -0.54996079206466675,
        "z": -0.54996079206466675
    },
    "shapeType": "sphere",
    "type": "Model",
    "script": Script.resolvePath('enemyClientEntity.js'),
    "serverScripts": Script.resolvePath('enemyServerEntity.js')
};

function searchForChildren(parentID, names, callback, timeoutMs) {
    // Map from name to entity ID for the children that have been found
    var foundEntities = {};
    for (var i = 0; i < names.length; ++i) {
        foundEntities[names[i]] = null;
    }

    const CHECK_EVERY_MS = 500;
    const maxChecks = Math.ceil(timeoutMs / CHECK_EVERY_MS);

    var check = 0;
    var intervalID = Script.setInterval(function() {
        check++;

        var childrenIDs = Entities.getChildrenIDs(parentID);
        print("\tNumber of children:", childrenIDs.length);

        for (var i = 0; i < childrenIDs.length; ++i) {
            print("\t\t" + i + ".", Entities.getEntityProperties(childrenIDs[i]).name);
            var id = childrenIDs[i];
            var name = Entities.getEntityProperties(id, 'name').name;
            var idx = names.indexOf(name);
            if (idx > -1) {
                foundEntities[name] = id;
                print(name, id);
                names.splice(idx, 1);
            }
        }

        if (names.length === 0 || check >= maxChecks) {
            Script.clearInterval(intervalID);
            callback(foundEntities);
        }
    }, CHECK_EVERY_MS);
}

ShortbowGameManager = function(rootEntityID, bowPositions, spawnPositions) {
    print("Starting game manager");
    var self = this;

    this.gameState = GAME_STATES.IDLE;

    this.rootEntityID = rootEntityID;
    this.bowPositions = bowPositions;
    this.rootPosition = null;
    this.spawnPositions = spawnPositions;

    this.loadedChildren = false;

    const START_BUTTON_NAME = 'SB.StartButton';
    const WAVE_DISPLAY_NAME = 'SB.DisplayWave';
    const SCORE_DISPLAY_NAME = 'SB.DisplayScore';
    const LIVES_DISPLAY_NAME = 'SB.DisplayLives';
    const HIGH_SCORE_DISPLAY_NAME = 'SB.DisplayHighScore';

    const SEARCH_FOR_CHILDREN_TIMEOUT = 5000;

    searchForChildren(rootEntityID, [
        START_BUTTON_NAME,
        WAVE_DISPLAY_NAME,
        SCORE_DISPLAY_NAME,
        LIVES_DISPLAY_NAME,
        HIGH_SCORE_DISPLAY_NAME
    ], function(children) {
        self.loadedChildren = true;
        self.startButtonID = children[START_BUTTON_NAME];
        self.waveDisplayID = children[WAVE_DISPLAY_NAME];
        self.scoreDisplayID = children[SCORE_DISPLAY_NAME];
        self.livesDisplayID = children[LIVES_DISPLAY_NAME];
        self.highScoreDisplayID = children[HIGH_SCORE_DISPLAY_NAME];

        sendAndUpdateHighScore(self.rootEntityID, self.score, self.waveNumber, 1, self.setHighScore.bind(self));

        self.reset();
    }, SEARCH_FOR_CHILDREN_TIMEOUT);

    // Gameplay state
    this.waveNumber = 0;
    this.livesLeft = STARTING_NUMBER_OF_LIVES;
    this.score = 0;
    this.nextWaveTimer = null;
    this.spawnEnemyTimers = [];
    this.remainingEnemies = [];
    this.bowIDs = [];

    this.startButtonChannelName = 'button-' + this.rootEntityID;

    // Entity client and server scripts will send messages to this channel
    this.commChannelName = "shortbow-" + this.rootEntityID;
    Messages.subscribe(this.commChannelName);
    Messages.messageReceived.connect(this, this.onReceivedMessage);
    print("Listening on: ", this.commChannelName);
    Messages.sendMessage(this.commChannelName, 'hi');
};
ShortbowGameManager.prototype = {
    reset: function() {
        Entities.editEntity(this.startButtonID, { visible: true });
    },
    cleanup: function() {
        Messages.unsubscribe(this.commChannelName);
        Messages.messageReceived.disconnect(this, this.onReceivedMessage);

        if (this.checkEnemiesTimer) {
            Script.clearInterval(this.checkEnemiesTimer);
            this.checkEnemiesTimer = null;
        }

        for (var i = this.bowIDs.length - 1; i >= 0; i--) {
            Entities.deleteEntity(this.bowIDs[i]);
        }
        this.bowIDs = [];
        for (i = 0; i < this.remainingEnemies.length; i++) {
            Entities.deleteEntity(this.remainingEnemies[i].id);
        }
        this.remainingEnemies = [];

        this.gameState = GAME_STATES.IDLE;
    },
    startGame: function() {
        if (this.gameState !== GAME_STATES.IDLE) {
            print("shortbowGameManagerManager.js | Error, trying to start game when not in idle state");
            return;
        }

        if (this.loadedChildren === false) {
            print('shortbowGameManager.js | Children have not loaded, not allowing game to start');
            return;
        }

        print("Game started!!");

        this.rootPosition = Entities.getEntityProperties(this.rootEntityID, 'position').position;

        Entities.editEntity(this.startButtonID, { visible: false });

        // Spawn bows
        var bowSpawnEntityIDs = findChildrenWithName(this.rootEntityID, 'SB.BowSpawn');
        var bowSpawnProperties = getPropertiesForEntities(bowSpawnEntityIDs, ['position', 'rotation']);
        for (var i = 0; i < bowSpawnProperties.length; ++i) {
            const props = bowSpawnProperties[i];
            Vec3.print("Creating bow: " + i, props.position);
            this.bowIDs.push(Entities.addEntity({
                "position": props.position,
                "rotation": props.rotation,
                "collisionsWillMove": 1,
                "compoundShapeURL": Script.resolvePath("bow/models/bow_collision_hull.obj"),
                "created": "2016-09-01T23:57:55Z",
                "dimensions": {
                    "x": 0.039999999105930328,
                    "y": 1.2999999523162842,
                    "z": 0.20000000298023224
                },
                "dynamic": 1,
                "gravity": {
                    "x": 0,
                    "y": -9.8,
                    "z": 0
                },
                "modelURL": Script.resolvePath("bow/models/bow-deadly.baked.fbx"),
                "name": "WG.Hifi-Bow",
                "script": Script.resolvePath("bow/bow.js"),
                "shapeType": "compound",
                "type": "Model",
                "userData": "{\"grabbableKey\":{\"grabbable\":true},\"wearable\":{\"joints\":{\"RightHand\":[{\"x\":0.0813,\"y\":0.0452,\"z\":0.0095},{\"x\":-0.3946,\"y\":-0.6604,\"z\":0.4748,\"w\":-0.4275}],\"LeftHand\":[{\"x\":-0.0881,\"y\":0.0259,\"z\":0.0159},{\"x\":0.4427,\"y\":-0.6519,\"z\":0.4592,\"w\":0.4099}]}}}"
            }));
        }

        // Initialize game state
        this.waveNumber = 0;
        this.setScore(0);
        this.setLivesLeft(STARTING_NUMBER_OF_LIVES);

        this.nextWaveTimer = Script.setTimeout(this.startNextWave.bind(this), 100);
        this.spawnEnemyTimers = [];
        this.checkEnemiesTimer = null;
        this.remainingEnemies = [];

        // SpawnQueue is a list of enemies left to spawn. Each entry looks like:
        //
        //   { spawnAt: 1000, position: { x: 0, y: 0, z: 0 } }
        //
        // where spawnAt is the number of millseconds after the start of the wave
        // to spawn the enemy. The list is sorted by spawnAt, ascending.
        this.spawnQueue = [];

        this.gameState = GAME_STATES.BETWEEN_WAVES;

        Audio.playSound(BEGIN_BUILDING_SOUND, {
            volume: 1.0,
            position: this.rootPosition
        });
		
		var liveChecker = setInterval(function() {
			if (this.livesLeft <= 0) {
				this.endGame();
				clearInterval(liveChecker);
			}
		}, 1000);
    },
    startNextWave: function() {
        if (this.gameState !== GAME_STATES.BETWEEN_WAVES) {
            return;
        }

        print("Starting next wave");
        this.gameState = GAME_STATES.PLAYING;
        this.waveNumber++;
        this.remainingEnemies= [];
        this.spawnQueue = [];
        this.spawnStartTime = Date.now();

        Entities.editEntity(this.waveDisplayID, { text: this.waveNumber});

        var numberOfEnemiesLeftToSpawn = this.waveNumber * ENEMIES_PER_WAVE_MULTIPLIER;
        var delayBetweenSpawns = 2000 / Math.max(1, Math.log(this.waveNumber));
        var currentDelay = 2000;

        print("Number of enemies:", numberOfEnemiesLeftToSpawn);
        this.checkEnemiesTimer = Script.setInterval(this.checkEnemies.bind(this), 100);

        var enemySpawnEntityIDs = findChildrenWithName(this.rootEntityID, 'SB.EnemySpawn');
        var enemySpawnProperties = getPropertiesForEntities(enemySpawnEntityIDs, ['position', 'rotation']);

        for (var i = 0; i < numberOfEnemiesLeftToSpawn; ++i) {
            print("Adding enemy");
            var idx = Math.floor(Math.random() * enemySpawnProperties.length);
            var props = enemySpawnProperties[idx];
            this.spawnQueue.push({
                spawnAt: currentDelay,
                position: props.position,
                rotation: props.rotation,
                velocity: Vec3.multiply(ENEMY_SPEED, Quat.getFront(props.rotation))

            });
            currentDelay += delayBetweenSpawns;
        }

        print("Starting wave", this.waveNumber);

    },
    checkWaveComplete: function() {
        if (this.gameState !== GAME_STATES.PLAYING) {
            return;
        }

        if (this.spawnQueue.length === 0 && this.remainingEnemies.length === 0) {
            this.gameState = GAME_STATES.BETWEEN_WAVES;
            Script.setTimeout(this.startNextWave.bind(this), 5000);

            Script.clearInterval(this.checkEnemiesTimer);
            this.checkEnemiesTimer = null;

            // Play after 1.5s to let other sounds finish playing
            var self = this;
            Script.setTimeout(function() {
                Audio.playSound(WAVE_COMPLETE_SOUND, {
                    volume: 1.0,
                    position: self.rootPosition
                });
            }, 1500);
        }
    },
    setHighScore: function(highScore) {
        print("Setting high score to:", this.highScoreDisplayID, highScore);
        Entities.editEntity(this.highScoreDisplayID, { text: highScore });
    },
    setLivesLeft: function(lives) {
        lives = Math.max(0, lives);
        this.livesLeft = lives;
        Entities.editEntity(this.livesDisplayID, { text: this.livesLeft });
    },
    setScore: function(score) {
        this.score = score;
        Entities.editEntity(this.scoreDisplayID, { text: this.score });
    },
    checkEnemies: function() {
        if (this.gameState !== GAME_STATES.PLAYING) {
            return;
        }

        // Check the spawn queueu to see if there are any enemies that need to
        // be spawned
        var waveElapsedTime = Date.now() - this.spawnStartTime;
        while (this.spawnQueue.length > 0 && waveElapsedTime > this.spawnQueue[0].spawnAt) {
            baseEnemyProperties.position = this.spawnQueue[0].position;
            baseEnemyProperties.rotation = this.spawnQueue[0].rotation;
            baseEnemyProperties.velocity= this.spawnQueue[0].velocity;

            baseEnemyProperties.userData = JSON.stringify({
                gameChannel: this.commChannelName,
                grabbableKey: {
                    grabbable: false
                }
            });

            var entityID = Entities.addEntity(baseEnemyProperties);
            this.remainingEnemies.push({
                id: entityID,
                lastKnownPosition: baseEnemyProperties.position,
                lastHeartbeat: Date.now()
            });
            this.spawnQueue.splice(0, 1);
            Script.setTimeout(function() {
                const JUMP_SPEED = 5.0;
                var velocity = Entities.getEntityProperties(entityID, 'velocity').velocity;
                velocity.y += JUMP_SPEED;
                Entities.editEntity(entityID, { velocity: velocity });

            }, 500 + Math.random() * 4000);
        }

        // Check the list of remaining enemies to see if any are too far away
        // or haven't been heard from in awhile - if so, delete them.
        var enemiesEscaped = false;
        const MAX_UNHEARD_TIME_BEFORE_DESTROYING_ENTITY_MS = 5000;
        const MAX_DISTANCE_FROM_GAME_BEFORE_DESTROYING_ENTITY = 200;
        for (var i = this.remainingEnemies.length - 1; i >= 0; --i) {
            var enemy = this.remainingEnemies[i];
            var timeSinceLastHeartbeat = Date.now() - enemy.lastHeartbeat;
            var distance = Vec3.distance(enemy.lastKnownPosition, this.rootPosition);
            if (timeSinceLastHeartbeat > MAX_UNHEARD_TIME_BEFORE_DESTROYING_ENTITY_MS
                || distance > MAX_DISTANCE_FROM_GAME_BEFORE_DESTROYING_ENTITY) {

                print("EXPIRING: ", enemy.id);
                Entities.deleteEntity(enemy.id);
                this.remainingEnemies.splice(i, 1);
				// Play the sound when you hit an enemy
                Audio.playSound(TARGET_HIT_SOUND, {
                    volume: 1.0,
                    position: this.rootPosition
                });
                this.setScore(this.score + POINTS_PER_KILL);
                enemiesEscaped = true;
            }
        }

        if (enemiesEscaped) {
            this.checkWaveComplete();
        }
    },
    endGame: function() {
        if (this.gameState !== GAME_STATES.PLAYING) {
            return;
        }

        var self = this;
        Script.setTimeout(function() {
            Audio.playSound(GAME_OVER_SOUND, {
                volume: 1.0,
                position: self.rootPosition
            });
        }, 1500);

        this.gameState = GAME_STATES.GAME_OVER;
        print("GAME OVER");

        // Update high score
        sendAndUpdateHighScore(this.rootEntityID, this.score, this.waveNumber, 1, this.setHighScore.bind(this));

        // Cleanup
        Script.clearTimeout(this.nextWaveTimer);
        this.nextWaveTimer = null;
        var i;
        for (i = 0; i < this.spawnEnemyTimers.length; ++i) {
            Script.clearTimeout(this.spawnEnemyTimers[i]);
        }
        this.spawnEnemyTimers = [];

        Script.clearInterval(this.checkEnemiesTimer);
        this.checkEnemiesTimer = null;


        for (i = this.bowIDs.length - 1; i >= 0; i--) {
            var id = this.bowIDs[i];
            print("Checking bow: ", id);
            var userData = utils.parseJSON(Entities.getEntityProperties(id, 'userData').userData);
            var bowIsHeld = userData.grabKey !== undefined && userData.grabKey !== undefined && userData.grabKey.refCount > 0;
            print("Held: ", bowIsHeld);
            if (!bowIsHeld) {
                Entities.deleteEntity(id);
                this.bowIDs.splice(i, 1);
            }
        }

        for (i = 0; i < this.remainingEnemies.length; i++) {
            Entities.deleteEntity(this.remainingEnemies[i].id);
        }
        this.remainingEnemies = [];

        // Wait a short time before showing the start button so that any current sounds
        // can finish playing.
        const WAIT_TO_REENABLE_GAME_TIMEOUT_MS = 3000;
        Script.setTimeout(function() {
            Entities.editEntity(this.startButtonID, { visible: true });
            this.gameState = GAME_STATES.IDLE;
        }.bind(this), WAIT_TO_REENABLE_GAME_TIMEOUT_MS);
    },
    onReceivedMessage: function(channel, messageJSON, senderID) {
        if (channel === this.commChannelName) {
            var message = utils.parseJSON(messageJSON);
            if (message === undefined) {
                print("shortbowGameManager.js | Received non-json message:", JSON.stringify(messageJSON));
                return;
            }
            switch (message.type) {
                case 'start-game':
                    this.startGame();
                    break;
                case 'enemy-killed':
                    this.onEnemyKilled(message.entityID, message.position);
                    break;
                case 'enemy-escaped':
                    this.onEnemyEscaped(message.entityID);
                    break;
                case 'enemy-heartbeat':
                    this.onEnemyHeartbeat(message.entityID, message.position);
                    break;
                default:
                    print("shortbowGameManager.js | Ignoring unknown message type: ", message.type);
                    break;
            }
        }
    },
    onEnemyKilled: function(entityID, position) {
		if (this.gameState !== GAME_STATES.PLAYING) {
            return;
        }
		
        for (var i = this.remainingEnemies.length - 1; i >= 0; --i) {
            var enemy = this.remainingEnemies[i];
            if (enemy.id === entityID) {
                this.remainingEnemies.splice(i, 1);
                Audio.playSound(TARGET_HIT_SOUND, {
                    volume: 1.0,
                    position: this.rootPosition
                });
                // Update score
                this.setScore(this.score + POINTS_PER_KILL);
                print("SCORE: ", this.score);

                this.checkWaveComplete();
                break;
            }
        }
    },
    onEnemyEscaped: function(entityID, position) {
        if (this.gameState !== GAME_STATES.PLAYING) {
            return;
        }

        var enemiesEscaped = false;
        for (var i = this.remainingEnemies.length - 1; i >= 0; --i) {
            var enemy = this.remainingEnemies[i];
            if (enemy.id === entityID) {
				
                Entities.deleteEntity(enemy.id);
                this.remainingEnemies.splice(i, 1);
                this.setLivesLeft(this.livesLeft - 1);
                Audio.playSound(ESCAPE_SOUND, {
                    volume: 1.0,
                    position: this.rootPosition
                });
                enemiesEscaped = true;
            }
        }
        if (this.livesLeft <= 0) {
            this.endGame();
        } else if (enemiesEscaped) {
            this.checkWaveComplete();
        }
    },
    onEnemyHeartbeat: function(entityID, position) {
        for (var i = 0; i < this.remainingEnemies.length; i++) {
            var enemy = this.remainingEnemies[i];
            if (enemy.id === entityID) {
                enemy.lastHeartbeat = Date.now();
                enemy.lastKnownPosition = position;
                break;
            }
        }
    }
};
