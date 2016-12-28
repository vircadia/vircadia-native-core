print("============= Script Starting =============");

var BEGIN_BUILDING_SOUND = SoundCache.getSound(Script.resolvePath("assets/sounds/letTheGamesBegin.wav"));
var GAME_OVER_SOUND = SoundCache.getSound(Script.resolvePath("assets/sounds/gameOver.wav"));
var WAVE_COMPLETE_SOUND = SoundCache.getSound(Script.resolvePath("assets/sounds/waveComplete.wav"));
var EXPLOSION_SOUND = SoundCache.getSound(Script.resolvePath("assets/sounds/explosion.wav"));

Script.include('utils.js');
print(utils.parseJSON);
print(utils.findSurfaceBelowPosition);

var randomNums = [0.550327773205936,0.9845642729196697,0.0923804109916091,0.02317189099267125,0.3967760754749179,0.31739208032377064,0.12527672364376485,0.7785351350903511,0.5003328016027808,0.7325339165981859,0.9973437152802944,0.5696579017676413,0.3218349125236273,0.31624294770881534,0.1574797066859901,0.607193318894133,0.6456831728573889,0.762436268851161,0.055773367173969746,0.2516116350889206,0.9599523325450718,0.7358639598824084,0.6161336801014841,0.5886498105246574,0.994422614807263,0.7153528861235827,0.31696250033564866,0.7215959236491472,0.4992282949388027,0.913538035703823,0.03025316959246993,0.10350738768465817,0.995452782837674,0.5243359236046672,0.9341779642272741,0.27633642544969916,0.24957748572342098,0.8252806619275361,0.9339258212130517,0.03701223572716117,0.9452723823487759,0.6275276178494096,0.5341641567647457,0.4005412443075329,0.6898896538186818,0.11714944546110928,0.131995229749009,0.1973097778391093,0.8488441850058734,0.6566063927020878,0.8172534313052893,0.7988312132656574,0.27918070019222796,0.7423286908306181,0.3513789263088256,0.418984186835587,0.4163232871796936,0.44792668521404266,0.056147431721910834,0.4671320249326527,0.2205709801055491,0.816504409071058,0.726218594936654,0.48132069990970194,0.33418917655944824,0.2568618259392679,0.7884722277522087,0.19624578021466732,0.24001670349389315,0.05126210208982229,0.8809637068770826,0.08631141181103885,0.83980125002563,0.38965122890658677,0.7475115314591676,0.22781660920009017,0.7292001177556813,0.12802458507940173,0.6163305244408548,0.8507104918826371,0.026970824925228953,0.45111535978503525,0.6384737638290972,0.9973178182262927,0.19859008654020727,0.9765442060306668,0.9542752094566822,0.4875906065572053,0.333038471871987,0.937375855166465,0.7295125185046345,0.9273903956636786,0.9793413057923317,0.613529595779255,0.3908261926844716,0.6914237674791366,0.34551386488601565,0.13964610872790217,0.5300214979797602,0.7015589624643326];
var currentRandomIdx = 0;
function getRandom() {
    return Math.random();
    //currentRandomIdx = randomNums
    //return randomNums[];
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


function GameManager(rootPosition, gatePosition, roofPosition, spawnPositions, startButtonID, waveDisplayID, scoreDisplayID, livesDisplayID) {
    this.gameState = GAME_STATES.IDLE;

    this.rootPosition = rootPosition;
    this.spawnPositions = spawnPositions;
    this.gatePosition = gatePosition;
    this.roofPosition = roofPosition;
    this.startButtonID = startButtonID;
    this.waveDisplayID = waveDisplayID;
    this.scoreDisplayID = scoreDisplayID;
    this.livesDisplayID = livesDisplayID;

    // Gameplay state
    this.waveNumber = 0;
    this.livesLeft = 5;
    this.score = 0;
    this.nextWaveTimer = null;
    this.spawnEnemyTimers = [];
    this.enemyIDs = [];
    this.entityIDs = [];





    // Spawn bows
    const bowPosition = Vec3.sum(this.roofPosition, { x: 0, y: 1, z: 0 });
    for (var j = 0; j < 20; ++j) {
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
}
GameManager.prototype = {
    cleanup: function() {
        for (var i = 0; i < this.entityIDs.length; i++) {
            Entities.deleteEntity(this.entityIDs[i]);
        }
        this.entityIDs = [];
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

        // Initialize game state
        this.waveNumber = 0;
        this.setScore(0);
        this.setLivesLeft(10);

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
            text: "Wave: " + this.waveNumber
        });

        var numberOfEnemiesLeftToSpawn = this.waveNumber * 2;
        var delayBetweenSpawns = 2000 / Math.max(1, Math.log(this.waveNumber));
        var currentDelay = 2000;

        print("Number of enemies:", numberOfEnemiesLeftToSpawn);
        this.checkEnemyPositionsTimer = Script.setInterval(this.checkForEscapedEnemies.bind(this), 100);

        for (var i = 0; i < numberOfEnemiesLeftToSpawn; ++i) {
            print("Adding enemy");
            var idx = Math.floor(getRandom() * this.spawnPositions.length);
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

            Audio.playSound(WAVE_COMPLETE_SOUND, {
                volume: 1.0,
                position: this.rootPosition
            });
        }
    },
    setLivesLeft: function(lives) {
        lives = Math.max(0, lives);
        this.livesLeft = lives;
        Entities.editEntity(this.livesDisplayID, {
            text: "Lives: " + this.livesLeft
        });
    },
    setScore: function(score) {
        this.score = score;
        Entities.editEntity(this.scoreDisplayID, {
            text: "Score: " + utils.formatNumberWithCommas(this.score)
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
        print("Spawn queue size: ", this.spawnQueue.length, "Elapsed time: ", waveElapsedTime, "Number of enemies:", this.enemyIDs.length);
    },
    checkForEscapedEnemies: function() {
        // Move this somewhere else?
        this.checkSpawnQueue();

        var enemiesEscaped = false;
        for (var i = this.enemyIDs.length - 1; i >= 0; --i) {
            var position = Entities.getEntityProperties(this.enemyIDs[i], 'position').position;
            if (position && position.z < this.gatePosition.z) {
                Entities.deleteEntity(this.enemyIDs[i]);
                this.enemyIDs.splice(i, 1);
                this.setLivesLeft(this.livesLeft - 1);
                this.numberOfEntitiesLeftForWave--;
                enemiesEscaped = true;
            }
        }
        print("LIVES LEFT: ", this.livesLeft, this.numberOfEntitiesLeftForWave);
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

        Audio.playSound(GAME_OVER_SOUND, {
            volume: 1.0,
            position: this.rootPosition
        });

        Entities.editEntity(this.livesDisplayID, { text: "GAME OVER" });

        this.gameState = GAME_STATES.GAME_OVER;
        print("GAME OVER");

        // Cleanup
        Script.clearTimeout(this.nextWaveTimer);
        this.nextWaveTimer = null;
        for (var i = 0; i < this.spawnEnemyTimers.length; ++i) {
            Script.clearTimeout(this.spawnEnemyTimers[i]);
        }
        this.spawnEnemyTimers = [];

        Script.clearInterval(this.checkEnemyPositionsTimer);
        this.checkEnemyPositionsTimer = null;

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
            Audio.playSound(EXPLOSION_SOUND, {
                volume: 1.0,
                position: position
            });

            this.numberOfEntitiesLeftForWave--;
            // Update score
            this.setScore(this.score + 100);
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
    const rootPosition = utils.findSurfaceBelowPosition(MyAvatar.position);

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

    const waveDisplayID = Entities.addEntity({
        type: "Text",
        name: "WG.Wave",
        position: Vec3.sum(rootPosition, {
            x: 0,
            y: 9,
            z: 10
        }),
        "backgroundColor": {
            "blue": 255,
            "green": 255,
            "red": 255
        },
        "lineHeight": 1,
        "rotation": {
            "w": -4.3711388286737929e-08,
            "x": 0,
            "y": -1,
            "z": 0
        },
        "dimensions": {
            "x": 10,
            "y": 1.7218999862670898,
            "z": 0.0099999997764825821
        },
        "text": "Game Over",
        "textColor": {
            "blue": 0,
            "green": 0,
            "red": 0
        },
    });
    entityIDs.push(waveDisplayID);

    const scoreDisplayID = Entities.addEntity({
        type: "Text",
        name: "WG.Score",
        position: Vec3.sum(rootPosition, {
            x: 0,
            y: 7,
            z: 10
        }),
        "backgroundColor": {
            "blue": 255,
            "green": 255,
            "red": 255
        },
        "lineHeight": 1,
        "rotation": {
            "w": -4.3711388286737929e-08,
            "x": 0,
            "y": -1,
            "z": 0
        },
        "dimensions": {
            "x": 10,
            "y": 1.7218999862670898,
            "z": 0.0099999997764825821
        },
        "text": "Game Over",
        "textColor": {
            "blue": 0,
            "green": 0,
            "red": 0
        },
    });
    entityIDs.push(scoreDisplayID);

    const livesDisplayID = Entities.addEntity({
        type: "Text",
        name: "WG.Lives",
        position: Vec3.sum(rootPosition, {
            x: 0,
            y: 5,
            z: 10
        }),
        "backgroundColor": {
            "blue": 255,
            "green": 255,
            "red": 255
        },
        "lineHeight": 1,
        "rotation": {
            "w": -4.3711388286737929e-08,
            "x": 0,
            "y": -1,
            "z": 0
        },
        "dimensions": {
            "x": 10,
            "y": 1.7218999862670898,
            "z": 0.0099999997764825821
        },
        "text": "Game Over",
        "textColor": {
            "blue": 0,
            "green": 0,
            "red": 0
        },
    });
    entityIDs.push(livesDisplayID);


    var goalPositionFront = Vec3.sum(goalPosition, { x: 0, y: 0, z: BASES_SIZE / 2 });
    return new GameManager(rootPosition, goalPositionFront, roofPosition, spawnPositions, buttonID, waveDisplayID, scoreDisplayID, livesDisplayID);
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
        var message = utils.parseJSON(messageJSON);
        if (message === undefined) {
            print("playWaveGame.js | Received non-json message");
            return;
        }
        switch (message.type) {
            case 'start-game':
                gameManager.startGame();
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
