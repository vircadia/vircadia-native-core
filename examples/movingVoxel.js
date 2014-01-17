//
//  This sample script creates a voxel moving back and forth in a line
//

var position  = { x: 0, y: 0, z: 0 };
var oldPosition = { x: 0, y: 0, z:0 };
var size = 0.25;
var direction = 1.0;
var range = 2.0;
var color = { r: 100, g: 50, b: 150 };
var colorEdge = { r:255, g:250, b:175 };
var frame = 0;
var thisColor = color;

function moveVoxel() {
    frame++;
	if (frame % 3 == 0) {
		//  Get a new position
		position.x += direction * size;
		if (position.x < 0) {
			direction *= -1.0;
			position.x = 0;
            thisColor = colorEdge;
		}
		if (position.x > range) { 
			direction *= -1.0;
			position.x = range;
            thisColor = colorEdge;
		}
		//  Create a new voxel
		Voxels.queueDestructiveVoxelAdd(position.x / TREE_SCALE, position.y / TREE_SCALE, position.z / TREE_SCALE, size / TREE_SCALE, thisColor.r, thisColor.g, thisColor.b);
		//  delete old voxel 
		Voxels.queueVoxelDelete(oldPosition.x / TREE_SCALE, oldPosition.y / TREE_SCALE, oldPosition.z / TREE_SCALE, size / TREE_SCALE);
		//  Copy old location to new 
		oldPosition.x = position.x;
        oldPosition.y = position.y;
        oldPosition.z = position.z;
        thisColor = color;
	}
}

Voxels.setPacketsPerSecond(300);
// Connect a call back that happens every frame
Agent.willSendVisualDataCallback.connect(moveVoxel);