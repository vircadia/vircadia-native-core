//
//  particleExplorer.js
//  
//
//  Created by James B. Pollack @imgntnon 9/26/2015
//  Copyright 2014 High Fidelity, Inc.
//
// 	Interface side of the App.
//  Quickly edit the aesthetics of a particle system.  This is an example of a new, easy way to do two way bindings between dynamically created GUI and in-world entities.  
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  todo: folders, color pickers, animation settings, scale gui width with window resizing  
//
var boxPoint,
	spawnPoint,
	animation = {
		fps: 30,
		frameIndex: 0,
		running: true,
		firstFrame: 0,
		lastFrame: 30,
		loop: true
	};


var boxPoint = Vec3.sum(MyAvatar.position, Vec3.multiply(4.0, Quat.getFront(Camera.getOrientation())));
boxPoint = Vec3.sum(boxPoint, {
	x: 0.0,
	y: -0.5,
	z: 0.0
});
spawnPoint = Vec3.sum(boxPoint, {
	x: 0.0,
	y: 1.0,
	z: 0.0
});

var PI = 3.141593,
	DEG_TO_RAD = PI / 180.0;

BlankBox = function() {
	this.animationIsPlaying = true;
	this.accelerationSpread_x_group = 0.1;
	this.accelerationSpread_y_group = 0.1;
	this.accelerationSpread_z_group = 0.1;
	this.alpha = 0.5;
	this.alphaStart = 1.0;
	this.alphaFinish = 0.1;
	this.color_red_group = 0;
	this.color_green_group = 0;
	this.color_blue_group = 0;
	this.colorSpread_red_group = 0;
	this.colorSpread_green_group = 0;
	this.colorSpread_blue_group = 0;
	this.colorStart_red_group = 0;
	this.colorStart_green_group = 0;
	this.colorStart_blue_group = 0;
	this.colorFinish_red_group = 0;
	this.colorFinish_green_group = 0;
	this.colorFinish_blue_group = 0;
	this.azimuthStart = -PI / 2.0;
	this.azimuthFinish = PI / 2.0;
	this.emitAccceleration_x_group = 0.01;
	this.emitAccceleration_y_group = 0.01;
	this.emitAccceleration_z_group = 0.01;
	this.emitDimensions_x_group = 0.01;
	this.emitDimensions_y_group = 0.01;
	this.emitDimensions_z_group = 0.01;
	this.emitOrientation_x_group = 0.01;
	this.emitOrientation_y_group = 0.01;
	this.emitOrientation_z_group = 0.01;
	this.emitOrientation_w_group = 0.01;
	this.emitRate = 0.1;
	this.emitSpeed = 0.1;
	this.polarStart = 0.01;
	this.polarFinish = 2.0 * DEG_TO_RAD;
	this.speedSpread = 0.1;
	this.radiusSpread = 0.035;
	this.radiusStart = 0.0;
	this.radiusFinish = 0.0;
	this.velocitySpread = 0;
	//
	this.type = "ParticleEffect";
	this.name = "ParticlesTest Emitter";
	this.position = spawnPoint;
	this.textures = "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png";
	this.lifespan = 5.0;
	this.visible = false;
	this.locked = false;
	this.animationSettings = animation;
	this.lifetime = 3600 // 1 hour; just in case
}

var blankBox = new BlankBox();

var box = Entities.addEntity({
	type: "Box",
	name: "ParticlesTest Box",
	position: boxPoint,
	rotation: {
		x: -0.7071067690849304,
		y: 0,
		z: 0,
		w: 0.7071067690849304
	},
	dimensions: {
		x: 0.3,
		y: 0.3,
		z: 0.3
	},
	color: {
		red: 128,
		green: 128,
		blue: 128
	},
	lifetime: 3600, // 1 hour; just in case
	visible: true
});

var testParticles = Entities.addEntity(blankBox);

SettingsWindow = function() {
	var _this = this;
	this.webWindow = null;
	this.init = function() {
		_this.webWindow = new WebWindow('ParticleExplorer', Script.resolvePath('index.html'), 400, 600, true);
		_this.webWindow.setVisible(true);
		_this.webWindow.eventBridge.webEventReceived.connect(_this.onWebEventReceived);
		print('INIT testParticles' + testParticles)
	};
	this.onWebEventReceived = function(data) {
		print('DATA ' + data)


		var _data = JSON.parse(data)
		if (_data.type !== 'particleExplorer_update') {
			return;
		}
		if (_data.shouldGroup === true) {
			print('USE GROUP PROPERTIES')
			editEntity(_data.groupProperties)
			return;
		} else {
			print('USE A SINGLE PROPERTY')
			editEntity(_data.singleProperty)

		}


	};

}


function editEntity(properties) {
	// print('TEST PARTICLES??' + testParticles)
	print('PROPS' + JSON.stringify(properties));
	Entities.editEntity(testParticles, properties);
	var currentProperties = Entities.getEntityProperties(testParticles)
	print('CURRENT PROPS', JSON.stringify(currentProperties))
}


var settingsWindow = new SettingsWindow();
settingsWindow.init();

function cleanup() {
	Entities.deleteEntity(testParticles);
	Entities.deleteEntity(box);
}
Script.scriptEnding.connect(cleanup);