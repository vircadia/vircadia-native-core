print("============= Script Starting =============");

var BEGIN_BUILDING_SOUND = SoundCache.getSound(Script.resolvePath("assets/sounds/gameOn.wav"));
var GAME_OVER_SOUND = SoundCache.getSound(Script.resolvePath("assets/sounds/gameOver.wav"));
var WAVE_COMPLETE_SOUND = SoundCache.getSound(Script.resolvePath("assets/sounds/waveComplete.wav"));
var EXPLOSION_SOUND = SoundCache.getSound(Script.resolvePath("assets/sounds/explosion.wav"));
var TARGET_HIT_SOUND = SoundCache.getSound(Script.resolvePath("assets/sounds/targetHit.wav"));
var ESCAPE_SOUND = SoundCache.getSound(Script.resolvePath("assets/sounds/escape.wav"));

Script.include('utils.js');
Script.include('shortbow.js?' + Date.now());
var TEMPLATES = SHORTBOW_ENTITIES.Entities;
print(utils.parseJSON);
print(utils.findSurfaceBelowPosition);

// Merge two objects into a new object. If a key name appears in both a and b,
// the value in a will be used.
function mergeObjects(a, b) {
    var obj = {};
    for (var key in b) {
        obj[key] = b[key];
    }
    for (var key in a) {
        obj[key] = a[key];
    }
    return obj;
}

function spawnTemplate(templateName, overrides) {
    var template = getTemplate(templateName);
    if (template === null) {
        print("ERROR, unknown template name:", templateName);
        return null;
    }
    print("Spawning: ", templateName);
    var properties = mergeObjects(overrides, template);
    return Entities.addEntity(properties);
}

function getTemplate(name) {
    for (var i = 0; i < TEMPLATES.length; ++i) {
        if (TEMPLATES[i].name == name) {
            return TEMPLATES[i];
        }
    }
    return null;
}

// Cleanup ShortBow template data
var scoreboardTemplate = getTemplate('SB.Scoreboard');
Vec3.print("Scoreboard:", scoreboardTemplate.position);
for (var i = 0; i < TEMPLATES.length; ++i) {
    if (TEMPLATES[i].name !== scoreboardTemplate.name) {
        var template = TEMPLATES[i];

        // Fixup position
        template.localPosition = Vec3.subtract(template.position, scoreboardTemplate.position);
        delete template.position;
    }

    // Fixup model url
    if (template.type == "Model") {
        var urlParts = template.modelURL.split("/");
        var filename = urlParts[urlParts.length - 1];
        var newURL = Script.resolvePath("assets/" + filename);
        print("Updated url", template.modelURL, "to", newURL);
        template.modelURL = newURL;
    }
}


var GAME_STATES = {
    IDLE: 0,
    PLAYING: 1,
    BETWEEN_WAVES: 2,
    GAME_OVER: 3,
};

var COMM_CHANNEL_NAME = 'wavegame';
var entityIDs = [];

var baseEnemyProperties = {
    name: "WG.Enemy",
    type: "Box",
    registrationPoint: { x: 0.5, y: 0, z: 0.5 },
    dimensions: { x: 0.7, y: 0.7, z: 0.7 },
    velocity: {
        x: 0,
        y: 0,
        z: -3
    },
    dynamic: true,
    gravity: {
        x: 0,
        y: -10,
        z: 0,
    },
    restitution: 0,
    friction: 0,
    damping: 0,
    linearDamping: 0,
    lifetime: 100,
    script: Script.resolvePath('enemyEntity.js'),
    userData: JSON.stringify({
        gameChannel: COMM_CHANNEL_NAME,
    })
}
var baseEnemyProperties = {
    damping: 0,
    linearDamping: 0,
    angularDamping: 0,
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
    "id": "{ed8f7339-8bbd-4750-968e-c3ceb9d64721}",
    "modelURL": "http://hifi-content.s3.amazonaws.com/Examples%20Content/production/marblecollection/Amber.fbx?2",
    "name": "SB.Enemy",
    "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
    "queryAACube": {
        "scale": 1.0999215841293335,
        "x": -0.54996079206466675,
        "y": -0.54996079206466675,
        "z": -0.54996079206466675
    },
    //"restitution": 0.99000000953674316,
    "rotation": {
        "w": 0.52459806203842163,
        "x": 0.3808099627494812,
        "y": -0.16060420870780945,
        "z": 0.74430292844772339
    },
    "shapeType": "sphere",
    "type": "Model",
    velocity: {
        x: 0,
        y: 0,
        z: -3
    },
    script: Script.resolvePath('enemyEntity.js'),
    userData: JSON.stringify({
        gameChannel: COMM_CHANNEL_NAME,
        "grabbableKey": {
            "grabbable": false
        }
    }),
};

// Encode a set of key-value pairs into a param string. Does NOT do any URL escaping.
function encodeURLParams(params) {
    var paramPairs = [];
    for (var key in params) {
        paramPairs.push(key + "=" + params[key]);
    }
    return paramPairs.join("&");
}

function sendAndUpdateHighScore(highScoreDisplayID, entityID, score, wave, numPlayers) {
    const URL = 'https://script.google.com/macros/s/AKfycbwbjCm9mGd1d5BzfAHmVT_XKmWyUYRkjCEqDOKm1368oM8nqWni/exec';
    print("Sending high score");

    const paramString = encodeURLParams({
        entityID: entityID,
        score: score,
        wave: wave,
        numPlayers: numPlayers
    });

    var req = new XMLHttpRequest();
    req.onreadystatechange = function() {
        print("ready state: ", req.readyState, req.status, req.readyState === req.DONE, req.response);
        if (req.readyState === req.DONE && req.status === 200) {
            print("Got response for high score: ", req.response);
            var response = JSON.parse(req.responseText);
            if (response.highScore !== undefined) {
                Entities.editEntity(highScoreDisplayID, {
                    text: response.highScore
                });
            }
        }
    };
    req.open('GET', URL + "?" + paramString);
    req.timeout = 10000;
    req.send();
}

// The method of checking the local entity for the high score is currently disabled.
// As of 1/9/2017 we don't have support for getting nearby entity data in server entity scripts,
// so until then we have to rely on a remote source to store and retrieve that information.
function getHighScoreFromDisplay(entityID) {
    var highScore = parseInt(Entities.getEntityProperties(entityID, 'text').text);
    print("High score is: ", entityID, highScore);
    if (highScore === NaN) {
        return -1;
    }
    return highScore;
}
function setHighScoreOnDisplay(entityID, highScore) {
    print("Setting high score to: ", entityID, highScore);
    Entities.editEntity(entityID, {
        text: highScore
    });
}


function GameManager(rootPosition, gatePosition, bowPositions, spawnPositions, rootEntityID, startButtonID, waveDisplayID, scoreDisplayID, livesDisplayID, highScoreDisplayID) {
    this.gameState = GAME_STATES.IDLE;

    this.bowPositions = bowPositions;
    this.rootPosition = rootPosition;
    this.spawnPositions = spawnPositions;
    this.gatePosition = gatePosition;
    this.rootEntityID = rootEntityID;
    this.startButtonID = startButtonID;
    this.waveDisplayID = waveDisplayID;
    this.scoreDisplayID = scoreDisplayID;
    this.livesDisplayID = livesDisplayID;
    this.highScoreDisplayID = highScoreDisplayID;

    // Gameplay state
    this.waveNumber = 0;
    this.livesLeft = 5;
    this.score = 0;
    this.nextWaveTimer = null;
    this.spawnEnemyTimers = [];
    this.enemyIDs = [];
    this.entityIDs = [];
    this.bowIDs = [];

    sendAndUpdateHighScore(this.highScoreDisplayID, this.rootEntityID, this.score + 10, this.waveNumber, 1);
}
GameManager.prototype = {
    cleanup: function() {
        for (var i = 0; i < this.entityIDs.length; i++) {
            Entities.deleteEntity(this.entityIDs[i]);
        }
        this.entityIDs = [];
        for (var i = this.bowIDs.length - 1; i >= 0; i--) {
            Entities.deleteEntity(this.bowIDs[i]);
        }
        this.bowIDs = [];
        for (var i = 0; i < this.enemyIDs.length; i++) {
            Entities.deleteEntity(this.enemyIDs[i]);
        }
        this.enemyIDs = [];
    },
    startGame: function() {
        if (this.gameState !== GAME_STATES.IDLE) {
            print("playWaveGameManager.js | Error, trying to start game when not in idle state");
            return;
        }

        print("Game started!!");

        Entities.editEntity(this.startButtonID, { visible: false });


        // Spawn bows
        for (var i = 0; i < this.bowPositions.length; ++i) {
            const bowPosition = this.bowPositions[i];
            Vec3.print("Creating bow: ", bowPosition);
            this.bowIDs.push(Entities.addEntity({
                position: bowPosition,
                "collisionsWillMove": 1,
                "compoundShapeURL": Script.resolvePath("bow/bow_collision_hull.obj"),
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
                "modelURL": Script.resolvePath("bow/bow-deadly.fbx"),
                "name": "WG.Hifi-Bow",
                "rotation": {
                    "w": 0.9718012809753418,
                    "x": 0.15440607070922852,
                    "y": -0.10469216108322144,
                    "z": -0.14418250322341919
                },
                "script": Script.resolvePath("bow/bow.js"),
                "shapeType": "compound",
                "type": "Model",
                "userData": "{\"grabbableKey\":{\"grabbable\":true},\"wearable\":{\"joints\":{\"RightHand\":[{\"x\":0.0813,\"y\":0.0452,\"z\":0.0095},{\"x\":-0.3946,\"y\":-0.6604,\"z\":0.4748,\"w\":-0.4275}],\"LeftHand\":[{\"x\":-0.0881,\"y\":0.0259,\"z\":0.0159},{\"x\":0.4427,\"y\":-0.6519,\"z\":0.4592,\"w\":0.4099}]}}}"
            }));
        }

        // Initialize game state
        this.waveNumber = 0;
        this.setScore(0);
        this.setLivesLeft(6);

        this.nextWaveTimer = Script.setTimeout(this.startNextWave.bind(this), 100);
        this.spawnEnemyTimers = [];
        this.checkEnemyPositionsTimer = null;
        this.enemyIDs = [];

        // SpawnQueue is a list of enemies left to spawn. Each entry looks like:
        //
        //   { spawnAt: 1000, position: { x: 0, y: 0, z: 0 } }
        //
        // where spawnAt is the number of millseconds after the start of the wave
        // to spawn the enemy. The list is sorted by spawnAt, ascending.
        this.spawnQueue = [];

        this.gameState = GAME_STATES.PLAYING;

        Audio.playSound(BEGIN_BUILDING_SOUND, {
            volume: 1.0,
            position: this.rootPosition
        });

    },
    startNextWave: function() {
        print("Starting next wave");
        this.gameState = GAME_STATES.PLAYING;
        this.waveNumber++;
        this.enemyIDs = [];
        this.spawnQueue = [];
        this.spawnStartTime = Date.now();

        Entities.editEntity(this.waveDisplayID, {
            text: this.waveNumber
        });

        var numberOfEnemiesLeftToSpawn = this.waveNumber * 2;
        var delayBetweenSpawns = 2000 / Math.max(1, Math.log(this.waveNumber));
        var currentDelay = 2000;

        print("Number of enemies:", numberOfEnemiesLeftToSpawn);
        this.checkEnemyPositionsTimer = Script.setInterval(this.checkForEscapedEnemies.bind(this), 100);

        for (var i = 0; i < numberOfEnemiesLeftToSpawn; ++i) {
            print("Adding enemy");
            var idx = Math.floor(Math.random() * this.spawnPositions.length);
            this.spawnQueue.push({ spawnAt: currentDelay, position: this.spawnPositions[idx] });
            currentDelay += delayBetweenSpawns;
        }

        print("Starting wave", this.waveNumber);

    },
    checkWaveComplete: function() {
        if (this.gameState !== GAME_STATES.PLAYING) {
            return;
        }

        if (this.spawnQueue.length <= 0 && this.enemyIDs.length === 0) {
            this.gameState = GAME_STATES.BETWEEN_WAVES;
            Script.setTimeout(this.startNextWave.bind(this), 5000);

            Script.clearInterval(this.checkEnemyPositionsTimer);
            this.checkEnemyPositionsTimer = null;

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
    setLivesLeft: function(lives) {
        lives = Math.max(0, lives);
        this.livesLeft = lives;
        Entities.editEntity(this.livesDisplayID, {
            text: this.livesLeft
        });
    },
    setScore: function(score) {
        this.score = score;
        Entities.editEntity(this.scoreDisplayID, {
            text: utils.formatNumberWithCommas(this.score)
        });
    },
    checkSpawnQueue: function() {
        var waveElapsedTime = Date.now() - this.spawnStartTime;
        while (this.spawnQueue.length > 0 && waveElapsedTime > this.spawnQueue[0].spawnAt) {
            baseEnemyProperties.position = this.spawnQueue[0].position;
            var entityID = Entities.addEntity(baseEnemyProperties);
            this.enemyIDs.push(entityID);
            this.spawnQueue.splice(0, 1);
            Script.setTimeout(function() {
                var velocity = Entities.getEntityProperties(entityID, 'velocity').velocity;
                velocity.y += 5;
                Entities.editEntity(entityID, { velocity: velocity });

            }, 500 + Math.random() * 4000);
        }
        //print("Spawn queue size: ", this.spawnQueue.length, "Elapsed time: ", waveElapsedTime, "Number of enemies:", this.enemyIDs.length);
    },
    checkForEscapedEnemies: function() {
        // Move this somewhere else?
        this.checkSpawnQueue();

        var enemiesEscaped = false;
        for (var i = this.enemyIDs.length - 1; i >= 0; --i) {
            var position = Entities.getEntityProperties(this.enemyIDs[i], 'position').position;
            if (position === undefined) {
                // If the enemy can no longer be found, assume it was hit 
                this.enemyIDs.splice(i, 1);
                Audio.playSound(TARGET_HIT_SOUND, {
                    volume: 1.0,
                    position: this.rootPosition,
                });
                this.setScore(this.score + 100);
                enemiesEscaped = true;
            } else if (position.z < this.gatePosition.z) {
                Entities.deleteEntity(this.enemyIDs[i]);
                this.enemyIDs.splice(i, 1);
                this.setLivesLeft(this.livesLeft - 1);
                Audio.playSound(ESCAPE_SOUND, {
                    volume: 1.0,
                    position: this.rootPosition
                });
                enemiesEscaped = true;
            }
        }
        //print("LIVES LEFT: ", this.livesLeft, this.numberOfEntitiesLeftForWave);
        if (this.livesLeft <= 0) {
            this.endGame();
        } else if (enemiesEscaped) {
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

        //Entities.editEntity(this.livesDisplayID, { text: "GAME OVER" });

        this.gameState = GAME_STATES.GAME_OVER;
        print("GAME OVER");


        // Update high score
        sendAndUpdateHighScore(this.highScoreDisplayID, this.rootEntityID, this.score, this.waveNumber, 1);

        //var highScore = getHighScoreFromDisplay(this.highScoreDisplayID);
        //if (this.score > highScore) {
        //    setHighScoreOnDisplay(this.highScoreDisplayID, this.score);
        //} else {
        //    print("Score not higher", this.score, highScore);
        //}


        // Cleanup
        Script.clearTimeout(this.nextWaveTimer);
        this.nextWaveTimer = null;
        for (var i = 0; i < this.spawnEnemyTimers.length; ++i) {
            Script.clearTimeout(this.spawnEnemyTimers[i]);
        }
        this.spawnEnemyTimers = [];

        Script.clearInterval(this.checkEnemyPositionsTimer);
        this.checkEnemyPositionsTimer = null;


        for (var i = this.bowIDs.length - 1; i >= 0; i--) {
            var id = this.bowIDs[i];
            print("Checking bow: ", id);
            var userData = utils.parseJSON(Entities.getEntityProperties(id, 'userData').userData);
            var bowIsHeld = userData.grabKey !== undefined && userData.grabKey !== undefined && userData.grabKey.refCount > 0
            print("Held: ", bowIsHeld);
            if (!bowIsHeld) {
                Entities.deleteEntity(id);
                this.bowIDs.splice(i, 1);
            }
        }

        Script.setTimeout(function() {
            Entities.editEntity(this.startButtonID, { visible: true });
            this.gameState = GAME_STATES.IDLE;
        }.bind(this), 3000);

        for (var i = 0; i < this.enemyIDs.length; i++) {
            Entities.deleteEntity(this.enemyIDs[i]);
        }
        this.enemyIDs = [];
    },
    onEnemyKilled: function(entityID, position) {
        if (this.gameState !== GAME_STATES.PLAYING) {
            return;
        }

        var idx = this.enemyIDs.indexOf(entityID);
        if (idx >= 0) {
            this.enemyIDs.splice(idx, 1);
            Audio.playSound(TARGET_HIT_SOUND, {
                volume: 1.0,
                //position: position,
                position: this.rootPosition,
            });

            // Update score
            this.setScore(this.score + 100);
            print("SCORE: ", this.score);

            this.checkWaveComplete();
        }
    },
};

// TODO: Eventually these will need to be found at runtime by the AC
var rootPosition = null;
var goalPosition = null;
var scoreboardID = null;
var buttonID = null;
var waveDisplayID = null;
var scoreDisplayID = null;
var highScoreDisplayID = null;
var livesDisplayID = null;
function createLocalGame() {
    rootPosition = utils.findSurfaceBelowPosition(MyAvatar.position);
    rootPosition.y += 6.11;

    scoreboardID = spawnTemplate("SB.Scoreboard", {
        position: rootPosition
    });
    entityIDs.push(scoreboardID);

    // Create start button
    buttonID = spawnTemplate("SB.StartButton", {
        parentID: scoreboardID,
        script: Script.resolvePath("startGameButton.js"),
        userData: JSON.stringify({
            grabbableKey: { 
                wantsTrigger: true
            },
            gameChannel: COMM_CHANNEL_NAME
        }),
    });
    entityIDs.push(buttonID);


    // Generate goal that the enemies try to get to
    goalPosition = Vec3.sum(rootPosition, { x: 0, y: -10, z: -20 });
    const BASES_HEIGHT = 16;
    const ROOF_HEIGHT = 0.2;

    goalPosition.y += BASES_HEIGHT - ROOF_HEIGHT;
    const roofPosition = goalPosition;

    waveDisplayID = spawnTemplate("SB.DisplayWave", {
        parentID: scoreboardID
    });
    entityIDs.push(waveDisplayID);

    scoreDisplayID = spawnTemplate("SB.DisplayScore", {
        parentID: scoreboardID
    });
    entityIDs.push(scoreDisplayID);

    livesDisplayID = spawnTemplate("SB.DisplayLives", {
        parentID: scoreboardID
    });
    entityIDs.push(livesDisplayID);

    highScoreDisplayID = spawnTemplate("SB.DisplayHighScore", {
        parentID: scoreboardID
    });
    entityIDs.push(highScoreDisplayID);



}

var gameHasBeenBuilt = false;
function buildGame() {
    const BASES_SIZE = 15;

    // TODO: Generate these when a button is pressed
    var platformID = spawnTemplate("SB.Platform", {
        parentID: scoreboardID
    });
    entityIDs.push(platformID);

    var bowPositions = [];
    var spawnPositions = [];
    for (var i = 0; i < TEMPLATES.length; ++i) {
        var template = TEMPLATES[i];
        if (template.name === "SB.BowSpawn") {
            bowPositions.push(Vec3.sum(rootPosition, template.localPosition));
            Vec3.print("PUshing bow position", Vec3.sum(rootPosition, template.localPosition));
        } else if (template.name === "SB.EnemySpawn") {
            spawnPositions.push(Vec3.sum(rootPosition, template.localPosition));
            Vec3.print("PUshing spawnposition", Vec3.sum(rootPosition, template.localPosition));
        }
    }

    gameHasBeenBuilt = true;

    var goalPositionFront = Vec3.sum(goalPosition, { x: 0, y: 0, z: BASES_SIZE / 2 });
    return new GameManager(rootPosition, goalPositionFront, bowPositions, spawnPositions, platformID, buttonID, waveDisplayID, scoreDisplayID, livesDisplayID, highScoreDisplayID);
}

function createACGame() {
    // TODO
    throw("AC not implemented");
}


// Setup game
var gameManager;
if (this.EntityViewer !== undefined) {
    createACGame();
} else {
    createLocalGame();
}

Messages.subscribe(COMM_CHANNEL_NAME);
Messages.messageReceived.connect(function(channel, messageJSON, senderID) {
    print("playWaveGame.js | Recieved: " + messageJSON + " from " + senderID);
    if (channel === COMM_CHANNEL_NAME) {
        var message = utils.parseJSON(messageJSON);
        if (message === undefined) {
            print("playWaveGame.js | Received non-json message");
            return;
        }
        switch (message.type) {
            case 'build-game':
                break;
            case 'start-game':
                if (gameHasBeenBuilt) {
                    gameManager.startGame();
                } else {
                    gameManager = buildGame();
                }
                break;
            case 'enemy-killed':
                gameManager.onEnemyKilled(message.entityID, message.position);
                break;
            case 'enemy-escaped':
                gameManager.onEnemyEscaped(message.entityID);
                break;
            default:
                print("playWaveGame.js | Ignoring unknown message type: ", message.type);
                break;
        }
    }
});

function cleanup() {
    for (var i = 0; i < entityIDs.length; ++i) {
        Entities.deleteEntity(entityIDs[i]);
    }
    gameManager.cleanup();
}

Script.scriptEnding.connect(cleanup);
