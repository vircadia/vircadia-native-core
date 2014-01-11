//
//  This sample script makes an occassional clapping sound at the location of your palm
//

//  First, load the clap sound from a URL 
var clap = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/clap1.raw");

function maybePlaySound() {
	if (Math.random() < 0.01) {
		//  Set the location and other info for the sound to play
		var options = new AudioInjectionOptions(); 
		var palmPosition = Controller.getSpatialControlPosition(0);
		options.position = palmPosition;
		options.volume = 0.1;
		Audio.playSound(clap, options);
	}
}

// Connect a call back that happens every frame
Agent.willSendVisualDataCallback.connect(maybePlaySound);