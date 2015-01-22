// PaddleBall.js 
// 
// Created by Philip Rosedale on January 21, 2015 
// Copyright 2014 High Fidelity, Inc.
//
// Move your hand with the hydra controller, and hit the ball with the paddle.  
// Click 'X' button to turn off this script. 
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var BALL_SIZE = 0.08;
var PADDLE_SIZE = 0.20;
var PADDLE_THICKNESS = 0.06;
var PADDLE_COLOR = { red: 184, green: 134, blue: 11 };
var BALL_COLOR = { red: 255, green: 0, blue: 0 };
var LINE_COLOR = { red: 255, green: 255, blue: 0 };
var PADDLE_OFFSET = { x: 0.05, y: 0.0, z: 0.0 };
var GRAVITY = 0.0;   
var SPRING_FORCE = 15.0; 
var lastSoundTime = 0; 
var controllerID = 1;
var gameOn = false; 

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
hitSound = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Collisions-ballhitsandcatches/billiards/collision1.wav");

var screenSize = Controller.getViewportDimensions();
var offButton = Overlays.addOverlay("image", {
                    x: screenSize.x - 48,
                    y: 96,
                    width: 32,
                    height: 32,
                    imageURL: HIFI_PUBLIC_BUCKET + "images/close.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });

var ball, paddle, paddleModel, line;

function createEntities() {
	ball = Entities.addEntity(
		    	{ type: "Sphere",
		        position: Controller.getSpatialControlPosition(controllerID),   
				dimensions: { x: BALL_SIZE, y: BALL_SIZE, z: BALL_SIZE }, 
		      	color: BALL_COLOR,
		      	gravity: {  x: 0, y: GRAVITY, z: 0 },
		        ignoreCollisions: false,
		        damping: 0.50,
		        collisionsWillMove: true });

	paddle = Entities.addEntity(
		    	{ type: "Box",
		        position: Controller.getSpatialControlPosition(controllerID),   
				dimensions: { x: PADDLE_SIZE, y: PADDLE_THICKNESS, z: PADDLE_SIZE * 0.80 }, 
		      	color: PADDLE_COLOR,
		      	gravity: {  x: 0, y: 0, z: 0 },
		        ignoreCollisions: false,
		        damping: 0.10,
		        visible: false,
		        rotation: Quat.multiply(MyAvatar.orientation, Controller.getSpatialControlRawRotation(controllerID)),
		        collisionsWillMove: false });

	modelURL = "http://public.highfidelity.io/models/attachments/pong_paddle.fbx";
	paddleModel = Entities.addEntity(
		    	{ type: "Model",
		        position: Vec3.sum(Controller.getSpatialControlPosition(controllerID), PADDLE_OFFSET),   
				dimensions: { x: PADDLE_SIZE * 1.5, y: PADDLE_THICKNESS, z: PADDLE_SIZE * 1.25 }, 
		      	color: PADDLE_COLOR,
		      	gravity: {  x: 0, y: 0, z: 0 },
		        ignoreCollisions: true,
		        modelURL: modelURL,
		        damping: 0.10,
		        rotation: Quat.multiply(MyAvatar.orientation, Controller.getSpatialControlRawRotation(controllerID)),
		        collisionsWillMove: false });

	line = Overlays.addOverlay("line3d", {
    			start: { x: 0, y: 0, z: 0 },
    			end: { x: 0, y: 0, z: 0 },
    			color: LINE_COLOR,
    			alpha: 1,
    			visible: true,
    			lineWidth: 2 });
}

function deleteEntities() {
	Entities.deleteEntity(ball);
    Entities.deleteEntity(paddle);
    Entities.deleteEntity(paddleModel);
    Overlays.deleteOverlay(line); 
}

function update(deltaTime) {
	var palmPosition = Controller.getSpatialControlPosition(controllerID);
	var controllerActive = (Vec3.length(palmPosition) > 0);

	if (!gameOn && controllerActive) {
		createEntities();
		gameOn = true;
	} else if (gameOn && !controllerActive) {
		deleteEntities();
		gameOn = false; 
	} 
	if (!gameOn || !controllerActive) {
		return;
	}

	if (!paddle.isKnownID) {
		paddle = Entities.identifyEntity(paddle);
	}
	if (!ball.isKnownID) {
	   ball = Entities.identifyEntity(ball);
	} else {
		var props = Entities.getEntityProperties(ball);
		var spring = Vec3.subtract(palmPosition, props.position);
		var paddleWorldOrientation = Quat.multiply(MyAvatar.orientation, Controller.getSpatialControlRawRotation(controllerID));
		var springLength = Vec3.length(spring);
		spring = Vec3.normalize(spring);
		var ballVelocity = Vec3.sum(props.velocity, Vec3.multiply(springLength * SPRING_FORCE * deltaTime, spring)); 
    	Entities.editEntity(ball, { velocity: ballVelocity });
    	Overlays.editOverlay(line, { start: props.position, end: palmPosition });
    	Entities.editEntity(paddle, { position: palmPosition, 
    								  velocity: Controller.getSpatialControlVelocity(controllerID),
    								  rotation: paddleWorldOrientation });
    	Entities.editEntity(paddleModel, { position: Vec3.sum(palmPosition, Vec3.multiplyQbyV(paddleWorldOrientation, PADDLE_OFFSET)), 
    								  velocity: Controller.getSpatialControlVelocity(controllerID),
    								  rotation: paddleWorldOrientation });
	}
}

function entityCollisionWithEntity(entity1, entity2, collision) {
	if ((entity1.id == ball.id) || (entity2.id ==ball.id)) {
    	var props1 = Entities.getEntityProperties(entity1); 
    	var props2 = Entities.getEntityProperties(entity2);
    	var dVel = Vec3.length(Vec3.subtract(props1.velocity, props2.velocity));
    	var currentTime = new Date().getTime();
    	var MIN_MSECS_BETWEEN_BOUNCE_SOUNDS = 100;
    	var MIN_VELOCITY_FOR_SOUND_IMPACT = 0.25;
    	if ((dVel > MIN_VELOCITY_FOR_SOUND_IMPACT) && (currentTime - lastSoundTime) > MIN_MSECS_BETWEEN_BOUNCE_SOUNDS) {
    		Audio.playSound(hitSound, { position: props1.position, volume: Math.min(dVel, 1.0) });
    		lastSoundTime = new Date().getTime();
    	} 
    }
}

function mousePressEvent(event) {
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
    if (clickedOverlay == offButton) {
        Script.stop();
    }
}

function scriptEnding() {
	if (gameOn) {
		deleteEntities();
	}
    Overlays.deleteOverlay(offButton);
}

Entities.entityCollisionWithEntity.connect(entityCollisionWithEntity);
Controller.mousePressEvent.connect(mousePressEvent);
Script.scriptEnding.connect(scriptEnding);
Script.update.connect(update);
