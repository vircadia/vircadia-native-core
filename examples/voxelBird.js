//
//  This sample script moves a voxel around like a bird and sometimes makes tweeting noises 
//

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

//  Decide what kind of bird we are 
var tweet;

var which = Math.random();
if (which < 0.2) {
	tweet = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/bushtit_1.raw");
} else if (which < 0.4) {
	tweet = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/rosyfacedlovebird.raw");
} else if (which < 0.6) {
	tweet = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/saysphoebe.raw");
} else  if (which < 0.8) {
	tweet = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/mexicanWhipoorwill.raw");
} else {
	tweet = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/westernscreechowl.raw");
} 

var position  = { x: 0, y: 0, z: 0 };
var lastPosition  = { x: 0, y: 0, z: 0 };
var oldPosition = { x: 0, y: 0, z:0 };
var targetPosition = { x: 0, y: 0, z: 0 };

var size = 0.125;
var range = 50.0;       //   Over what distance in meters do you want your bird to fly around 
var color = { r: 100, g: 50, b: 150 };
var colorEdge = { r:255, g:250, b:175 };
var frame = 0;
var thisColor = color;
var moving = false;
var tweeting = 0;
var moved = true;

var CHANCE_OF_MOVING = 0.05;
var CHANCE_OF_TWEETING = 0.05;

function moveBird(deltaTime) {
    frame++;
	if (frame % 3 == 0) {
		// Tweeting behavior
		if (tweeting == 0) {
			if (Math.random() < CHANCE_OF_TWEETING) {
				//print("tweet!" + "\n");
				var options = new AudioInjectionOptions(); 
				options.position = position;
				options.volume = 0.75;
				Audio.playSound(tweet, options);
				tweeting = 10;
			}
		} else {
			tweeting -= 1;
		}
		// Moving behavior 
		if (moving == false) {
			if (Math.random() < CHANCE_OF_MOVING) {
				targetPosition = randVector(0, range);
				//printVector(position);
				moving = true;
			}
		}
		if (moving) {
			position = vInterpolate(position, targetPosition, 0.5);
			if (vLength(vMinus(position, targetPosition)) < (size / 2.0)) {
				moved = false;
				moving = false;
			} else {
				moved = true;
			}
		}

		if (tweeting > 0) {
			//  Change color of voxel to blinky red a bit while playing the sound
			var blinkColor = { r: Math.random() * 255, g: 0, b: 0 };
			Voxels.setVoxel(position.x, 
                            position.y, 
                            position.z, 
                            size, 
                            blinkColor.r, blinkColor.g, blinkColor.b);
		}
		if (moved) {
			Voxels.setVoxel(position.x, position.y, position.z, size, thisColor.r, thisColor.g, thisColor.b);
			//  delete old voxel 
			
			Voxels.eraseVoxel(oldPosition.x, oldPosition.y, oldPosition.z, size);
			//  Copy old location to new 
			vCopy(oldPosition, position);
			moved = false; 
		}
	}
}

Voxels.setPacketsPerSecond(10000);
// Connect a call back that happens every frame
Script.update.connect(moveBird);