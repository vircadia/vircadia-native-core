function randInt(min, max) {
	return Math.floor(Math.random() * (max - min)) + min;
}

var wandModel = "http://hifi-public.s3.amazonaws.com/james/bubblewand/models/wand/wand.fbx?" + randInt(0, 10000);
var scriptURL = "http://hifi-public.s3.amazonaws.com/james/bubblewand/scripts/wand.js" + randInt(1, 10048)
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));
var paintGun = Entities.addEntity({
	type: "Model",
	modelURL: wandModel,
	position: center,
	dimensions: {
		x: 0.1,
		y: 1,
		z: 0.1
	},
	collisionsWillMove: true,
	shapeType: 'box',
	script: scriptURL
});

function cleanup() {
	Entities.deleteEntity(paintGun);
}


Script.scriptEnding.connect(cleanup);