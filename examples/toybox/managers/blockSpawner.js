HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
var modelUrl = HIFI_PUBLIC_BUCKET + 'marketplace/hificontent/Games/blocks/block.fbx';

var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));
var BASE_DIMENSIONS = Vec3.multiply({x: 0.2, y: 0.1, z: 0.8}, 0.2);
var NUM_BLOCKS = 4;

var blocks = [];
    
spawnBlocks();

// var table = Entities.addEntity({
//     type: "Box",
//     position: Vec3.sum(center, {x: 0, y: -0.2, z: 0}),
//     dimensions: {x: 2, y: .01, z: 2},
//     color: {red: 20, green: 20, blue: 20}
// })
function spawnBlocks() {
	for (var i = 0; i < NUM_BLOCKS; i++) {
		var block = Entities.addEntity({
			type: "Model",
			modelURL: modelUrl,
			position: {x: 548.3, y:495.55, z:504.4},
			shapeType: 'box',
			name: "block",
			dimensions: Vec3.sum(BASE_DIMENSIONS, {x: Math.random()/10, y: Math.random()/10, z:Math.random()/10}),
            collisionsWillMove: true,
            gravity: {x: 0, y: -9.8, z: 0},
            velocity: {x: 0, y: -.01, z: 0}

		});
		blocks.push(block);
	}
}

function cleanup() {
    // Entities.deleteEntity(table);
// 	blocks.forEach(function(block) {
// 	    Entities.deleteEntity(block);
// 	});
}

Script.scriptEnding.connect(cleanup);