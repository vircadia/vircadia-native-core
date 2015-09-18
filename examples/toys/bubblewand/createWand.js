//  createWand.js
//  part of bubblewand
//
//  Created by James B. Pollack @imgntn -- 09/03/2015
//  Copyright 2015 High Fidelity, Inc.
//
// 	Loads a wand model and attaches the bubble wand behavior.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html



Script.include("https://raw.githubusercontent.com/highfidelity/hifi/master/examples/utilities.js");
Script.include("https://raw.githubusercontent.com/highfidelity/hifi/master/examples/libraries/utils.js");

var wandModel = "http://hifi-public.s3.amazonaws.com/james/bubblewand/models/wand/wand.fbx?" + randInt(0, 10000);
var scriptURL = "http://hifi-public.s3.amazonaws.com/james/bubblewand/scripts/wand.js?" + randInt(1, 100500)


//create the wand in front of the avatar
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));
var wand = Entities.addEntity({
	type: "Model",
	modelURL: wandModel,
	position: center,
	dimensions: {
		x: 0.1,
		y: 1,
		z: 0.1
	},
	//must be enabled to be grabbable in the physics engine
	collisionsWillMove: true,
	shapeType: 'box',
	script: scriptURL
});

function cleanup() {
	Entities.deleteEntity(wand);
}


Script.scriptEnding.connect(cleanup);