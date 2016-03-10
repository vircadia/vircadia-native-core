//
//  createColorBusterWand.js
//
//  Created by James B. Pollack @imgntn on 11/2/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This script creates a wand that can be used to remove color buster blocks.  Touch your wand to someone else's to combine colors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var DELETE_AT_ENDING = false;

var COLOR_WAND_MODEL_URL = 'http://hifi-public.s3.amazonaws.com/models/color_busters/wand.fbx';
var COLOR_WAND_COLLISION_HULL_URL = 'http://hifi-public.s3.amazonaws.com/models/color_busters/wand_collision_hull.obj';
var COLOR_WAND_SCRIPT_URL = Script.resolvePath('colorBusterWand.js');

var COLOR_WAND_DIMENSIONS = {
	x: 0.04,
	y: 0.87,
	z: 0.04
};

var COLOR_WAND_START_POSITION = {
	x: 0,
	y: 0,
	z: 0
};

var STARTING_COLORS = [
	['red', {
		red: 255,
		green: 0,
		blue: 0
	}],
	['yellow', {
		red: 255,
		green: 255,
		blue: 0
	}],
	['blue', {
		red: 0,
		green: 0,
		blue: 255
	}]
];

var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
	x: 0,
	y: 0.5,
	z: 0
}), Vec3.multiply(0.5, Quat.getFront(Camera.getOrientation())));


function chooseStartingColor() {
	var startingColor = STARTING_COLORS[Math.floor(Math.random() * STARTING_COLORS.length)];
	return startingColor
}

var wand;

function createColorBusterWand() {
	var startingColor = chooseStartingColor();
	var colorBusterWandProperties = {
		name: 'Hifi-ColorBusterWand',
		type: 'Model',
		modelURL: COLOR_WAND_MODEL_URL,
		shapeType: 'compound',
		compoundShapeURL: COLOR_WAND_COLLISION_HULL_URL,
		dimensions: COLOR_WAND_DIMENSIONS,
		position: center,
		script: COLOR_WAND_SCRIPT_URL,
		dynamic: true,
		userData: JSON.stringify({
			hifiColorBusterWandKey: {
				owner: MyAvatar.sessionUUID,
				currentColor: startingColor[0],
				originalColorName: startingColor[0],
				colorLocked: false
			},
			grabbableKey: {
				invertSolidWhileHeld: false
			}
		})
	};

	wand = Entities.addEntity(colorBusterWandProperties);
}

function deleteWand() {
	Entities.deleteEntity(wand);
}

if (DELETE_AT_ENDING === true) {
	Script.scriptEnding.connect(deleteWand);
}

createColorBusterWand();
