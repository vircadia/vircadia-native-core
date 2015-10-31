var COLOR_WAND_MODEL_URL = '';
var COLOR_WAND_DIMENSIONS = {
	x: 0,
	y: 0,
	z: 0
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
]

function chooseStartingColor() {
	var startingColor = STARTING_COLORS[Math.floor(Math.random() * STARTING_COLORS.length)];
	return startingColor
}

function createColorBusterWand() {

	var startingColor = chooseStartingColor();
	var colorBusterWandProperties = {
		name: 'Hifi-ColorBusterWand',
		type: 'Model',
		url: COLOR_WAND_MODEL_URL,
		dimensions: COLOR_WAND_DIMENSIONS,
		position: COLOR_WAND_START_POSITION,
		userData: JSON.stringify({
			hifiColorBusterWandKey: {
				owner: MyAvatar.sessionUUID,
				currentColor: startingColor[1]
				originalColorName: startingColor[0],
				colorLocked: false
			}
		})
	};

	Entities.addEntity(colorBusterWandProperties);
}