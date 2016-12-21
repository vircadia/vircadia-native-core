print("============= Script Starting =============");

if (!Function.prototype.bind) {
  Function.prototype.bind = function(oThis) {
    if (typeof this !== 'function') {
      // closest thing possible to the ECMAScript 5
      // internal IsCallable function
      throw new TypeError('Function.prototype.bind - what is trying to be bound is not callable');
    }

    var aArgs   = Array.prototype.slice.call(arguments, 1),
        fToBind = this,
        fNOP    = function() {},
        fBound  = function() {
          return fToBind.apply(this instanceof fNOP
                 ? this
                 : oThis,
                 aArgs.concat(Array.prototype.slice.call(arguments)));
        };

    if (this.prototype) {
      // Function.prototype doesn't have a prototype property
      fNOP.prototype = this.prototype;
    }
    fBound.prototype = new fNOP();

    return fBound;
  };
}


// Utility functions
function parseJSON(json) {
    try {
        return JSON.parse(json);
    } catch (e) {
        return undefined;
    }
}

function findSurfaceBelowPosition(pos) {
    var result = Entities.findRayIntersection({
        origin: pos,
        direction: { x: 0, y: -1, z: 0 }
    });
    if (result.intersects) {
        return result.intersection;
    }
    return pos;
}
// End of utility function



var GAME_STATES = {
    IDLE: 0,
    PLAYING: 1,
    GAME_OVER: 2,
};

var COMM_CHANNEL_NAME = 'wavegame';
var entityIDs = [];

function GameManager(gatePosition, roofPosition, spawnPositions, startButtonID) {
    this.gameState = GAME_STATES.IDLE;
    this.spawnPositions = spawnPositions;
    this.gatePosition = gatePosition;
    this.roofPosition = roofPosition;
    this.startButtonID = startButtonID;

    // Gameplay state
    this.waveNumber = 0;
    this.livesLeft = 5;
    this.score = 0;
    this.nextWaveTimer = null;
    this.spawnEnemyTimers = [];
    this.enemyIDs = [];
    this.entityIDs = [];
}
GameManager.prototype = {
    cleanup: function() {
        for (var i = 0; i < this.entityIDs.length; i++) {
            Entities.deleteEntity(this.entityIDs[i]);
        }
        this.entityIDs = [];
    },
    startGame: function() {
        if (this.gameState !== GAME_STATES.IDLE) {
            print("playWaveGameManager.js | Error, trying to start game when not in idle state");
            return;
        }

        print("Game started!!");

        Entities.editEntity(this.startButtonID, { visible: false });

        // Initialize game state
        this.waveNumber = 0;
        this.score = 0;
        this.livesLeft = 5;

        this.nextWaveTimer = Script.setTimeout(this.startNextWave.bind(this), 100);
        this.spawnEnemyTimers = [];
        this.checkEnemyPositionsTimer = null;
        this.enemyIDs = [];

        this.gameState = GAME_STATES.PLAYING;
        this.checkEnemyPositionsTimer = Script.setInterval(this.checkForEscapedEnemies.bind(this), 100);

        // Spawn bows
        const bowPosition = Vec3.sum(this.roofPosition, { x: 0, y: 1, z: 0 });
        for (var j = 0; j < 4; ++j) {
            this.entityIDs.push(Entities.addEntity({
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
                    "y": -1,
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
    },
    startNextWave: function() {
        print("Starting next wave");
        this.waveNumber++;
        this.enemyIDs = [];
        const numberOfEnemiesToSpawn = this.waveNumber * 2;
        print("Number to spawn:", numberOfEnemiesToSpawn, this.waveNumber);
        for (var i = 0; i < numberOfEnemiesToSpawn; ++i) {
            Vec3.print(i, this.spawnPositions[i % this.spawnPositions.length]);
            // Spawn enemy
            var enemyID = Entities.addEntity({
                name: "WG.Enemy",
                type: "Box",
                position: this.spawnPositions[i % this.spawnPositions.length],
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
            });
            this.enemyIDs.push(enemyID);
        }
    },
    checkWaveComplete: function() {
        if (this.gameState !== GAME_STATES.PLAYING) {
            return;
        }

        if (this.enemyIDs.length == 0) {
            Script.setTimeout(this.startNextWave.bind(this), 1000);
        }
    },
    checkForEscapedEnemies: function() {
        var enemiesEscaped = false;
        for (var i = this.enemyIDs.length - 1; i >= 0; --i) {
            var position = Entities.getEntityProperties(this.enemyIDs[i], 'position').position;
            if (position && position.z < this.gatePosition.z) {
                Entities.deleteEntity(this.enemyIDs[i]);
                this.enemyIDs.splice(i, 1);
                this.livesLeft--;
                enemiesEscaped = true;
            }
        }
        print("LIVES LEFT: ", this.livesLeft);
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

        Entities.editEntity(this.startButtonID, { visible: true });

        this.gameState = GAME_STATES.GAME_OVER;
        print("GAME OVER");

        // Cleanup
        Script.clearTimeout(this.nextWaveTimer);
        this.nextWaveTimer = null;
        for (var i = 0; i < this.spawnEnemyTimers.length; ++i) {
            Script.clearTimeout(this.spawnEnemyTimers[i]);
            this.spawnEnemyTimers = [];
        }
        Script.clearInterval(this.checkEnemyPositionsTimer);
        this.checkEnemyPositionsTimer = null;

        Script.setTimeout(function() {
            this.gameState = GAME_STATES.IDLE;
        }.bind(this), 3000);
    },
    onEnemyKilled: function(entityID) {
        if (this.gameState !== GAME_STATES.PLAYING) {
            return;
        }

        var idx = this.enemyIDs.indexOf(entityID);
        if (idx >= 0) {
            this.enemyIDs.splice(idx, 1);

            // Update score
            this.score += 100;
            print("SCORE: ", this.score);

            this.checkWaveComplete();
        }
    },
//    onEnemyEscaped: function(entityID) {
//        if (this.gameState !== GAME_STATES.PLAYING) {
//            return;
//        }
//
//        this.livesLeft--;
//        print("LIVES LEFT: ", this.livesLeft);
//        if (this.livesLeft <= 0) {
//            this.endGame();
//            return true;
//        }
//        return false;
//    }
};

function createLocalGame() {
    const rootPosition = findSurfaceBelowPosition(MyAvatar.position);

    // Create start button
    var buttonProperties = {
        type: 'Box',
        name: 'WG.StartButton',
        position: rootPosition,
        dimensions: { x: 1, y: 1, z: 1 },
        color: { red: 0, green: 255, blue: 0 },
        script: Script.resolvePath("startGameButton.js"),
        userData: JSON.stringify({
            grabbableKey: { 
                wantsTrigger: true
            },
            gameChannel: COMM_CHANNEL_NAME
        }),
    }
    var buttonID = Entities.addEntity(buttonProperties);
    entityIDs.push(buttonID);


    // Generate goal that the enemies try to get to
    const goalPosition = Vec3.sum(rootPosition, { x: 0, y: -10, z: -20 });
    const BASES_HEIGHT = 16;
    const BASES_SIZE = 15;
    const BASES_WIDTH = 20;
    const BASES_DEPTH = 15;
    const ROOF_HEIGHT = 0.2;
    var arenaProperties = {
        name: 'WG.Arena.goal',
        type: 'Box',
        position: goalPosition,
        registrationPoint: { x: 0.5, y: 0, z: 0.5 },
        dimensions: { x: BASES_SIZE, y: BASES_HEIGHT, z: BASES_SIZE },
        color: { red: 255, green: 255, blue: 255 },
        script: Script.resolvePath("warpToTopEntity.js"),
        visible: false,
        collisionless: true,
        userData: JSON.stringify({
            gameChannel: COMM_CHANNEL_NAME,
        })
    };
    // Base block
    var arenaID = Entities.addEntity(arenaProperties);
    entityIDs.push(arenaID);

    // Generate platform to shoot from
    goalPosition.y += BASES_HEIGHT - ROOF_HEIGHT;
    const roofPosition = goalPosition;
    var roofProperties = {
        name: 'WG.Roof',
        type: 'Box',
        position: goalPosition,
        registrationPoint: { x: 0.5, y: 0, z: 0.5 },
        dimensions: { x: BASES_SIZE, y: ROOF_HEIGHT, z: BASES_SIZE },
        color: { red: 255, green: 255, blue: 255 },
        script: Script.resolvePath('roofEntity.js'),
        userData: JSON.stringify({
            gameChannel: COMM_CHANNEL_NAME,
        })
    }
    var roofID = Entities.addEntity(roofProperties)
    entityIDs.push(roofID);

    // Generate positions that the enemies spawn from. spawnOffsets is a list
    // of spawn position relative to rootPosition.
    const spawnOffsets = [
        { x: -7.5, y: 0, z: 10 },
        { x: -2.5, y: 0, z: 10 },
        { x: 2.5, y: 0, z: 10 },
        { x: 7.5, y: 0, z: 10 },
    ];
    var spawnPositions = [];
    for (var i = 0; i < spawnOffsets.length; ++i) {
        const spawnPosition = Vec3.sum(rootPosition, spawnOffsets[i]);
        var spawnID = Entities.addEntity({
            name: 'WG.Spawn',
            type: 'Box',
            position: spawnPosition,
            registrationPoint: { x: 0.5, y: 0, z: 0.5 },
            dimensions: { x: 0.5, y: 0.5, z: 0.5},
            color: { red: 255, green: 0, blue: 0 },
        });
        entityIDs.push(spawnID);
        spawnPositions.push(spawnPosition);
    }

    var goalPositionFront = Vec3.sum(goalPosition, { x: 0, y: 0, z: BASES_SIZE / 2 });
    return new GameManager(goalPositionFront, roofPosition, spawnPositions, buttonID);
}

function createACGame() {
    // TODO
    throw("AC not implemented");
}


// Setup game
var gameManager;
if (this.EntityViewer !== undefined) {
    gameManager = createACGame();
} else {
    gameManager = createLocalGame();
}

Messages.subscribe(COMM_CHANNEL_NAME);
Messages.messageReceived.connect(function(channel, messageJSON, senderID) {
    print("playWaveGame.js | Recieved: " + messageJSON + " from " + senderID);
    if (channel === COMM_CHANNEL_NAME) {
        var message = parseJSON(messageJSON);
        if (message === undefined) {
            print("playWaveGame.js | Received non-json message");
            return;
        }
        switch (message.type) {
            case 'start-game':
                gameManager.startGame();
                break;
            case 'enemy-killed':
                gameManager.onEnemyKilled(message.entityID);
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
