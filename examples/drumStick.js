//
//  This sample script watches your hydra hands and makes clapping sound when they come close together fast 
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

//  First, load two percussion sounds to be used on the sticks

//var drum1 = new Sound("http://public.highfidelity.io/sounds/Colissions-hitsandslaps/Hit1.raw");
var drum1 = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/MusicalInstruments/drums/snare.raw");
var drum2 = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/MusicalInstruments/drums/snare.raw");

//  State = 
//  0 = not triggered 
//  1 = triggered, waiting to stop to play sound
var state = new Array();
state[0] = 0; 
state[1] = 0; 
var strokeSpeed = new Array();
strokeSpeed[0] = 0.0; 
strokeSpeed[1] = 0.0; 

function checkSticks() {
	//  Set the location and other info for the sound to play

	for (var palm = 0; palm < 2; palm++) {
		var palmVelocity = Controller.getSpatialControlVelocity(palm * 2 + 1);
		var speed = length(palmVelocity);
		
		//  lower trigger speed let you 'drum' more slowly. 
		const TRIGGER_SPEED = 0.30;
		const STOP_SPEED = 0.01; 
		const GAIN = 0.5;
		const AVERAGING = 0.2;

		//   Measure trailing average stroke speed to ultimately set volume
		strokeSpeed[palm] = (1.0 - AVERAGING) * strokeSpeed[palm] + AVERAGING * (speed * GAIN);

		if (state[palm] == 0) {
			//  Waiting for downward speed to indicate stroke
			if ((palmVelocity.y < 0.0) && (strokeSpeed[palm] > TRIGGER_SPEED)) {
				state[palm] = 1;
				print("waiting\n");
			}
		} else if (state[palm] == 1) {
			//   Waiting for change in velocity direction or slowing to trigger drum sound
			if ((palmVelocity.y > 0.0) || (speed < STOP_SPEED)) {
				state[palm] = 0;
				print("boom, volume = " + strokeSpeed[palm] + "\n");
				var options = new AudioInjectionOptions(); 
				options.position = Controller.getSpatialControlPosition(palm * 2 + 1);
				if (strokeSpeed[palm] > 1.0) { strokeSpeed[palm] = 1.0; }
				options.volume = strokeSpeed[palm];

				if (palm == 0) {
					Audio.playSound(drum1, options);
				} else {
					Audio.playSound(drum2, options);
				}
			}
		}
	}
}

// Connect a call back that happens every frame
Agent.willSendVisualDataCallback.connect(checkSticks);