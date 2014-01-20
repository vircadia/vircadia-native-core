

function vLength(v) {
    return Math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

function printVector(v) {
	print(v.x + ", " + v.y + ", " + v.z + "\n");
}

//  Create a random vector with individual lengths between a,b
function randVector(a, b) {
	var rval = { x: a + Math.random() * (b - a), y: a + Math.random() * (b - a), z: a + Math.random() * (b - a) };
	return rval;
}

function vMinus(a, b) { 
	var rval = { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z };
	return rval;
}

function vPlus(a, b) { 
	var rval = { x: a.x + b.x, y: a.y + b.y, z: a.z + b.z };
	return rval;
}

function vCopy(a, b) {
	a.x = b.x;
	a.y = b.y;
	a.z = b.z;
	return;
}

//  Returns a vector which is fraction of the way between a and b
function vInterpolate(a, b, fraction) { 
	var rval = { x: a.x + (b.x - a.x) * fraction, y: a.y + (b.y - a.y) * fraction, z: a.z + (b.z - a.z) * fraction };
	return rval;
}
 
var position = { x: 5.0 / TREE_SCALE, y: 5.0 / TREE_SCALE, z: 5.0 / TREE_SCALE }; 
Voxels.queueDestructiveVoxelAdd(position.x, position.y - (1.0 / TREE_SCALE), position.z, 0.5 / TREE_SCALE, 255, 255, 1);
	
function makeFountain() {
	if (Math.random() < 0.06) {
	    //print("Made particle!\n");

        var size = (0.02 + (Math.random() * 0.05)) / TREE_SCALE;
        var velocity = { x: (Math.random() * 1.0 - 0.5) / TREE_SCALE,
                         y: (1.0 + (Math.random() * 2.0)) / TREE_SCALE,
                         z: (Math.random() * 1.0 - 0.5) / TREE_SCALE };
		
        var gravity = {  x: 0, y: -0.5 / TREE_SCALE, z: 0 }; // gravity has no effect on these bullets
        var color = {  red: 0, green: 0, blue: 128 };
        var damping = 0.25; // no damping
        var inHand = false;
        var script = "";

        Particles.queueParticleAdd(position, size, color, velocity, gravity, damping, inHand, script);
    }
}
// register the call back so it fires before each data send
Agent.willSendVisualDataCallback.connect(makeFountain);