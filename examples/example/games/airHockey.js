// 
//  AirHockey.js 
//  
//  Created by Philip Rosedale on January 26, 2015
//  Copyright 2015 High Fidelity, Inc.
//  
//  AirHockey table and pucks
// 
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var debugVisible = false; 

var FIELD_WIDTH = 1.21;
var FIELD_LENGTH = 1.92;
var FLOOR_THICKNESS = 0.20;
var EDGE_THICKESS = 0.10;
var EDGE_HEIGHT = 0.10;
var DROP_HEIGHT = 0.3; 
var PUCK_SIZE = 0.15;
var PUCK_THICKNESS = 0.03;
var PADDLE_SIZE = 0.12;
var PADDLE_THICKNESS = 0.03;

var GOAL_WIDTH = 0.35;

var GRAVITY = -9.8;
var LIFETIME = 6000;    
var PUCK_DAMPING = 0.03;
var PADDLE_DAMPING = 0.35;
var ANGULAR_DAMPING = 0.10;
var PADDLE_ANGULAR_DAMPING = 0.35;
var MODEL_SCALE = 1.52;
var MODEL_OFFSET = { x: 0, y: -0.18, z: 0 };

var scoreSound = SoundCache.getSound("https://s3.amazonaws.com/hifi-public/sounds/Collisions-hitsandslaps/airhockey_score.wav");

var polyTable = "https://hifi-public.s3.amazonaws.com/ozan/props/airHockeyTable/airHockeyTableForPolyworld.fbx"
var normalTable = "https://hifi-public.s3.amazonaws.com/ozan/props/airHockeyTable/airHockeyTable.fbx"
var hitSound1 = "https://s3.amazonaws.com/hifi-public/sounds/Collisions-hitsandslaps/airhockey_hit1.wav"
var hitSound2 = "https://s3.amazonaws.com/hifi-public/sounds/Collisions-hitsandslaps/airhockey_hit2.wav"
var hitSideSound = "https://s3.amazonaws.com/hifi-public/sounds/Collisions-hitsandslaps/airhockey_hit3.wav"

var center = Vec3.sum(MyAvatar.position, Vec3.multiply((FIELD_WIDTH + FIELD_LENGTH) * 0.60, Quat.getFront(Camera.getOrientation())));

var floor = Entities.addEntity(
	    	{ type: "Box",
	        position: Vec3.subtract(center, { x: 0, y: 0, z: 0 }), 
			dimensions: { x: FIELD_WIDTH, y: FLOOR_THICKNESS, z: FIELD_LENGTH }, 
	      	color: { red: 128, green: 128, blue: 128 },
	      	gravity: {  x: 0, y: 0, z: 0 },
	        ignoreCollisions: false,
	        locked: true,
	        visible: debugVisible,
	        lifetime: LIFETIME });

var edge1 = Entities.addEntity(
	    	{ type: "Box",
	    	collisionSoundURL: hitSideSound,
	        position: Vec3.sum(center, { x: FIELD_WIDTH / 2.0, y: FLOOR_THICKNESS / 2.0, z: 0 }), 
			dimensions: { x: EDGE_THICKESS, y: EDGE_HEIGHT, z: FIELD_LENGTH + EDGE_THICKESS }, 
	      	color: { red: 100, green: 100, blue: 100 },
	      	gravity: {  x: 0, y: 0, z: 0 },
	        ignoreCollisions: false,
	        visible: debugVisible,
	        locked: true,
	        lifetime: LIFETIME });

var edge2 = Entities.addEntity(
	    	{ type: "Box",
	    	collisionSoundURL: hitSideSound,
	        position: Vec3.sum(center, { x: -FIELD_WIDTH / 2.0, y: FLOOR_THICKNESS / 2.0, z: 0 }), 
			dimensions: { x: EDGE_THICKESS, y: EDGE_HEIGHT, z: FIELD_LENGTH + EDGE_THICKESS },  
	      	color: { red: 100, green: 100, blue: 100 },
	      	gravity: {  x: 0, y: 0, z: 0 },
	        ignoreCollisions: false,
	        visible: debugVisible,
	        locked: true,
	        lifetime: LIFETIME });

var edge3a = Entities.addEntity(
	    	{ type: "Box",
	    	collisionSoundURL: hitSideSound,
	        position: Vec3.sum(center, { x: FIELD_WIDTH / 4.0 + (GOAL_WIDTH / 4.0), y: FLOOR_THICKNESS / 2.0, z: -FIELD_LENGTH / 2.0 }), 
			dimensions: { x: FIELD_WIDTH / 2.0 - GOAL_WIDTH / 2.0, y: EDGE_HEIGHT, z: EDGE_THICKESS }, 
	      	color: { red: 100, green: 100, blue: 100 },
	      	gravity: {  x: 0, y: 0, z: 0 },
	        ignoreCollisions: false,
	        visible: debugVisible,
	        locked: true,
	        lifetime: LIFETIME });

var edge3b = Entities.addEntity(
	    	{ type: "Box",
	    	collisionSoundURL: hitSideSound,
	        position: Vec3.sum(center, { x: -FIELD_WIDTH / 4.0 - (GOAL_WIDTH / 4.0), y: FLOOR_THICKNESS / 2.0, z: -FIELD_LENGTH / 2.0 }), 
			dimensions: { x: FIELD_WIDTH / 2.0 - GOAL_WIDTH / 2.0, y: EDGE_HEIGHT, z: EDGE_THICKESS }, 
	      	color: { red: 100, green: 100, blue: 100 },
	      	gravity: {  x: 0, y: 0, z: 0 },
	        ignoreCollisions: false,
	        visible: debugVisible,
	        locked: true,
	        lifetime: LIFETIME });

var edge4a = Entities.addEntity(
	    	{ type: "Box",
	    	collisionSoundURL: hitSideSound,
	        position: Vec3.sum(center, { x: FIELD_WIDTH / 4.0 + (GOAL_WIDTH / 4.0), y: FLOOR_THICKNESS / 2.0, z: FIELD_LENGTH / 2.0 }), 
			dimensions: { x: FIELD_WIDTH / 2.0 - GOAL_WIDTH / 2.0, y: EDGE_HEIGHT, z: EDGE_THICKESS }, 
	      	color: { red: 100, green: 100, blue: 100 },
	      	gravity: {  x: 0, y: 0, z: 0 },
	        ignoreCollisions: false,
	        visible: debugVisible,
	        locked: true,
	        lifetime: LIFETIME });

var edge4b = Entities.addEntity(
	    	{ type: "Box",
	    	collisionSoundURL: hitSideSound,
	        position: Vec3.sum(center, { x: -FIELD_WIDTH / 4.0 - (GOAL_WIDTH / 4.0), y: FLOOR_THICKNESS / 2.0, z: FIELD_LENGTH / 2.0 }), 
			dimensions: { x: FIELD_WIDTH / 2.0 - GOAL_WIDTH / 2.0, y: EDGE_HEIGHT, z: EDGE_THICKESS }, 
	      	color: { red: 100, green: 100, blue: 100 },
	      	gravity: {  x: 0, y: 0, z: 0 },
	        ignoreCollisions: false,
	        visible: debugVisible,
	        locked: true,
	        lifetime: LIFETIME });

var table = Entities.addEntity(
	    	{ type: "Model",
	    	modelURL: polyTable,
	    	dimensions: Vec3.multiply({ x: 0.8, y: 0.45,  z: 1.31 }, MODEL_SCALE),
	        position: Vec3.sum(center, MODEL_OFFSET),
	        ignoreCollisions: false,
	        visible: true,
	        locked: true,
	        lifetime: LIFETIME });

var puck;
var paddle1, paddle2;

//  Create pucks 

function makeNewProp(which) {
	if (which == "puck") {
		return Entities.addEntity(
	    	{ type: "Model",
	    	modelURL: "http://headache.hungry.com/~seth/hifi/puck.obj",
	    	compoundShapeURL: "http://headache.hungry.com/~seth/hifi/puck.obj",
	    	collisionSoundURL: hitSound1,
	        position: Vec3.sum(center, { x: 0, y: DROP_HEIGHT, z: 0 }),  
			dimensions: { x: PUCK_SIZE, y: PUCK_THICKNESS, z: PUCK_SIZE }, 
	      	gravity: {  x: 0, y: GRAVITY, z: 0 },
	      	velocity: { x: 0, y: 0.05, z: 0 },
	        ignoreCollisions: false,
	        damping: PUCK_DAMPING,
	        angularDamping: ANGULAR_DAMPING,
	        lifetime: LIFETIME,
	        collisionsWillMove: true }); 
	}
	else if (which == "paddle1") {
		return Entities.addEntity(
	    	{ type: "Model",
	    	modelURL: "http://headache.hungry.com/~seth/hifi/puck.obj",
	    	compoundShapeURL: "http://headache.hungry.com/~seth/hifi/puck.obj",
	    	collisionSoundURL: hitSound2,
	        position: Vec3.sum(center, { x: 0, y: DROP_HEIGHT, z: FIELD_LENGTH * 0.35 }),  
			dimensions: { x: PADDLE_SIZE, y: PADDLE_THICKNESS, z: PADDLE_SIZE }, 
	      	gravity: {  x: 0, y: GRAVITY, z: 0 },
	      	velocity: { x: 0, y: 0.05, z: 0 },
	        ignoreCollisions: false,
	        damping: PADDLE_DAMPING,
	        angularDamping: PADDLE_ANGULAR_DAMPING,
	        lifetime: LIFETIME,
	        collisionsWillMove: true }); 
	}
	else if (which == "paddle2") {
		return Entities.addEntity(
	    	{ type: "Model",
	    	modelURL: "http://headache.hungry.com/~seth/hifi/puck.obj",
	    	compoundShapeURL: "http://headache.hungry.com/~seth/hifi/puck.obj",
	    	collisionSoundURL: hitSound2,
	        position: Vec3.sum(center, { x: 0, y: DROP_HEIGHT, z: -FIELD_LENGTH * 0.35 }),  
			dimensions: { x: PADDLE_SIZE, y: PADDLE_THICKNESS, z: PADDLE_SIZE },
	      	gravity: {  x: 0, y: GRAVITY, z: 0 },
	      	velocity: { x: 0, y: 0.05, z: 0 },
	        ignoreCollisions: false,
	        damping: PADDLE_DAMPING,
	        angularDamping: PADDLE_ANGULAR_DAMPING,
	        lifetime: LIFETIME,
	        collisionsWillMove: true }); 
	}
}


puck = makeNewProp("puck");
paddle1 = makeNewProp("paddle1");
paddle2 = makeNewProp("paddle2");

function update(deltaTime) {
	if (Math.random() < 0.1) {
		puckProps = Entities.getEntityProperties(puck);
		paddle1Props = Entities.getEntityProperties(paddle1);
		paddle2Props = Entities.getEntityProperties(paddle2);
		if (puckProps.position.y < (center.y - DROP_HEIGHT)) {
			Audio.playSound(scoreSound, {
	      	position: center,
	      	volume: 1.0
	    	});
	    	Entities.deleteEntity(puck);
	    	puck = makeNewProp("puck");
		}
		
		if (paddle1Props.position.y < (center.y - DROP_HEIGHT)) {
			Entities.deleteEntity(paddle1);
	    	paddle1 = makeNewProp("paddle1");
		}
		if (paddle2Props.position.y < (center.y - DROP_HEIGHT)) {
			Entities.deleteEntity(paddle2);
	    	paddle2 = makeNewProp("paddle2");
		}
	}
}

function scriptEnding() {

	Entities.editEntity(edge1, { locked: false });
	Entities.editEntity(edge2, { locked: false });
	Entities.editEntity(edge3a, { locked: false });
	Entities.editEntity(edge3b, { locked: false });
	Entities.editEntity(edge4a, { locked: false });
	Entities.editEntity(edge4b, { locked: false });
	Entities.editEntity(floor, { locked: false });
	Entities.editEntity(table, { locked: false });


	Entities.deleteEntity(edge1);
	Entities.deleteEntity(edge2);
	Entities.deleteEntity(edge3a);
	Entities.deleteEntity(edge3b);
	Entities.deleteEntity(edge4a);
	Entities.deleteEntity(edge4b);
	Entities.deleteEntity(floor);
	Entities.deleteEntity(puck);
	Entities.deleteEntity(paddle1);
	Entities.deleteEntity(paddle2);
	Entities.deleteEntity(table);
}

Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);