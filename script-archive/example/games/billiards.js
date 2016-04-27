// Billiards.js 
// 
// Created by Philip Rosedale on January 21, 2015 
// Copyright 2014 High Fidelity, Inc.
//
// Creates a pool table in front of you.  Hold and release space ball to shoot a ball.  
// Cue ball will return if falls off table.  Delete and reset to restart. 
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

var tableParts = []; 
var balls = [];
var cueBall;

var LENGTH = 2.84; 
var WIDTH = 1.42; 
var HEIGHT = 0.80;
var SCALE = 2.0; 
var BALL_SIZE = 0.05715;
var BUMPER_WIDTH = 0.15;
var BUMPER_HEIGHT = BALL_SIZE * 2.0;
var HOLE_SIZE = BALL_SIZE;
var DROP_HEIGHT = BALL_SIZE * 3.0;
var GRAVITY = -9.8;
var BALL_GAP = 0.001;
var tableCenter;
var cuePosition; 

var startStroke = 0;

// Sounds to use 
var hitSound = HIFI_PUBLIC_BUCKET + "sounds/Collisions-ballhitsandcatches/billiards/collision1.wav";
SoundCache.getSound(hitSound);

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
var screenSize = Controller.getViewportDimensions();
var reticle = Overlays.addOverlay("image", {
    x: screenSize.x / 2 - 16,
    y: screenSize.y / 2 - 16,
    width: 32,
    height: 32,
    imageURL: HIFI_PUBLIC_BUCKET + "images/billiardsReticle.png",
    color: { red: 255, green: 255, blue: 255},
    alpha: 1
});

function makeTable(pos) {
    // Top 
    tableParts.push(Entities.addEntity(
        { type: "Box",
          position: pos, 
      dimensions: { x: LENGTH * SCALE, y: HEIGHT, z: WIDTH * SCALE }, 
            color: {  red: 0, green: 255, blue: 0 } }));
    // Long Bumpers 
    tableParts.push(Entities.addEntity(
        { type: "Box",
          position: { x: pos.x - LENGTH / 2.0, 
                  y: pos.y + (HEIGHT / 2.0 + BUMPER_HEIGHT / 2.0), 
                  z: pos.z - (WIDTH / 2.0 + BUMPER_WIDTH / 2.0) * SCALE },
      dimensions: { x: (LENGTH - 3.0 * HOLE_SIZE) * SCALE / 2.0, y: BUMPER_HEIGHT, z: BUMPER_WIDTH * SCALE }, 
            color: {  red: 237, green: 201, blue: 175 } }));
    tableParts.push(Entities.addEntity(
        { type: "Box",
          position: { x: pos.x + LENGTH / 2.0, 
                  y: pos.y + (HEIGHT / 2.0 + BUMPER_HEIGHT / 2.0), 
                  z: pos.z - (WIDTH / 2.0 + BUMPER_WIDTH / 2.0) * SCALE },
      dimensions: { x: (LENGTH - 3.0 * HOLE_SIZE) * SCALE / 2.0, y: BUMPER_HEIGHT, z: BUMPER_WIDTH * SCALE }, 
            color: {  red: 237, green: 201, blue: 175 } }));

    tableParts.push(Entities.addEntity(
        { type: "Box",
          position: { x: pos.x - LENGTH / 2.0, 
                  y: pos.y + (HEIGHT / 2.0 + BUMPER_HEIGHT / 2.0), 
                  z: pos.z + (WIDTH / 2.0 + BUMPER_WIDTH / 2.0) * SCALE },
      dimensions: { x: (LENGTH - 3.0 * HOLE_SIZE) * SCALE / 2.0, y: BUMPER_HEIGHT, z: BUMPER_WIDTH * SCALE }, 
            color: {  red: 237, green: 201, blue: 175 } }));
    tableParts.push(Entities.addEntity(
        { type: "Box",
          position: { x: pos.x + LENGTH / 2.0, 
                  y: pos.y + (HEIGHT / 2.0 + BUMPER_HEIGHT / 2.0), 
                  z: pos.z + (WIDTH / 2.0 + BUMPER_WIDTH / 2.0) * SCALE },
      dimensions: { x: (LENGTH - 3.0 * HOLE_SIZE) * SCALE / 2.0, y: BUMPER_HEIGHT, z: BUMPER_WIDTH * SCALE }, 
            color: {  red: 237, green: 201, blue: 175 } }));
    // End bumpers 
    tableParts.push(Entities.addEntity(
        { type: "Box",
          position: { x: pos.x + (LENGTH / 2.0 + BUMPER_WIDTH / 2.0) * SCALE, 
                  y: pos.y + (HEIGHT / 2.0 + BUMPER_HEIGHT / 2.0), 
                  z: pos.z },
      dimensions: { z: (WIDTH - 2.0 * HOLE_SIZE) * SCALE, y: BUMPER_HEIGHT, x: BUMPER_WIDTH * SCALE }, 
            color: {  red: 237, green: 201, blue: 175 } }));

    tableParts.push(Entities.addEntity(
        { type: "Box",
          position: { x: pos.x - (LENGTH / 2.0 + BUMPER_WIDTH / 2.0) * SCALE, 
                  y: pos.y + (HEIGHT / 2.0 + BUMPER_HEIGHT / 2.0), 
                  z: pos.z },
      dimensions: { z: (WIDTH - 2.0 * HOLE_SIZE) * SCALE, y: BUMPER_HEIGHT, x: BUMPER_WIDTH * SCALE }, 
            color: {  red: 237, green: 201, blue: 175 } }));

}

function makeBalls(pos) {
    // Object balls 
    var whichBall = [ 1, 14, 15, 4, 8, 7, 12, 9, 3, 13, 10, 5, 6, 11, 2 ];
    var ballNumber = 0;
    var ballPosition = { x: pos.x + (LENGTH / 4.0) * SCALE, y: pos.y + HEIGHT / 2.0 + DROP_HEIGHT, z: pos.z }; 
    for (var row = 1; row <= 5; row++) {
    ballPosition.z = pos.z - ((row - 1.0) / 2.0 * (BALL_SIZE + BALL_GAP) * SCALE); 
    for (var spot = 0; spot < row; spot++) {
        balls.push(Entities.addEntity(
            { type: "Model",
                  modelURL: "https://s3.amazonaws.com/hifi-public/models/props/Pool/ball_" +
                      whichBall[ballNumber].toString() + ".fbx",
              position: ballPosition,   
          dimensions: { x: BALL_SIZE * SCALE, y: BALL_SIZE * SCALE, z: BALL_SIZE * SCALE }, 
                  rotation: Quat.fromPitchYawRollDegrees((Math.random() - 0.5) * 20,
                                                         (Math.random() - 0.5) * 20,
                                                         (Math.random() - 0.5) * 20),
                color: { red: 255, green: 255, blue: 255 },
                gravity: {  x: 0, y: GRAVITY, z: 0 },
              velocity: {x: 0, y: -0.2, z: 0 },
              ignoreCollisions: false,
              damping: 0.50,
                  shapeType: "sphere",
          collisionSoundURL: hitSound,
              dynamic: true }));
        ballPosition.z += (BALL_SIZE + BALL_GAP) * SCALE;
            ballNumber++;
    }
    ballPosition.x += (BALL_GAP + Math.sqrt(3.0) / 2.0 * BALL_SIZE) * SCALE;
    }

    // Cue Ball 
    cuePosition = { x: pos.x - (LENGTH / 4.0) * SCALE, y: pos.y + HEIGHT / 2.0 + DROP_HEIGHT, z: pos.z }; 
    cueBall = Entities.addEntity(
    { type: "Model",
          modelURL: "https://s3.amazonaws.com/hifi-public/models/props/Pool/cue_ball.fbx",
      position: cuePosition,   
      dimensions: { x: BALL_SIZE * SCALE, y: BALL_SIZE * SCALE, z: BALL_SIZE * SCALE }, 
      color: { red: 255, green: 255, blue: 255 },
      gravity: {  x: 0, y: GRAVITY, z: 0 },
      angularVelocity: { x: 0, y: 0, z: 0 },
      velocity: {x: 0, y: -0.2, z: 0 },
      ignoreCollisions: false,
      damping: 0.50,
          shapeType: "sphere",
      dynamic: true });
    
}

function isObjectBall(id) {
    for (var i; i < balls.length; i++) {
    if (balls[i].id == id) {
        return true;
    }        
    }
    return false; 
}

function shootCue(velocity) {
    var DISTANCE_FROM_CAMERA = BALL_SIZE * 5.0 * SCALE;
    var camera = Camera.getPosition();
    var forwardVector = Quat.getFront(Camera.getOrientation());
    var cuePosition = Vec3.sum(camera, Vec3.multiply(forwardVector, DISTANCE_FROM_CAMERA));
    var velocity = Vec3.multiply(forwardVector, velocity);
    var BULLET_LIFETIME = 3.0;
    var BULLET_GRAVITY = 0.0;
    var SHOOTER_COLOR = { red: 255, green: 0, blue: 0 };
    var SHOOTER_SIZE = BALL_SIZE / 1.5 * SCALE;

    bulletID = Entities.addEntity(
        { type: "Sphere",
          position: cuePosition, 
          dimensions: { x: SHOOTER_SIZE, y: SHOOTER_SIZE, z: SHOOTER_SIZE }, 
          color: SHOOTER_COLOR,      
          velocity: velocity, 
          lifetime: BULLET_LIFETIME,
          gravity: {  x: 0, y: BULLET_GRAVITY, z: 0 },
          damping: 0.10,
          density: 8000,
          ignoreCollisions: false,
          dynamic: true
        });
    print("Shot, velocity = " + velocity);
}

function keyReleaseEvent(event) {
    if ((startStroke > 0) && event.text == "SPACE") {
    var endTime = new Date().getTime();
    var delta = endTime - startStroke;
        shootCue(delta / 100.0);
        startStroke = 0;
    }
}

function keyPressEvent(event) {
    // Fire a cue ball 
    if ((startStroke == 0) && (event.text == "SPACE")) {
        startStroke = new Date().getTime();    
    }
}

function cleanup() {
    for (var i = 0; i < tableParts.length; i++) {
    Entities.deleteEntity(tableParts[i]);
    }
    for (var i = 0; i < balls.length; i++) {
    Entities.deleteEntity(balls[i]);
    }
    Overlays.deleteOverlay(reticle); 
    Entities.deleteEntity(cueBall);
}

function update(deltaTime) {
    //  Check if cue ball has fallen off table, re-drop if so 
    var cueProperties = Entities.getEntityProperties(cueBall);
    if (cueProperties.position.y < tableCenter.y) {
        // Replace the cueball 
        Entities.editEntity(cueBall, { position: cuePosition } );

    }
}

tableCenter = Vec3.sum(MyAvatar.position, Vec3.multiply(4.0, Quat.getFront(Camera.getOrientation())));

makeTable(tableCenter);
makeBalls(tableCenter);

Script.scriptEnding.connect(cleanup);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
Script.update.connect(update);

