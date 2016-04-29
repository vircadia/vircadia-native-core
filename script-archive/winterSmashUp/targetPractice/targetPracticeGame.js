//
//  targetPracticeGame.js
//  examples/winterSmashUp/targetPractice
//
//  Created by Thijs Wenker on 12/21/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  The Winter Smash Up Target Practice Game using a bow.
//  This script runs on the client side (it is loaded through the platform trigger entity)
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

const GAME_CHANNEL = 'winterSmashUpGame';
const SCORE_POST_URL = 'https://script.google.com/macros/s/AKfycbwZAMx6cMBx6-8NGEhR8ELUA-dldtpa_4P55z38Q4vYHW6kneg/exec';
const MODEL_URL = 'http://cdn.highfidelity.com/chris/production/winter/game/';
const MAX_GAME_TIME = 120; //seconds
const TARGET_CLOSE_OFFSET = 0.5;
const MILLISECS_IN_SEC = 1000;
const HIT_SOUND_URL = 'http://hifi-public.s3.amazonaws.com/sounds/Clay_Pigeon_02.L.wav';
const GRAVITY = -9.8;
const MESSAGE_WIDTH = 100;
const MESSAGE_HEIGHT = 50;

const TARGETS = [
    {pitch: -1, yaw: -20, maxDistance: 17},
    {pitch: -1, yaw: -15, maxDistance: 17},
    {pitch: -1, yaw: -10, maxDistance: 17},
    {pitch: -2, yaw: -5, maxDistance: 17},
    {pitch: -1, yaw: 0, maxDistance: 17},
    {pitch: 3, yaw: 10, maxDistance: 17},
    {pitch: -1, yaw: 15, maxDistance: 17},
    {pitch: -1, yaw: 20, maxDistance: 17},
    {pitch: 2, yaw: 25, maxDistance: 17},
    {pitch: 0, yaw: 30, maxDistance: 17}
];

var gameRunning = false;
var gameStartTime;
var gameEntityID;
var gameTimeoutTimer;
var targetEntities = [];
var hitSound = SoundCache.getSound(HIT_SOUND_URL);
var messageOverlay = undefined;
var messageExpire = 0;

var clearMessage = function() {
    if (messageOverlay !== undefined) {
        Overlays.deleteOverlay(messageOverlay);
        messageOverlay = undefined;
    }
};

var displayMessage = function(message, timeout) {
    clearMessage();
    messageExpire = Date.now() + timeout;
        messageOverlay = Overlays.addOverlay('text', {
        text: message,
        x: (Window.innerWidth / 2) - (MESSAGE_WIDTH / 2),
        y: (Window.innerHeight / 2) - (MESSAGE_HEIGHT / 2),
        width: MESSAGE_WIDTH,
        height: MESSAGE_HEIGHT,
        alpha: 1,
        backgroundAlpha: 0.0,
        font: {size: 20}
    });
};

var getStatsText = function() {
    var timeLeft = MAX_GAME_TIME - ((Date.now() - gameStartTime) / MILLISECS_IN_SEC);
    return 'Time remaining: ' + timeLeft.toFixed(1) + 's\nTargets hit: ' + (TARGETS.length - targetEntities.length) + '/' + TARGETS.length;
};

const timerOverlayWidth = 50;
var timerOverlay = Overlays.addOverlay('text', {
    text: '',
    x: Window.innerWidth / 2 - (timerOverlayWidth / 2),
    y: 100,
    width: timerOverlayWidth,
    alpha: 1,
    backgroundAlpha: 0.0,
    visible: false,
    font: {size: 20}
});

var cleanRemainingTargets = function() {
    while (targetEntities.length > 0) {
        Entities.deleteEntity(targetEntities.pop());
    }
};

var createTarget = function(position, rotation, scale) {
    var initialDimensions = {x: 1.8437, y: 0.1614, z: 1.8438};
    var dimensions = Vec3.multiply(initialDimensions, scale);
    return Entities.addEntity({
        type: 'Model',
        rotation: Quat.multiply(rotation, Quat.fromPitchYawRollDegrees(90, 0, 0)),
        lifetime: MAX_GAME_TIME,
        modelURL: MODEL_URL + 'target.fbx',
        shapeType: 'compound',
        compoundShapeURL: MODEL_URL + 'targetCollision.obj',
        dimensions: dimensions,
        position: position
    });
};

var createTargetInDirection = function(startPosition, startRotation, pitch, yaw, maxDistance, scale) {
    var directionQuat = Quat.multiply(startRotation, Quat.fromPitchYawRollDegrees(pitch, yaw, 0.0));
    var directionVec = Vec3.multiplyQbyV(directionQuat, Vec3.FRONT);
    var intersection = Entities.findRayIntersection({direction: directionVec, origin: startPosition}, true);
    var distance = maxDistance;
    if (intersection.intersects && intersection.distance <= maxDistance) {
        distance = intersection.distance - TARGET_CLOSE_OFFSET;
    }
    return createTarget(Vec3.sum(startPosition, Vec3.multiplyQbyV(directionQuat, Vec3.multiply(Vec3.FRONT, distance))), startRotation, scale);    
};

var killTimer = function() {
    if (gameTimeoutTimer !== undefined) {
        Script.clearTimeout(gameTimeoutTimer);
        gameTimeoutTimer = undefined;
    }
};

var submitScore = function() {
    gameRunning = false;
    killTimer();
    Overlays.editOverlay(timerOverlay, {visible: false});
    if (!GlobalServices.username) {
        displayMessage('Could not submit score, you are not logged in.', 5000);
        return;
    }
    var timeItTook = Date.now() - gameStartTime;
    var req = new XMLHttpRequest();
    req.open('POST', SCORE_POST_URL, false);
    req.send(JSON.stringify({
        username: GlobalServices.username,
        time: timeItTook / MILLISECS_IN_SEC
    }));
    displayMessage('Your score has been submitted!', 5000);
};

var onTargetHit = function(targetEntity, projectileEntity, collision) {
    var targetIndex = targetEntities.indexOf(targetEntity);
    if (targetIndex !== -1) {
        try {
            var data = JSON.parse(Entities.getEntityProperties(projectileEntity).userData);
            if (data.creatorSessionUUID === MyAvatar.sessionUUID) {
                this.audioInjector = Audio.playSound(hitSound, {
                    position: collision.contactPoint,
                    volume: 0.5
                });
                // Attach arrow to target for the nice effect
                Entities.editEntity(projectileEntity, {
                    collisionless: true,
                    parentID: targetEntity
                });
                Entities.editEntity(targetEntity, {
                    dynamic: true,
                    gravity: {x: 0, y: GRAVITY, z: 0},
                    velocity: {x: 0, y: -0.01, z: 0}
                });
                targetEntities.splice(targetIndex, 1);
                if (targetEntities.length === 0) {
                    submitScore();
                }
            }
        } catch(e) {
        }
    }
};

var startGame = function(entityID) {
    cleanRemainingTargets();
    killTimer();
    gameEntityID = entityID;
    targetEntities = [];
    var parentEntity = Entities.getEntityProperties(gameEntityID);
    for (var i in TARGETS) {
        var target = TARGETS[i];
        var targetEntity = createTargetInDirection(parentEntity.position, parentEntity.rotation, target.pitch, target.yaw, target.maxDistance, 0.67);
        Script.addEventHandler(targetEntity, 'collisionWithEntity', onTargetHit);
        targetEntities.push(targetEntity);
    }
    gameStartTime = Date.now();
    gameTimeoutTimer = Script.setTimeout(function() {
        cleanRemainingTargets();
        Overlays.editOverlay(timerOverlay, {visible: false});
        gameRunning = false;
        displayMessage('Game timed out.', 5000);
    }, MAX_GAME_TIME * MILLISECS_IN_SEC);
    gameRunning = true;
    Overlays.editOverlay(timerOverlay, {visible: true, text: getStatsText()});
    displayMessage('Game started! GO GO GO!', 3000);
};

Messages.messageReceived.connect(function (channel, message, senderID) {
    if (channel == GAME_CHANNEL) {
        var data = JSON.parse(message);
        switch (data.action) {
            case 'start':
                if (data.playerSessionUUID === MyAvatar.sessionUUID) {
                    startGame(data.gameEntityID);
                }
                break;
        }
    }
});

Messages.subscribe(GAME_CHANNEL);
Messages.sendMessage(GAME_CHANNEL, JSON.stringify({action: 'scriptLoaded'}));

Script.update.connect(function(deltaTime) {
    if (gameRunning) {
        Overlays.editOverlay(timerOverlay, {text: getStatsText()});
    }
    if (messageOverlay !== undefined && Date.now() > messageExpire) {
        clearMessage();
    }
});

Script.scriptEnding.connect(function() {
    Overlays.deleteOverlay(timerOverlay);
    cleanRemainingTargets();
    clearMessage();
});
