//
//  This sample script watches your hydra hands and makes clapping sound 
//

function length(v) {
    return Math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}


function printVector(v) {
	print(v.x + ", " + v.y + ", " + v.z + "\n");
}

function vMinus(a, b) { 
	var rval = { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z };
	return rval;
}

//  First, load the clap sound from a URL 
var clap = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/clap1.raw");
var clapping = new Array();
clapping[0] = false; 
clapping[1] = false; 

function maybePlaySound() {
	//  Set the location and other info for the sound to play
	var palm1Position = Controller.getSpatialControlPosition(0);
	var palm2Position = Controller.getSpatialControlPosition(2);
	var distanceBetween = length(vMinus(palm1Position, palm2Position));

	for (var palm = 0; palm < 2; palm++) {
		var palmVelocity = Controller.getSpatialControlVelocity(palm * 2 + 1);
		var speed = length(palmVelocity);
		
		const CLAP_SPEED = 0.2;
		const CLAP_DISTANCE = 0.3; 

    	if (!clapping[palm] && (distanceBetween < CLAP_DISTANCE) && (speed > CLAP_SPEED)) {
    		var options = new AudioInjectionOptions(); 
			options.position = palm1Position;
			options.volume = speed / 2.0;
			if (options.volume > 1.0) options.volume = 1.0;
			Audio.playSound(clap, options);
			clapping[palm] = true; 
    	} else if (clapping[palm] && (speed < (CLAP_SPEED / 1.5))) {
    		clapping[palm] = false;
    	}
	}
}

// Connect a call back that happens every frame
Agent.willSendVisualDataCallback.connect(maybePlaySound);