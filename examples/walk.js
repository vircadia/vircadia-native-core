//
//  walk.js
//
//  version 1.007b
//
//  Created by Davedub, August / September 2014
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// path to the animation files
var pathToAnimFiles = 'http://s3-us-west-1.amazonaws.com/highfidelity-public/procedural-animator/animation-files/';	// working (but only without https)

// path to the images used for the overlays
var pathToOverlays = 'http://s3-us-west-1.amazonaws.com/highfidelity-public/procedural-animator/overlays/';			// working (but only without https)

// path to the sounds used for the footsteps
var pathToSounds = 'http://highfidelity-public.s3-us-west-1.amazonaws.com/sounds/Footsteps/';

// load the animation datafiles
Script.include(pathToAnimFiles+"dd-female-strut-walk-animation.js");
Script.include(pathToAnimFiles+"dd-female-flying-up-animation.js");
Script.include(pathToAnimFiles+"dd-female-flying-animation.js");
Script.include(pathToAnimFiles+"dd-female-flying-down-animation.js");
Script.include(pathToAnimFiles+"dd-female-standing-one-animation.js");
Script.include(pathToAnimFiles+"dd-female-sidestep-left-animation.js");
Script.include(pathToAnimFiles+"dd-female-sidestep-right-animation.js");
Script.include(pathToAnimFiles+"dd-male-strut-walk-animation.js");
Script.include(pathToAnimFiles+"dd-male-flying-up-animation.js");
Script.include(pathToAnimFiles+"dd-male-flying-animation.js");
Script.include(pathToAnimFiles+"dd-male-flying-down-animation.js");
Script.include(pathToAnimFiles+"dd-male-standing-one-animation.js");
Script.include(pathToAnimFiles+"dd-male-sidestep-left-animation.js");
Script.include(pathToAnimFiles+"dd-male-sidestep-right-animation.js");

// read in the data from the animation files
var FemaleStrutWalkFile = new FemaleStrutWalk();
var femaleStrutWalk = FemaleStrutWalkFile.loadAnimation();
var FemaleFlyingUpFile = new FemaleFlyingUp();
var femaleFlyingUp = FemaleFlyingUpFile.loadAnimation();
var FemaleFlyingFile = new FemaleFlying();
var femaleFlying = FemaleFlyingFile.loadAnimation();
var FemaleFlyingDownFile = new FemaleFlyingDown();
var femaleFlyingDown = FemaleFlyingDownFile.loadAnimation();
var FemaleStandOneFile = new FemaleStandingOne();
var femaleStandOne = FemaleStandOneFile.loadAnimation();
var FemaleSideStepLeftFile = new FemaleSideStepLeft();
var femaleSideStepLeft = FemaleSideStepLeftFile.loadAnimation();
var FemaleSideStepRightFile = new FemaleSideStepRight();
var femaleSideStepRight = FemaleSideStepRightFile.loadAnimation();
var MaleStrutWalkFile = new MaleStrutWalk();
var maleStrutWalk = MaleStrutWalkFile.loadAnimation();
var MaleFlyingUpFile = new MaleFlyingUp();
var maleFlyingUp = MaleFlyingUpFile.loadAnimation();
var MaleFlyingFile = new MaleFlying();
var maleFlying = MaleFlyingFile.loadAnimation();
var MaleFlyingDownFile = new MaleFlyingDown();
var maleFlyingDown = MaleFlyingDownFile.loadAnimation();
var MaleStandOneFile = new MaleStandingOne();
var maleStandOne = MaleStandOneFile.loadAnimation();
var MaleSideStepLeftFile = new MaleSideStepLeft();
var maleSideStepLeft = MaleSideStepLeftFile.loadAnimation();
var MaleSideStepRightFile = new MaleSideStepRight();
var maleSideStepRight = MaleSideStepRightFile.loadAnimation();

// read in the sounds
var footsteps = [];
footsteps.push(new Sound(pathToSounds+"FootstepW2Left-12db.wav"));
footsteps.push(new Sound(pathToSounds+"FootstepW2Right-12db.wav"));
footsteps.push(new Sound(pathToSounds+"FootstepW3Left-12db.wav"));
footsteps.push(new Sound(pathToSounds+"FootstepW3Right-12db.wav"));
footsteps.push(new Sound(pathToSounds+"FootstepW5Left-12db.wav"));
footsteps.push(new Sound(pathToSounds+"FootstepW5Right-12db.wav"));

// all slider controls have a range (with the exception of phase controls that are always +-180) so we store them all here
var sliderRanges = {"joints":[{"name":"hips","pitchRange":25,"yawRange":25,"rollRange":25,"pitchOffsetRange":25,"yawOffsetRange":25,"rollOffsetRange":25,"thrustRange":0.01,"bobRange":0.02,"swayRange":0.01},{"name":"upperLegs","pitchRange":90,"yawRange":35,"rollRange":35,"pitchOffsetRange":60,"yawOffsetRange":20,"rollOffsetRange":20},{"name":"lowerLegs","pitchRange":90,"yawRange":20,"rollRange":20,"pitchOffsetRange":90,"yawOffsetRange":20,"rollOffsetRange":20},{"name":"feet","pitchRange":60,"yawRange":20,"rollRange":20,"pitchOffsetRange":60,"yawOffsetRange":50,"rollOffsetRange":50},{"name":"toes","pitchRange":90,"yawRange":20,"rollRange":20,"pitchOffsetRange":90,"yawOffsetRange":20,"rollOffsetRange":20},{"name":"spine","pitchRange":40,"yawRange":40,"rollRange":40,"pitchOffsetRange":90,"yawOffsetRange":50,"rollOffsetRange":50},{"name":"spine1","pitchRange":20,"yawRange":40,"rollRange":20,"pitchOffsetRange":90,"yawOffsetRange":50,"rollOffsetRange":50},{"name":"spine2","pitchRange":20,"yawRange":40,"rollRange":20,"pitchOffsetRange":90,"yawOffsetRange":50,"rollOffsetRange":50},{"name":"shoulders","pitchRange":35,"yawRange":40,"rollRange":20,"pitchOffsetRange":180,"yawOffsetRange":180,"rollOffsetRange":180},{"name":"upperArms","pitchRange":90,"yawRange":90,"rollRange":90,"pitchOffsetRange":180,"yawOffsetRange":180,"rollOffsetRange":180},{"name":"lowerArms","pitchRange":90,"yawRange":90,"rollRange":120,"pitchOffsetRange":180,"yawOffsetRange":180,"rollOffsetRange":180},{"name":"hands","pitchRange":90,"yawRange":180,"rollRange":90,"pitchOffsetRange":180,"yawOffsetRange":180,"rollOffsetRange":180},{"name":"head","pitchRange":20,"yawRange":20,"rollRange":20,"pitchOffsetRange":90,"yawOffsetRange":90,"rollOffsetRange":90}]};

//  internal state (FSM based) constants
var STANDING = 1;
var WALKING = 2;
var SIDE_STEPPING = 3;
var FLYING = 4;
var CONFIG_WALK_STYLES = 5;
var CONFIG_WALK_TWEAKS = 6;
var CONFIG_WALK_JOINTS = 7;
var CONFIG_STANDING = 8;
var CONFIG_FLYING = 9;
var CONFIG_FLYING_UP = 10;
var CONFIG_FLYING_DOWN = 11;
var CONFIG_SIDESTEP_LEFT = 12;
var CONFIG_SIDESTEP_RIGHT = 14;
var INTERNAL_STATE = STANDING;

// status
var powerOn = true;
var paused = false; // pause animation playback whilst adjusting certain parameters
var minimised = true;
var armsFree = true; // set true for hydra support - temporary fix for Hydras
var statsOn = false;
var playFootStepSounds = true;

// constants
var MAX_WALK_SPEED = 1257; // max oscillation speed
var FLYING_SPEED = 6.4;// 12.4;  // m/s - real humans can't run any faster than 12.4 m/s
var TERMINAL_VELOCITY = 300;  // max speed imposed by Interface
var DIRECTION_UP = 1;
var DIRECTION_DOWN = 2;
var DIRECTION_LEFT = 4;
var DIRECTION_RIGHT = 8;
var DIRECTION_FORWARDS = 16;
var DIRECTION_BACKWARDS = 32;
var DIRECTION_NONE = 64;
var MALE = 1;
var FEMALE = 2;

// start of animation control section
var cumulativeTime = 0.0;
var lastOrientation;

// avi gender and default animations
var avatarGender = MALE;
var selectedWalk = maleStrutWalk; // the currently selected animation walk file (to edit any animation, paste it's name here and select walk editing mode)
var selectedStand = maleStandOne;
var selectedFlyUp = maleFlyingUp;
var selectedFly = maleFlying;
var selectedFlyDown = maleFlyingDown;
var selectedSideStepLeft = maleSideStepLeft;
var selectedSideStepRight = maleSideStepRight;
if(avatarGender===FEMALE) {

	// to make toggling the default quick
	selectedWalk = femaleStrutWalk;
	selectedStand = femaleStandOne;
	selectedFlyUp = femaleFlyingUp;
	selectedFly = femaleFlying;
	selectedFlyDown = femaleFlyingDown;
	selectedSideStepLeft = femaleSideStepLeft;
	selectedSideStepRight = femaleSideStepRight;
}
var currentAnimation = selectedStand; // the current animation
var selectedJointIndex = 0; // the index of the joint currently selected for editing
var currentTransition = null; // used as a pointer to a Transition

// walkwheel (foot / ground speed matching)
var sideStepCycleStartLeft = 270;
var sideStepCycleStartRight = 90;
var walkWheelPosition = 0;
var nextStep = DIRECTION_RIGHT; // first step is always right, because the sine waves say so. Unless you're mirrored.
var nFrames = 0; // counts number of frames
var strideLength = 0; // stride calibration
var aviFootSize = {x:0.1, y:0.1, z:0.25}; // experimental values for addition to stride length - TODO: analyse and confirm is increasing smaller stride lengths accuracy once we have better ground detection

// stats
var frameStartTime = 0;  // when the frame first starts we take a note of the time
var frameExecutionTimeMax = 0; // keep track of the longest frame execution time

// constructor for recent RecentMotion (i.e. frame data) class
function RecentMotion(velocity, acceleration, principleDirection, state) {
	this.velocity = velocity;
	this.acceleration = acceleration;
	this.principleDirection = principleDirection;
	this.state = state;
}

// constructor for the FramesHistory object
function FramesHistory() {
	this.recentMotions = [];
	for(var i = 0 ; i < 10 ; i++) {
		var blank = new RecentMotion({ x:0, y:0, z:0 }, { x:0, y:0, z:0 }, DIRECTION_FORWARDS, STANDING );
		this.recentMotions.push(blank);
	}

	// recentDirection 'method' args: (direction enum, number of steps back to compare)
	this.recentDirection = function () {

		if( arguments[0] && arguments[1] ) {

			var directionCount = 0;
			if( arguments[1] > this.recentMotions.length )
				arguments[1] = this.recentMotions.length;

			for(var i = 0 ; i < arguments[1] ; i++ ) {
				if( this.recentMotions[i].principleDirection === arguments[0] )
					directionCount++;
			}
			return directionCount / arguments[1] === 1 ? true : false;
		}
		return false;
	}

	// recentState 'method' args: (state enum, number of steps back to compare)
	this.recentState = function () {

		if( arguments[0] && arguments[1] ) {

			var stateCount = 0;
			if( arguments[1] > this.recentMotions.length )
				arguments[1] = this.recentMotions.length;

			for(var i = 0 ; i < arguments[1] ; i++ ) {
				if( this.recentMotions[i].state === arguments[0] )
					stateCount++;
			}
			return stateCount / arguments[1] === 1 ? true : false;
		}
		return false;
	}
	this.lastWalkStartTime = 0; 	// short walks and long walks need different handling
}
var framesHistory = new FramesHistory();

// constructor for animation Transition class
function Transition(lastAnimation, nextAnimation, reachPoses, transitionDuration, easingLower, easingUpper) {
	this.lastAnimation = lastAnimation; 			// name of last animation
	if(lastAnimation === selectedWalk ||
	   nextAnimation === selectedSideStepLeft ||
	   nextAnimation === selectedSideStepRight)
		this.walkingAtStart = true;					// boolean - is the last animation a walking animation?
	else
		this.walkingAtStart = false;					// boolean - is the last animation a walking animation?
	this.nextAnimation = nextAnimation;				// name of next animation
	if(nextAnimation === selectedWalk ||
	   nextAnimation === selectedSideStepLeft ||
	   nextAnimation === selectedSideStepRight)
		this.walkingAtEnd = true;						// boolean - is the next animation a walking animation?
	else
		this.walkingAtEnd = false;					// boolean - is the next animation a walking animation?
	this.reachPoses = reachPoses;					// array of reach poses - am very much looking forward to putting these in!
	this.transitionDuration = transitionDuration;	// length of transition (seconds)
	this.easingLower = easingLower;					// Bezier curve handle (normalised)
	this.easingUpper = easingUpper;					// Bezier curve handle (normalised)
	this.startTime = new Date().getTime();			// Starting timestamp (seconds)
	this.progress = 0;								// how far are we through the transition?
	this.walkWheelIncrement = 3;					// how much to turn the walkwheel each frame when coming to a halt. Get's set to 0 once the feet are under the avi
	this.walkWheelAdvance = 0;						// how many degrees the walk wheel has been advanced during the transition
	this.walkStopAngle = 0;							// what angle should we stop the walk cycle? (calculated on the fly)
}

// convert a local (to the avi) translation to a global one
function localToGlobal(localTranslation) {

	var aviOrientation = MyAvatar.orientation;
	var front = Quat.getFront(aviOrientation);
	var right = Quat.getRight(aviOrientation);
	var up    = Quat.getUp   (aviOrientation);
	var aviFront = Vec3.multiply(front,localTranslation.z);
	var aviRight = Vec3.multiply(right,localTranslation.x);
	var aviUp    = Vec3.multiply(up   ,localTranslation.y);
	var globalTranslation = {x:0,y:0,z:0}; // final value
	globalTranslation = Vec3.sum(globalTranslation, aviFront);
	globalTranslation = Vec3.sum(globalTranslation, aviRight);
	globalTranslation = Vec3.sum(globalTranslation, aviUp);
	return globalTranslation;
}

// similar ot above - convert hips translations to global and apply
function translateHips(localHipsTranslation) {

	var aviOrientation = MyAvatar.orientation;
	var front = Quat.getFront(aviOrientation);
	var right = Quat.getRight(aviOrientation);
	var up    = Quat.getUp   (aviOrientation);
	var aviFront = Vec3.multiply(front,localHipsTranslation.y);
	var aviRight = Vec3.multiply(right,localHipsTranslation.x);
	var aviUp    = Vec3.multiply(up   ,localHipsTranslation.z);
	var AviTranslationOffset = {x:0,y:0,z:0}; // final value
	AviTranslationOffset = Vec3.sum(AviTranslationOffset, aviFront);
	AviTranslationOffset = Vec3.sum(AviTranslationOffset, aviRight);
	AviTranslationOffset = Vec3.sum(AviTranslationOffset, aviUp);
	MyAvatar.position = {x: MyAvatar.position.x + AviTranslationOffset.x,
						 y: MyAvatar.position.y + AviTranslationOffset.y,
						 z: MyAvatar.position.z + AviTranslationOffset.z };
}

// clear all joint data
function resetJoints() {

    var avatarJointNames = MyAvatar.getJointNames();
    for (var i = 0; i < avatarJointNames.length; i++) {
        MyAvatar.clearJointData(avatarJointNames[i]);
    }
}

// play footstep sound
function playFootstep(side) {

    var options = new AudioInjectionOptions();
    options.position = Camera.getPosition();
    options.volume = 0.5;
    var walkNumber = 2; // 0 to 2
    if(side===DIRECTION_RIGHT && playFootStepSounds) {
		Audio.playSound(footsteps[walkNumber+1], options);
	}
    else if(side===DIRECTION_LEFT && playFootStepSounds) {
		Audio.playSound(footsteps[walkNumber], options);
	}
}

// put the fingers into a relaxed pose
function curlFingers() {

    // left hand fingers
    for(var i = 18 ; i < 34 ; i++) {
        MyAvatar.setJointData(jointList[i], Quat.fromPitchYawRollDegrees(8,0,0));
    }
    // left hand thumb
    for(var i = 34 ; i < 38 ; i++) {
        MyAvatar.setJointData(jointList[i], Quat.fromPitchYawRollDegrees(0,0,0));
    }
    // right hand fingers
    for(var i = 42 ; i < 58 ; i++) {
        MyAvatar.setJointData(jointList[i], Quat.fromPitchYawRollDegrees(8,0,0));
    }
    // right hand thumb
    for(var i = 58 ; i < 62 ; i++) {
        MyAvatar.setJointData(jointList[i], Quat.fromPitchYawRollDegrees(0,0,0));
    }
}

// additional maths functions
function degToRad( degreesValue ) { return degreesValue * Math.PI / 180; }
function radToDeg( radiansValue ) { return radiansValue * 180 / Math.PI; }

// animateAvatar - sine wave generators working like clockwork
function animateAvatar( deltaTime, velocity, principleDirection ) {

	    // adjusting the walk speed in edit mode causes a nasty flicker. pausing the animation stops this
        if(paused) return;

		var cycle = cumulativeTime;
        var transitionProgress = 1;
        var adjustedFrequency = currentAnimation.settings.baseFrequency;

		// upper legs phase reversal for walking backwards
        var forwardModifier = 1;
        if(principleDirection===DIRECTION_BACKWARDS)
			forwardModifier = -1;

        // don't want to lean forwards if going directly upwards
        var leanPitchModifier = 1;
        if(principleDirection===DIRECTION_UP)
			leanPitchModifier = 0;

        // is there a Transition to include?
        if(currentTransition!==null) {

			// if is a new transiton
			if(currentTransition.progress===0) {

				if( currentTransition.walkingAtStart ) {

					if( INTERNAL_STATE !== SIDE_STEPPING ) {

						// work out where we want the walk cycle to stop
						var leftStop  = selectedWalk.settings.stopAngleForwards + 180;
						var rightStop = selectedWalk.settings.stopAngleForwards;
						if( principleDirection === DIRECTION_BACKWARDS ) {
							leftStop  = selectedWalk.settings.stopAngleBackwards + 180;
							rightStop = selectedWalk.settings.stopAngleBackwards;
						}

						// find the closest stop point from the walk wheel's angle
						var angleToLeftStop  = 180 - Math.abs( Math.abs( walkWheelPosition - leftStop  ) - 180);
						var angleToRightStop = 180 - Math.abs( Math.abs( walkWheelPosition - rightStop ) - 180);
						if( walkWheelPosition > angleToLeftStop ) angleToLeftStop = 360 - angleToLeftStop;
						if( walkWheelPosition > angleToRightStop ) angleToRightStop = 360 - angleToRightStop;

						currentTransition.walkWheelIncrement = 6;

						// keep the walkwheel turning by setting the walkWheelIncrement until our feet are tucked nicely underneath us.
						if( angleToLeftStop < angleToRightStop ) {

							currentTransition.walkStopAngle = leftStop;

						} else {

							currentTransition.walkStopAngle = rightStop;
						}

					} else {

						// freeze wheel for sidestepping transitions
						currentTransition.walkWheelIncrement = 0;
					}
				}
			} // end if( currentTransition.walkingAtStart )

			// calculate the Transition progress
			var elapasedTime = (new Date().getTime() - currentTransition.startTime) / 1000;
			currentTransition.progress = elapasedTime / currentTransition.transitionDuration;
			transitionProgress = getBezier((1-currentTransition.progress), {x:0,y:0}, currentTransition.easingLower, currentTransition.easingUpper, {x:1,y:1}).y;

			if(currentTransition.progress>=1) {

				// time to kill off the transition
				delete currentTransition;
				currentTransition = null;

			} else {

				if( currentTransition.walkingAtStart ) {

					if( INTERNAL_STATE !== SIDE_STEPPING ) {

						// if at a stop angle, hold the walk wheel position for remainder of transition
						var tolerance = 7;  // must be greater than the walkWheel increment
						if(( walkWheelPosition > (currentTransition.walkStopAngle - tolerance )) &&
						   ( walkWheelPosition < (currentTransition.walkStopAngle + tolerance ))) {

							currentTransition.walkWheelIncrement = 0;
						}
						// keep turning walk wheel until both feet are below the avi
						walkWheelPosition += currentTransition.walkWheelIncrement;
						currentTransition.walkWheelAdvance += currentTransition.walkWheelIncrement;
					}
				}
			}
		}

        // will we need to use the walk wheel this frame?
        if(currentAnimation  === selectedWalk ||
           currentAnimation  === selectedSideStepLeft ||
           currentAnimation  === selectedSideStepRight ||
           currentTransition !== null) {

			// set the stride length
			if(	INTERNAL_STATE!==SIDE_STEPPING &&
		    	INTERNAL_STATE!==CONFIG_SIDESTEP_LEFT &&
		    	INTERNAL_STATE!==CONFIG_SIDESTEP_RIGHT &&
				currentTransition===null ) {

				// if the timing's right, take a snapshot of the stride max and recalibrate
				var tolerance = 1.0; // higher the number, the higher the chance of a calibration taking place, but is traded off with lower accuracy
				var strideOne = 40;
				var strideTwo = 220;

				if( principleDirection === DIRECTION_BACKWARDS ) {
					strideOne = 130;
					strideTwo = 300;
				}

				if(( walkWheelPosition < (strideOne+tolerance) && walkWheelPosition > (strideOne-tolerance) ) ||
				  (  walkWheelPosition < (strideTwo+tolerance) && walkWheelPosition > (strideTwo-tolerance) )) {

					// calculate the feet's offset from each other (in local Z only)
					var footRPos = localToGlobal(MyAvatar.getJointPosition("RightFoot"));
					var footLPos = localToGlobal(MyAvatar.getJointPosition("LeftFoot"));
					var footOffsetZ = Math.abs(footRPos.z - footLPos.z);
					if(footOffsetZ>1) strideLength = 2 * footOffsetZ + aviFootSize.z; // sometimes getting very low value here - just ignore for now
					if(statsOn) print('Stride length calibrated to '+strideLength.toFixed(4)+' metres at '+walkWheelPosition.toFixed(1)+' degrees');
					if(principleDirection===DIRECTION_FORWARDS) {
						currentAnimation.calibration.strideLengthForwards = strideLength;
					} else if (principleDirection===DIRECTION_BACKWARDS) {
						currentAnimation.calibration.strideLengthBackwards = strideLength;
					}

				} else {

					if(principleDirection===DIRECTION_FORWARDS)
						strideLength = currentAnimation.calibration.strideLengthForwards;
					else if (principleDirection===DIRECTION_BACKWARDS)
						strideLength = currentAnimation.calibration.strideLengthBackwards;
				}
			}
			else if(( INTERNAL_STATE===SIDE_STEPPING ||
		        	  INTERNAL_STATE===CONFIG_SIDESTEP_LEFT ||
		    		  INTERNAL_STATE===CONFIG_SIDESTEP_RIGHT )  ) {

				// calculate lateral stride - same same, but different
				// if the timing's right, take a snapshot of the stride max and recalibrate the stride length
				var tolerance = 1.0; // higher the number, the higher the chance of a calibration taking place, but is traded off with lower accuracy
				if(principleDirection===DIRECTION_LEFT) {

					if( walkWheelPosition < (3+tolerance) && walkWheelPosition > (3-tolerance) ) {

						// calculate the feet's offset from the hips (in local X only)
						var footRPos = localToGlobal(MyAvatar.getJointPosition("RightFoot"));
						var footLPos = localToGlobal(MyAvatar.getJointPosition("LeftFoot"));
						var footOffsetX = Math.abs(footRPos.x - footLPos.x);
						strideLength = footOffsetX;
						if(statsOn) print('Stride width calibrated to '+strideLength.toFixed(4)+ ' metres at '+walkWheelPosition.toFixed(1)+' degrees');
						currentAnimation.calibration.strideLengthLeft = strideLength;

					} else {

						strideLength = currentAnimation.calibration.strideLengthLeft;
					}
				}
				else if (principleDirection===DIRECTION_RIGHT) {

					if( walkWheelPosition < (170+tolerance) && walkWheelPosition > (170-tolerance) ) {

						// calculate the feet's offset from the hips (in local X only)
						var footRPos = localToGlobal(MyAvatar.getJointPosition("RightFoot"));
						var footLPos = localToGlobal(MyAvatar.getJointPosition("LeftFoot"));
						var footOffsetX = Math.abs(footRPos.x - footLPos.x);
						strideLength = footOffsetX;
						if(statsOn) print('Stride width calibrated to '+strideLength.toFixed(4)+ ' metres at '+walkWheelPosition.toFixed(1)+' degrees');
						currentAnimation.calibration.strideLengthRight = strideLength;

					} else {

						strideLength = currentAnimation.calibration.strideLengthRight;
					}
				}
			} // end stride length calculations

			// wrap the stride length around a 'surveyor's wheel' twice and calculate the angular velocity at the given (linear) velocity
			// omega = v / r , where r = circumference / 2 PI , where circumference = 2 * stride length
			var wheelRadius = strideLength / Math.PI;
			var angularVelocity = velocity / wheelRadius;

			// calculate the degrees turned (at this angular velocity) since last frame
			var radiansTurnedSinceLastFrame = deltaTime * angularVelocity;
			var degreesTurnedSinceLastFrame = radToDeg(radiansTurnedSinceLastFrame);

			// if we are in an edit mode, we will need fake time to turn the wheel
			if( INTERNAL_STATE!==WALKING &&
				INTERNAL_STATE!==SIDE_STEPPING )
				degreesTurnedSinceLastFrame = currentAnimation.settings.baseFrequency / 70;

			if( walkWheelPosition >= 360 )
				walkWheelPosition = walkWheelPosition % 360;

			// advance the walk wheel the appropriate amount
			if( currentTransition===null || currentTransition.walkingAtEnd )
			    walkWheelPosition += degreesTurnedSinceLastFrame;

			// set the new values for the exact correct walk cycle speed at this velocity
			adjustedFrequency = 1;
			cycle = walkWheelPosition;

			// show stats and walk wheel?
			if(statsOn) {

				var distanceTravelled = velocity * deltaTime;
				var deltaTimeMS = deltaTime * 1000;

				if( INTERNAL_STATE===SIDE_STEPPING ||
		    		INTERNAL_STATE===CONFIG_SIDESTEP_LEFT ||
		    		INTERNAL_STATE===CONFIG_SIDESTEP_RIGHT ) {

					// draw the walk wheel turning around the z axis for sidestepping
					var directionSign = 1;
					if(principleDirection===DIRECTION_RIGHT) directionSign = -1;
					var yOffset = hipsToFeetDistance - (wheelRadius/1.2); // /1.2 is a visual kludge, probably necessary because of either the 'avi feet penetrate floor' issue - TODO - once ground plane following is in Interface, lock this down
					var sinWalkWheelPosition = wheelRadius * Math.sin(degToRad( directionSign * walkWheelPosition ));
					var cosWalkWheelPosition = wheelRadius * Math.cos(degToRad( directionSign * -walkWheelPosition ));
					var wheelXPos = {x: cosWalkWheelPosition, y:-sinWalkWheelPosition - yOffset, z: 0};
					var wheelXEnd = {x: -cosWalkWheelPosition, y:sinWalkWheelPosition - yOffset, z: 0};
					sinWalkWheelPosition = wheelRadius * Math.sin(degToRad( -directionSign * walkWheelPosition+90 ));
					cosWalkWheelPosition = wheelRadius * Math.cos(degToRad( -directionSign * walkWheelPosition+90 ));
					var wheelYPos = {x:cosWalkWheelPosition, y:sinWalkWheelPosition - yOffset, z: 0};
					var wheelYEnd = {x:-cosWalkWheelPosition, y:-sinWalkWheelPosition - yOffset, z: 0};
					Overlays.editOverlay(walkWheelYLine, {visible: true, position:wheelYPos, end:wheelYEnd});
					Overlays.editOverlay(walkWheelZLine, {visible: true, position:wheelXPos, end:wheelXEnd});

				} else {

					// draw the walk wheel turning around the x axis for walking forwards or backwards
					var yOffset = hipsToFeetDistance - (wheelRadius/1.2); // /1.2 is a visual kludge, probably necessary because of either the 'avi feet penetrate floor' issue - TODO - once ground plane following is in Interface, lock this down
					var sinWalkWheelPosition = wheelRadius * Math.sin(degToRad((forwardModifier*-1) * walkWheelPosition));
					var cosWalkWheelPosition = wheelRadius * Math.cos(degToRad((forwardModifier*-1) * -walkWheelPosition));
					var wheelZPos = {x:0, y:-sinWalkWheelPosition - yOffset, z: cosWalkWheelPosition};
					var wheelZEnd = {x:0, y:sinWalkWheelPosition - yOffset, z: -cosWalkWheelPosition};
					sinWalkWheelPosition = wheelRadius * Math.sin(degToRad(forwardModifier * walkWheelPosition+90));
					cosWalkWheelPosition = wheelRadius * Math.cos(degToRad(forwardModifier * walkWheelPosition+90));
					var wheelYPos = {x:0, y:sinWalkWheelPosition - yOffset, z: cosWalkWheelPosition};
					var wheelYEnd = {x:0, y:-sinWalkWheelPosition - yOffset, z: -cosWalkWheelPosition};
					Overlays.editOverlay(walkWheelYLine, { visible: true, position:wheelYPos, end:wheelYEnd });
					Overlays.editOverlay(walkWheelZLine, { visible: true, position:wheelZPos, end:wheelZEnd });
				}

				// populate stats overlay
				var walkWheelInfo =
					'         Walk Wheel Stats\n--------------------------------------\n \n \n'
					+ '\nFrame time: '+deltaTimeMS.toFixed(2)
					+ ' mS\nVelocity: '+velocity.toFixed(2)
					+ ' m/s\nDistance: '+distanceTravelled.toFixed(3)
					+ ' m\nOmega: '+angularVelocity.toFixed(3)
					+ ' rad / s\nDeg to turn: '+degreesTurnedSinceLastFrame.toFixed(2)
					+ ' deg\nWheel position: '+cycle.toFixed(1)
					+ ' deg\nWheel radius: '+wheelRadius.toFixed(3)
					+ ' m\nHips To Feet: '+hipsToFeetDistance.toFixed(3)
					+ ' m\nStride: '+strideLength.toFixed(3)
					+ ' m\n';
				Overlays.editOverlay(walkWheelStats, {text: walkWheelInfo});
			}
		} // end of walk wheel and stride length calculation


		// Start applying motion
		var pitchOscillation = 0;
		var yawOscillation = 0;
		var rollOscillation = 0;
		var pitchOscillationLast = 0;
		var yawOscillationLast = 0;
		var rollOscillationLast = 0;
        var pitchOffset = 0;
        var yawOffset = 0;
        var rollOffset = 0;
        var pitchOffsetLast = 0;
        var yawOffsetLast = 0;
        var rollOffsetLast = 0;

		// calcualte any hips translation
		// Note: can only apply hips translations whilst in a config (edit) mode at present, not whilst under locomotion
		if( INTERNAL_STATE===CONFIG_WALK_STYLES ||
		    INTERNAL_STATE===CONFIG_WALK_TWEAKS ||
		    INTERNAL_STATE===CONFIG_WALK_JOINTS ||
		    INTERNAL_STATE===CONFIG_SIDESTEP_LEFT  ||
		    INTERNAL_STATE===CONFIG_SIDESTEP_RIGHT ||
		    INTERNAL_STATE===CONFIG_FLYING ||
		    INTERNAL_STATE===CONFIG_FLYING_UP ||
		    INTERNAL_STATE===CONFIG_FLYING_DOWN ) {

			// calculate hips translation
			var motorOscillation  = Math.sin(degToRad((cycle * adjustedFrequency * 2) + currentAnimation.joints[0].thrustPhase)) + currentAnimation.joints[0].thrustOffset;
			var swayOscillation   = Math.sin(degToRad((cycle * adjustedFrequency    ) + currentAnimation.joints[0].swayPhase)) + currentAnimation.joints[0].swayOffset;
			var bobOscillation    = Math.sin(degToRad((cycle * adjustedFrequency * 2) + currentAnimation.joints[0].bobPhase)) + currentAnimation.joints[0].bobOffset;

			// apply hips translation
			translateHips({x:swayOscillation*currentAnimation.joints[0].sway, y:motorOscillation*currentAnimation.joints[0].thrust, z:bobOscillation*currentAnimation.joints[0].bob});
		}

        // hips rotation
        // apply the current Transition?
        if(currentTransition!==null) {

			if( currentTransition.walkingAtStart ) {

				pitchOscillation = currentAnimation.joints[0].pitch * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency * 2)
									   + currentAnimation.joints[0].pitchPhase)) + currentAnimation.joints[0].pitchOffset;

				yawOscillation   = currentAnimation.joints[0].yaw   * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency )
									   + currentAnimation.joints[0].yawPhase)) + currentAnimation.joints[0].yawOffset;

				rollOscillation  = (currentAnimation.joints[0].roll  * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency )
									   + currentAnimation.joints[0].rollPhase)) + currentAnimation.joints[0].rollOffset);

				pitchOscillationLast = currentTransition.lastAnimation.joints[0].pitch * Math.sin(degToRad(( walkWheelPosition * 2)
									   + currentTransition.lastAnimation.joints[0].pitchPhase)) + currentTransition.lastAnimation.joints[0].pitchOffset;

				yawOscillationLast = currentTransition.lastAnimation.joints[0].yaw * Math.sin(degToRad(( walkWheelPosition )
									   + currentTransition.lastAnimation.joints[0].yawPhase)) + currentTransition.lastAnimation.joints[0].yawOffset;

				rollOscillationLast = (currentTransition.lastAnimation.joints[0].roll * Math.sin(degToRad(( walkWheelPosition )
									   + currentTransition.lastAnimation.joints[0].rollPhase)) + currentTransition.lastAnimation.joints[0].rollOffset);

			} else {

				pitchOscillation = currentAnimation.joints[0].pitch * Math.sin(degToRad((cycle * adjustedFrequency * 2)
									   + currentAnimation.joints[0].pitchPhase)) + currentAnimation.joints[0].pitchOffset;

				yawOscillation   = currentAnimation.joints[0].yaw   * Math.sin(degToRad((cycle * adjustedFrequency )
									   + currentAnimation.joints[0].yawPhase))   + currentAnimation.joints[0].yawOffset;

				rollOscillation  = (currentAnimation.joints[0].roll  * Math.sin(degToRad((cycle * adjustedFrequency )
									   + currentAnimation.joints[0].rollPhase))  + currentAnimation.joints[0].rollOffset);

				pitchOscillationLast = currentTransition.lastAnimation.joints[0].pitch * Math.sin(degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency * 2)
									   + currentTransition.lastAnimation.joints[0].pitchPhase)) + currentTransition.lastAnimation.joints[0].pitchOffset;

				yawOscillationLast = currentTransition.lastAnimation.joints[0].yaw * Math.sin(degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
									   + currentTransition.lastAnimation.joints[0].yawPhase)) + currentTransition.lastAnimation.joints[0].yawOffset;

				rollOscillationLast = (currentTransition.lastAnimation.joints[0].roll * Math.sin(degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
									   + currentTransition.lastAnimation.joints[0].rollPhase)) + currentTransition.lastAnimation.joints[0].rollOffset);

			}

			pitchOscillation = (transitionProgress * pitchOscillation) + ((1-transitionProgress) * pitchOscillationLast);
			yawOscillation   = (transitionProgress * yawOscillation)   + ((1-transitionProgress) * yawOscillationLast);
			rollOscillation  = (transitionProgress * rollOscillation)  + ((1-transitionProgress) * rollOscillationLast);

		} else {

			pitchOscillation = currentAnimation.joints[0].pitch * Math.sin(degToRad((cycle * adjustedFrequency * 2)
								   + currentAnimation.joints[0].pitchPhase)) + currentAnimation.joints[0].pitchOffset;

			yawOscillation   = currentAnimation.joints[0].yaw   * Math.sin(degToRad((cycle * adjustedFrequency )
								   + currentAnimation.joints[0].yawPhase))   + currentAnimation.joints[0].yawOffset;

			rollOscillation  = (currentAnimation.joints[0].roll  * Math.sin(degToRad((cycle * adjustedFrequency )
								   + currentAnimation.joints[0].rollPhase))  + currentAnimation.joints[0].rollOffset);
		}

        // apply hips rotation
        MyAvatar.setJointData("Hips", Quat.fromPitchYawRollDegrees(-pitchOscillation + (leanPitchModifier * getLeanPitch(velocity)),   	// getLeanPitch - lean forwards as velocity increases
                                                                    yawOscillation,                                       				// Yup, that's correct ;-)
                                                                    rollOscillation + getLeanRoll(deltaTime, velocity))); 				// getLeanRoll - banking on cornering

		// upper legs
		if( INTERNAL_STATE!==SIDE_STEPPING &&
		    INTERNAL_STATE!==CONFIG_SIDESTEP_LEFT &&
		    INTERNAL_STATE!==CONFIG_SIDESTEP_RIGHT ) {

			// apply the current Transition to the upper legs?
			if(currentTransition!==null) {

				if(currentTransition.walkingAtStart) {

					pitchOscillation = currentAnimation.joints[1].pitch * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency ) + (forwardModifier * currentAnimation.joints[1].pitchPhase)));
					yawOscillation   = currentAnimation.joints[1].yaw   * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency ) + currentAnimation.joints[1].yawPhase));
					rollOscillation  = currentAnimation.joints[1].roll  * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency ) + currentAnimation.joints[1].rollPhase));

					pitchOffset = currentAnimation.joints[1].pitchOffset;
					yawOffset = currentAnimation.joints[1].yawOffset;
					rollOffset = currentAnimation.joints[1].rollOffset;

					pitchOscillationLast = currentTransition.lastAnimation.joints[1].pitch
											   * Math.sin(degToRad( walkWheelPosition + (forwardModifier * currentTransition.lastAnimation.joints[1].pitchPhase )));

					yawOscillationLast   = currentTransition.lastAnimation.joints[1].yaw
											   * Math.sin(degToRad( walkWheelPosition + currentTransition.lastAnimation.joints[1].yawPhase ));

					rollOscillationLast  = currentTransition.lastAnimation.joints[1].roll
											   * Math.sin(degToRad( walkWheelPosition + currentTransition.lastAnimation.joints[1].rollPhase ));

					pitchOffsetLast = currentTransition.lastAnimation.joints[1].pitchOffset;
					yawOffsetLast = currentTransition.lastAnimation.joints[1].yawOffset;
					rollOffsetLast = currentTransition.lastAnimation.joints[1].rollOffset;

				} else {

					pitchOscillation = currentAnimation.joints[1].pitch * Math.sin(degToRad((cycle * adjustedFrequency ) + (forwardModifier * currentAnimation.joints[1].pitchPhase)));
					yawOscillation   = currentAnimation.joints[1].yaw   * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[1].yawPhase));
					rollOscillation  = currentAnimation.joints[1].roll  * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[1].rollPhase));

					pitchOffset = currentAnimation.joints[1].pitchOffset;
					yawOffset   = currentAnimation.joints[1].yawOffset;
					rollOffset  = currentAnimation.joints[1].rollOffset;

					pitchOscillationLast = currentTransition.lastAnimation.joints[1].pitch
											   * Math.sin(degToRad( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency
											   + (forwardModifier * currentTransition.lastAnimation.joints[1].pitchPhase )));

					yawOscillationLast   = currentTransition.lastAnimation.joints[1].yaw
											   * Math.sin(degToRad( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency
											   + currentTransition.lastAnimation.joints[1].yawPhase ));

					rollOscillationLast  = currentTransition.lastAnimation.joints[1].roll
											   * Math.sin(degToRad( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency
											   + currentTransition.lastAnimation.joints[1].rollPhase ));

					pitchOffsetLast = currentTransition.lastAnimation.joints[1].pitchOffset;
					yawOffsetLast = currentTransition.lastAnimation.joints[1].yawOffset;
					rollOffsetLast = currentTransition.lastAnimation.joints[1].rollOffset;
				}

				pitchOscillation = (transitionProgress * pitchOscillation ) + ((1-transitionProgress) * pitchOscillationLast);
				yawOscillation   = (transitionProgress * yawOscillation )   + ((1-transitionProgress) * yawOscillationLast);
				rollOscillation  = (transitionProgress * rollOscillation )  + ((1-transitionProgress) * rollOscillationLast);

				pitchOffset = (transitionProgress * pitchOffset) + ((1-transitionProgress) * pitchOffsetLast);
				yawOffset = (transitionProgress * yawOffset) + ((1-transitionProgress) * yawOffsetLast);
				rollOffset = (transitionProgress * rollOffset) + ((1-transitionProgress) * rollOffsetLast);

			} else {

				pitchOscillation = currentAnimation.joints[1].pitch * Math.sin(degToRad((cycle * adjustedFrequency ) + (forwardModifier * currentAnimation.joints[1].pitchPhase)));
				yawOscillation   = currentAnimation.joints[1].yaw   * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[1].yawPhase));
				rollOscillation  = currentAnimation.joints[1].roll  * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[1].rollPhase));

				pitchOffset = currentAnimation.joints[1].pitchOffset;
				yawOffset   = currentAnimation.joints[1].yawOffset;
				rollOffset  = currentAnimation.joints[1].rollOffset;
			}

			// apply the upper leg rotations
			MyAvatar.setJointData("RightUpLeg", Quat.fromPitchYawRollDegrees(  pitchOscillation + pitchOffset, yawOscillation + yawOffset, -rollOscillation - rollOffset ));
			MyAvatar.setJointData("LeftUpLeg",  Quat.fromPitchYawRollDegrees( -pitchOscillation + pitchOffset, yawOscillation - yawOffset, -rollOscillation + rollOffset ));


			// lower leg
			if(currentTransition!==null) {

				if(currentTransition.walkingAtStart) {

					pitchOscillation = currentAnimation.joints[2].pitch * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency ) + currentAnimation.joints[2].pitchPhase));
					yawOscillation   = currentAnimation.joints[2].yaw   * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency ) + currentAnimation.joints[2].yawPhase));
					rollOscillation  = currentAnimation.joints[2].roll  * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency ) + currentAnimation.joints[2].rollPhase));

					pitchOffset = currentAnimation.joints[2].pitchOffset;
					yawOffset = currentAnimation.joints[2].yawOffset;
					rollOffset = currentAnimation.joints[2].rollOffset;

					pitchOscillationLast = currentTransition.lastAnimation.joints[2].pitch * Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[2].pitchPhase));
					yawOscillationLast   = currentTransition.lastAnimation.joints[2].yaw   * Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[2].yawPhase));
					rollOscillationLast  = currentTransition.lastAnimation.joints[2].roll  * Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[2].rollPhase));

					pitchOffsetLast = currentTransition.lastAnimation.joints[2].pitchOffset;
					yawOffsetLast = currentTransition.lastAnimation.joints[2].yawOffset;
					rollOffsetLast = currentTransition.lastAnimation.joints[2].rollOffset;

				} else {

					pitchOscillation = currentAnimation.joints[2].pitch * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[2].pitchPhase));
					yawOscillation   = currentAnimation.joints[2].yaw   * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[2].yawPhase));
					rollOscillation  = currentAnimation.joints[2].roll  * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[2].rollPhase));

					pitchOffset = currentAnimation.joints[2].pitchOffset;
					yawOffset   = currentAnimation.joints[2].yawOffset;
					rollOffset  = currentAnimation.joints[2].rollOffset;

					pitchOscillationLast = currentTransition.lastAnimation.joints[2].pitch
											* Math.sin(degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
										   	+ currentTransition.lastAnimation.joints[2].pitchPhase));
					yawOscillationLast   = currentTransition.lastAnimation.joints[2].yaw
											* Math.sin(degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
					                       	+ currentTransition.lastAnimation.joints[2].yawPhase));
					rollOscillationLast  = currentTransition.lastAnimation.joints[2].roll
											* Math.sin(degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
					                        + currentTransition.lastAnimation.joints[2].rollPhase));

					pitchOffsetLast = currentTransition.lastAnimation.joints[2].pitchOffset;
					yawOffsetLast = currentTransition.lastAnimation.joints[2].yawOffset;
					rollOffsetLast = currentTransition.lastAnimation.joints[2].rollOffset;
				}

				pitchOscillation = ( transitionProgress * pitchOscillation ) + ( (1-transitionProgress) * pitchOscillationLast );
				yawOscillation   = ( transitionProgress * yawOscillation )   + ( (1-transitionProgress) * yawOscillationLast );
				rollOscillation  = ( transitionProgress * rollOscillation )  + ( (1-transitionProgress) * rollOscillationLast );

				pitchOffset = ( transitionProgress * pitchOffset ) + ( (1-transitionProgress) * pitchOffsetLast );
				yawOffset   = ( transitionProgress * yawOffset )   + ( (1-transitionProgress) * yawOffsetLast );
				rollOffset  = ( transitionProgress * rollOffset )  + ( (1-transitionProgress) * rollOffsetLast );

			} else {

				pitchOscillation = currentAnimation.joints[2].pitch * Math.sin( degToRad(( cycle * adjustedFrequency ) + currentAnimation.joints[2].pitchPhase ));
				yawOscillation   = currentAnimation.joints[2].yaw   * Math.sin( degToRad(( cycle * adjustedFrequency ) + currentAnimation.joints[2].yawPhase ));
				rollOscillation  = currentAnimation.joints[2].roll  * Math.sin( degToRad(( cycle * adjustedFrequency ) + currentAnimation.joints[2].rollPhase ));

				pitchOffset = currentAnimation.joints[2].pitchOffset;
				yawOffset   = currentAnimation.joints[2].yawOffset;
				rollOffset  = currentAnimation.joints[2].rollOffset;
			}

			// apply lower leg joint rotations
			MyAvatar.setJointData("RightLeg", Quat.fromPitchYawRollDegrees( -pitchOscillation + pitchOffset, yawOscillation - yawOffset, rollOscillation - rollOffset ));
			MyAvatar.setJointData("LeftLeg",  Quat.fromPitchYawRollDegrees(  pitchOscillation + pitchOffset, yawOscillation + yawOffset, rollOscillation + rollOffset ));

	   } // end if !SIDE_STEPPING

	   else if( INTERNAL_STATE===SIDE_STEPPING ||
		    	INTERNAL_STATE===CONFIG_SIDESTEP_LEFT ||
		    	INTERNAL_STATE===CONFIG_SIDESTEP_RIGHT ) {

		   	// sidestepping uses the sinewave generators slightly differently for the legs
		   	pitchOscillation = currentAnimation.joints[1].pitch * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[1].pitchPhase));
			yawOscillation   = currentAnimation.joints[1].yaw   * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[1].yawPhase));
			rollOscillation  = currentAnimation.joints[1].roll  * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[1].rollPhase));

			// apply upper leg rotations for sidestepping
			MyAvatar.setJointData("RightUpLeg", Quat.fromPitchYawRollDegrees(
			   -pitchOscillation + currentAnimation.joints[1].pitchOffset,
				yawOscillation + currentAnimation.joints[1].yawOffset,
			    rollOscillation  + currentAnimation.joints[1].rollOffset ));
			MyAvatar.setJointData("LeftUpLeg",  Quat.fromPitchYawRollDegrees(
			    pitchOscillation + currentAnimation.joints[1].pitchOffset,
				yawOscillation - currentAnimation.joints[1].yawOffset,
			   -rollOscillation  - currentAnimation.joints[1].rollOffset ));

			// calculate lower leg joint rotations for sidestepping
			pitchOscillation = currentAnimation.joints[2].pitch * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[2].pitchPhase));
			yawOscillation   = currentAnimation.joints[2].yaw   * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[2].yawPhase));
			rollOscillation  = currentAnimation.joints[2].roll  * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[2].rollPhase));

			// apply lower leg joint rotations
			MyAvatar.setJointData("RightLeg", Quat.fromPitchYawRollDegrees(
				-pitchOscillation + currentAnimation.joints[2].pitchOffset,
				 yawOscillation - currentAnimation.joints[2].yawOffset,
				 rollOscillation - currentAnimation.joints[2].rollOffset)); // TODO: needs a kick just before fwd peak
			MyAvatar.setJointData("LeftLeg",  Quat.fromPitchYawRollDegrees(
				 pitchOscillation + currentAnimation.joints[2].pitchOffset,
				 yawOscillation + currentAnimation.joints[2].yawOffset,
				 rollOscillation + currentAnimation.joints[2].rollOffset));
	   	}

		// feet
		if(currentTransition!==null) {

			if(currentTransition.walkingAtStart) {

				pitchOscillation = currentAnimation.joints[3].pitch * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency ) + currentAnimation.joints[3].pitchPhase));
				yawOscillation   = currentAnimation.joints[3].yaw   * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency ) + currentAnimation.joints[3].yawPhase));
				rollOscillation  = currentAnimation.joints[3].roll  * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency ) + currentAnimation.joints[3].rollPhase));

				pitchOffset = currentAnimation.joints[3].pitchOffset;
				yawOffset = currentAnimation.joints[3].yawOffset;
				rollOffset = currentAnimation.joints[3].rollOffset;

				pitchOscillationLast = currentTransition.lastAnimation.joints[3].pitch * Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[3].pitchPhase));
				yawOscillationLast   = currentTransition.lastAnimation.joints[3].yaw   * Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[3].yawPhase));
				rollOscillationLast  = currentTransition.lastAnimation.joints[3].roll  * Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[3].rollPhase));

				pitchOffsetLast = currentTransition.lastAnimation.joints[3].pitchOffset;
				yawOffsetLast = currentTransition.lastAnimation.joints[3].yawOffset;
				rollOffsetLast = currentTransition.lastAnimation.joints[3].rollOffset;

			} else {

				pitchOscillation = currentAnimation.joints[3].pitch * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[3].pitchPhase));
				yawOscillation   = currentAnimation.joints[3].yaw   * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[3].yawPhase));
				rollOscillation  = currentAnimation.joints[3].roll  * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[3].rollPhase));

				pitchOffset = currentAnimation.joints[3].pitchOffset;
				yawOffset   = currentAnimation.joints[3].yawOffset;
				rollOffset  = currentAnimation.joints[3].rollOffset;

				pitchOscillationLast = currentTransition.lastAnimation.joints[3].pitch
										* Math.sin(degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
										+ currentTransition.lastAnimation.joints[3].pitchPhase));

				yawOscillationLast   = currentTransition.lastAnimation.joints[3].yaw
										* Math.sin(degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
										+ currentTransition.lastAnimation.joints[3].yawPhase));

				rollOscillationLast  = currentTransition.lastAnimation.joints[3].roll
										* Math.sin(degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
										+ currentTransition.lastAnimation.joints[3].rollPhase));

				pitchOffsetLast = currentTransition.lastAnimation.joints[3].pitchOffset;
				yawOffsetLast = currentTransition.lastAnimation.joints[3].yawOffset;
				rollOffsetLast = currentTransition.lastAnimation.joints[3].rollOffset;
			}

			pitchOscillation = (transitionProgress * pitchOscillation ) + ((1-transitionProgress) * pitchOscillationLast);
			yawOscillation   = (transitionProgress * yawOscillation )   + ((1-transitionProgress) * yawOscillationLast);
			rollOscillation  = (transitionProgress * rollOscillation )  + ((1-transitionProgress) * rollOscillationLast);

			pitchOffset = (transitionProgress * pitchOffset) + ((1-transitionProgress) * pitchOffsetLast);
			yawOffset = (transitionProgress * yawOffset) + ((1-transitionProgress) * yawOffsetLast);
			rollOffset = (transitionProgress * rollOffset) + ((1-transitionProgress) * rollOffsetLast);

		} else {

			pitchOscillation = currentAnimation.joints[3].pitch * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[3].pitchPhase));
			yawOscillation   = currentAnimation.joints[3].yaw   * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[3].yawPhase));
			rollOscillation  = currentAnimation.joints[3].roll  * Math.sin(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[3].rollPhase));

			pitchOffset = currentAnimation.joints[3].pitchOffset;
			yawOffset   = currentAnimation.joints[3].yawOffset;
			rollOffset  = currentAnimation.joints[3].rollOffset;
		}

		// apply foot rotations
		MyAvatar.setJointData("RightFoot", Quat.fromPitchYawRollDegrees(  pitchOscillation + pitchOffset, yawOscillation + yawOffset, rollOscillation + rollOffset ));
		MyAvatar.setJointData("LeftFoot",  Quat.fromPitchYawRollDegrees( -pitchOscillation + pitchOffset, yawOscillation - yawOffset, rollOscillation - rollOffset ));

		// play footfall sound yet? To determine this, we take the differential of the foot's pitch curve to decide when the foot hits the ground.
		if( INTERNAL_STATE===WALKING ||
		    INTERNAL_STATE===SIDE_STEPPING ||
		    INTERNAL_STATE===CONFIG_WALK_STYLES ||
		    INTERNAL_STATE===CONFIG_WALK_TWEAKS ||
		    INTERNAL_STATE===CONFIG_WALK_JOINTS ||
		    INTERNAL_STATE===CONFIG_SIDESTEP_LEFT ||
		    INTERNAL_STATE===CONFIG_SIDESTEP_RIGHT ) {

			// finding dy/dx is as simple as determining the cosine wave for the foot's pitch function.
			var feetPitchDifferential = Math.cos(degToRad((cycle * adjustedFrequency ) + currentAnimation.joints[3].pitchPhase));
			var threshHold = 0.9; // sets the audio trigger point. with accuracy.
			if(feetPitchDifferential<-threshHold &&
			   nextStep===DIRECTION_LEFT &&
			   principleDirection!==DIRECTION_UP &&
			   principleDirection!==DIRECTION_DOWN) {

				playFootstep(DIRECTION_LEFT);
				nextStep = DIRECTION_RIGHT;
			}
			else if(feetPitchDifferential>threshHold &&
					nextStep===DIRECTION_RIGHT &&
					principleDirection!==DIRECTION_UP &&
					principleDirection!==DIRECTION_DOWN) {

				playFootstep(DIRECTION_RIGHT);
				nextStep = DIRECTION_LEFT;
			}
		}

		// toes
		if(currentTransition!==null) {

			if(currentTransition.walkingAtStart) {

				pitchOscillation = currentAnimation.joints[4].pitch * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency ) + currentAnimation.joints[4].pitchPhase));
				yawOscillation   = currentAnimation.joints[4].yaw   * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency ) + currentAnimation.joints[4].yawPhase)) + currentAnimation.joints[4].yawOffset;
				rollOscillation  = currentAnimation.joints[4].roll  * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency ) + currentAnimation.joints[4].rollPhase)) + currentAnimation.joints[4].rollOffset;

				pitchOffset = currentAnimation.joints[4].pitchOffset;

				pitchOscillationLast = currentTransition.lastAnimation.joints[4].pitch * Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[4].pitchPhase));
				yawOscillationLast   = currentTransition.lastAnimation.joints[4].yaw   * Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[4].yawPhase)) + currentTransition.lastAnimation.joints[4].yawOffset;;
				rollOscillationLast  = currentTransition.lastAnimation.joints[4].roll  * Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[4].rollPhase))+ currentTransition.lastAnimation.joints[4].rollOffset;

				pitchOffsetLast = currentTransition.lastAnimation.joints[4].pitchOffset;

			} else {

				pitchOscillation = currentAnimation.joints[4].pitch * Math.sin(degToRad((cycle * adjustedFrequency) + currentAnimation.joints[4].pitchPhase));
				yawOscillation   = currentAnimation.joints[4].yaw   * Math.sin(degToRad((cycle * adjustedFrequency) + currentAnimation.joints[4].yawPhase)) + currentAnimation.joints[4].yawOffset;
				rollOscillation  = currentAnimation.joints[4].roll  * Math.sin(degToRad((cycle * adjustedFrequency) + currentAnimation.joints[4].rollPhase)) + currentAnimation.joints[4].rollOffset;

				pitchOffset = currentAnimation.joints[4].pitchOffset;

				pitchOscillationLast = currentTransition.lastAnimation.joints[4].pitch
										* Math.sin(degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
										+ currentTransition.lastAnimation.joints[4].pitchPhase));

				yawOscillationLast   = currentTransition.lastAnimation.joints[4].yaw
										* Math.sin(degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
										+ currentTransition.lastAnimation.joints[4].yawPhase)) + currentTransition.lastAnimation.joints[4].yawOffset;;

				rollOscillationLast  = currentTransition.lastAnimation.joints[4].roll
										* Math.sin(degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
										+ currentTransition.lastAnimation.joints[4].rollPhase))+ currentTransition.lastAnimation.joints[4].rollOffset;

				pitchOffsetLast = currentTransition.lastAnimation.joints[4].pitchOffset;
			}

			pitchOscillation = (transitionProgress * pitchOscillation) + ((1-transitionProgress) * pitchOscillationLast);
			yawOscillation   = (transitionProgress * yawOscillation)   + ((1-transitionProgress) * yawOscillationLast);
			rollOscillation  = (transitionProgress * rollOscillation)  + ((1-transitionProgress) * rollOscillationLast);

			pitchOffset = (transitionProgress * pitchOffset) + ((1-transitionProgress) * pitchOffsetLast);

		} else {

			pitchOscillation = currentAnimation.joints[4].pitch * Math.sin(degToRad((cycle * adjustedFrequency) + currentAnimation.joints[4].pitchPhase));
			yawOscillation   = currentAnimation.joints[4].yaw   * Math.sin(degToRad((cycle * adjustedFrequency) + currentAnimation.joints[4].yawPhase)) + currentAnimation.joints[4].yawOffset;
			rollOscillation  = currentAnimation.joints[4].roll  * Math.sin(degToRad((cycle * adjustedFrequency) + currentAnimation.joints[4].rollPhase)) + currentAnimation.joints[4].rollOffset;

			pitchOffset = currentAnimation.joints[4].pitchOffset;
		}

		// apply toe rotations
		MyAvatar.setJointData("RightToeBase", Quat.fromPitchYawRollDegrees(-pitchOscillation + pitchOffset, yawOscillation, rollOscillation));
		MyAvatar.setJointData("LeftToeBase",  Quat.fromPitchYawRollDegrees( pitchOscillation + pitchOffset, yawOscillation, rollOscillation));

		// spine
		if( currentTransition !== null ) {

			if( currentTransition.walkingAtStart ) {

				pitchOscillation = currentAnimation.joints[5].pitch * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency * 2 ) + currentAnimation.joints[5].pitchPhase)) + currentAnimation.joints[5].pitchOffset;
				yawOscillation   = currentAnimation.joints[5].yaw   * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency    ) + currentAnimation.joints[5].yawPhase))   + currentAnimation.joints[5].yawOffset;
				rollOscillation  = currentAnimation.joints[5].roll  * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency    ) + currentAnimation.joints[5].rollPhase))  + currentAnimation.joints[5].rollOffset;

				// calculate where we would have been if we'd continued in the last state
				pitchOscillationLast = currentTransition.lastAnimation.joints[5].pitch
										* Math.sin(degToRad(( walkWheelPosition * 2  ) + currentTransition.lastAnimation.joints[5].pitchPhase))
										+ currentTransition.lastAnimation.joints[5].pitchOffset;
				yawOscillationLast   = currentTransition.lastAnimation.joints[5].yaw
										* Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[5].yawPhase))
										+ currentTransition.lastAnimation.joints[5].yawOffset;
				rollOscillationLast  = currentTransition.lastAnimation.joints[5].roll
										* Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[5].rollPhase))
										+ currentTransition.lastAnimation.joints[5].rollOffset;
			} else {

				pitchOscillation = currentAnimation.joints[5].pitch * Math.sin( degToRad(( cycle * adjustedFrequency * 2)
									+ currentAnimation.joints[5].pitchPhase)) + currentAnimation.joints[5].pitchOffset;

				yawOscillation   = currentAnimation.joints[5].yaw   * Math.sin( degToRad(( cycle * adjustedFrequency )
									+ currentAnimation.joints[5].yawPhase)) + currentAnimation.joints[5].yawOffset;

				rollOscillation  = currentAnimation.joints[5].roll  * Math.sin( degToRad(( cycle * adjustedFrequency )
									+ currentAnimation.joints[5].rollPhase)) + currentAnimation.joints[5].rollOffset;

				pitchOscillationLast = currentTransition.lastAnimation.joints[5].pitch
										* Math.sin( degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency * 2  )
										+ currentTransition.lastAnimation.joints[5].pitchPhase))
										+ currentTransition.lastAnimation.joints[5].pitchOffset;

				yawOscillationLast   = currentTransition.lastAnimation.joints[5].yaw
										* Math.sin( degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
										+ currentTransition.lastAnimation.joints[5].yawPhase))
										+ currentTransition.lastAnimation.joints[5].yawOffset;

				rollOscillationLast  = currentTransition.lastAnimation.joints[5].roll
										* Math.sin( degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
										+ currentTransition.lastAnimation.joints[5].rollPhase))
										+ currentTransition.lastAnimation.joints[5].rollOffset;
			}

			pitchOscillation = (transitionProgress * pitchOscillation ) + ((1-transitionProgress) * pitchOscillationLast);
			yawOscillation   = (transitionProgress * yawOscillation )   + ((1-transitionProgress) * yawOscillationLast);
			rollOscillation  = (transitionProgress * rollOscillation )  + ((1-transitionProgress) * rollOscillationLast);

		} else {

			pitchOscillation = currentAnimation.joints[5].pitch * Math.sin(degToRad(( cycle * adjustedFrequency * 2) + currentAnimation.joints[5].pitchPhase)) + currentAnimation.joints[5].pitchOffset;
			yawOscillation   = currentAnimation.joints[5].yaw   * Math.sin(degToRad(( cycle * adjustedFrequency    ) + currentAnimation.joints[5].yawPhase))   + currentAnimation.joints[5].yawOffset;
			rollOscillation  = currentAnimation.joints[5].roll  * Math.sin(degToRad(( cycle * adjustedFrequency    ) + currentAnimation.joints[5].rollPhase))  + currentAnimation.joints[5].rollOffset;
		}

		// apply spine joint rotations
		MyAvatar.setJointData("Spine", Quat.fromPitchYawRollDegrees( pitchOscillation, yawOscillation, rollOscillation ));

		// spine 1
		if(currentTransition!==null) {

			if(currentTransition.walkingAtStart) {

				pitchOscillation = currentAnimation.joints[6].pitch * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency * 2) + currentAnimation.joints[6].pitchPhase)) + currentAnimation.joints[6].pitchOffset;
				yawOscillation   = currentAnimation.joints[6].yaw   * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency    ) + currentAnimation.joints[6].yawPhase))   + currentAnimation.joints[6].yawOffset;
				rollOscillation  = currentAnimation.joints[6].roll  * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency    ) + currentAnimation.joints[6].rollPhase))  + currentAnimation.joints[6].rollOffset;

				pitchOscillationLast = currentTransition.lastAnimation.joints[6].pitch
										* Math.sin(degToRad(( walkWheelPosition * 2 ) + currentTransition.lastAnimation.joints[6].pitchPhase))
										+ currentTransition.lastAnimation.joints[6].pitchOffset;
				yawOscillationLast   = currentTransition.lastAnimation.joints[6].yaw
										* Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[6].yawPhase))
										+ currentTransition.lastAnimation.joints[6].yawOffset;
				rollOscillationLast  = currentTransition.lastAnimation.joints[6].roll
										* Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[6].rollPhase))
										+ currentTransition.lastAnimation.joints[6].rollOffset;

			} else {

				pitchOscillation = currentAnimation.joints[6].pitch * Math.sin( degToRad(( cycle * adjustedFrequency * 2)
									+ currentAnimation.joints[6].pitchPhase)) + currentAnimation.joints[6].pitchOffset;

				yawOscillation   = currentAnimation.joints[6].yaw   * Math.sin( degToRad(( cycle * adjustedFrequency )
									+ currentAnimation.joints[6].yawPhase))   + currentAnimation.joints[6].yawOffset;

				rollOscillation  = currentAnimation.joints[6].roll  * Math.sin( degToRad(( cycle * adjustedFrequency )
									+ currentAnimation.joints[6].rollPhase))  + currentAnimation.joints[6].rollOffset;

				pitchOscillationLast = currentTransition.lastAnimation.joints[6].pitch
										* Math.sin( degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency * 2 )
										+ currentTransition.lastAnimation.joints[6].pitchPhase))
										+ currentTransition.lastAnimation.joints[6].pitchOffset;

				yawOscillationLast   = currentTransition.lastAnimation.joints[6].yaw
										* Math.sin( degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
										+ currentTransition.lastAnimation.joints[6].yawPhase))
										+ currentTransition.lastAnimation.joints[6].yawOffset;

				rollOscillationLast  = currentTransition.lastAnimation.joints[6].roll
										* Math.sin( degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
										+ currentTransition.lastAnimation.joints[6].rollPhase))
										+ currentTransition.lastAnimation.joints[6].rollOffset;
			}

			pitchOscillation = (transitionProgress * pitchOscillation) + ((1-transitionProgress) * pitchOscillationLast);
			yawOscillation   = (transitionProgress * yawOscillation)   + ((1-transitionProgress) * yawOscillationLast);
			rollOscillation  = (transitionProgress * rollOscillation)  + ((1-transitionProgress) * rollOscillationLast);

		} else {

			pitchOscillation = currentAnimation.joints[6].pitch * Math.sin(degToRad(( cycle * adjustedFrequency * 2) + currentAnimation.joints[6].pitchPhase)) + currentAnimation.joints[6].pitchOffset;
			yawOscillation   = currentAnimation.joints[6].yaw   * Math.sin(degToRad(( cycle * adjustedFrequency    ) + currentAnimation.joints[6].yawPhase))   + currentAnimation.joints[6].yawOffset;
			rollOscillation  = currentAnimation.joints[6].roll  * Math.sin(degToRad(( cycle * adjustedFrequency    ) + currentAnimation.joints[6].rollPhase))  + currentAnimation.joints[6].rollOffset;
		}
		// apply spine1 joint rotations
		MyAvatar.setJointData("Spine1", Quat.fromPitchYawRollDegrees( pitchOscillation, yawOscillation, rollOscillation ));

		// spine 2
		if(currentTransition!==null) {

			if(currentTransition.walkingAtStart) {
				pitchOscillation = currentAnimation.joints[7].pitch * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency * 2) + currentAnimation.joints[7].pitchPhase)) + currentAnimation.joints[7].pitchOffset;
				yawOscillation   = currentAnimation.joints[7].yaw   * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency    ) + currentAnimation.joints[7].yawPhase))   + currentAnimation.joints[7].yawOffset;
				rollOscillation  = currentAnimation.joints[7].roll  * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency    ) + currentAnimation.joints[7].rollPhase))  + currentAnimation.joints[7].rollOffset;

				pitchOscillationLast = currentTransition.lastAnimation.joints[7].pitch
										* Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[7].pitchPhase))
										+ currentTransition.lastAnimation.joints[7].pitchOffset;

				yawOscillationLast   = currentTransition.lastAnimation.joints[7].yaw
										* Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[7].yawPhase))
										+ currentTransition.lastAnimation.joints[7].yawOffset;

				rollOscillationLast  = currentTransition.lastAnimation.joints[7].roll
										* Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[7].rollPhase))
										+ currentTransition.lastAnimation.joints[7].rollOffset;

			} else {

				pitchOscillation = currentAnimation.joints[7].pitch
									* Math.sin( degToRad(( cycle * adjustedFrequency * 2)
									+ currentAnimation.joints[7].pitchPhase))
								   	+ currentAnimation.joints[7].pitchOffset;

				yawOscillation   = currentAnimation.joints[7].yaw
									* Math.sin( degToRad(( cycle * adjustedFrequency    )
									+ currentAnimation.joints[7].yawPhase))
								   	+ currentAnimation.joints[7].yawOffset;

				rollOscillation  = currentAnimation.joints[7].roll
									* Math.sin( degToRad(( cycle * adjustedFrequency    )
									+ currentAnimation.joints[7].rollPhase))
								   	+ currentAnimation.joints[7].rollOffset;

				pitchOscillationLast = currentTransition.lastAnimation.joints[7].pitch
										* Math.sin( degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
										+ currentTransition.lastAnimation.joints[7].pitchPhase))
										+ currentTransition.lastAnimation.joints[7].pitchOffset;

				yawOscillationLast   = currentTransition.lastAnimation.joints[7].yaw
										* Math.sin( degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
										+ currentTransition.lastAnimation.joints[7].yawPhase))
										+ currentTransition.lastAnimation.joints[7].yawOffset;

				rollOscillationLast  = currentTransition.lastAnimation.joints[7].roll
										* Math.sin( degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
										+ currentTransition.lastAnimation.joints[7].rollPhase))
										+ currentTransition.lastAnimation.joints[7].rollOffset;
			}

			pitchOscillation = (transitionProgress * pitchOscillation ) + ((1-transitionProgress) * pitchOscillationLast);
			yawOscillation   = (transitionProgress * yawOscillation )   + ((1-transitionProgress) * yawOscillationLast);
			rollOscillation  = (transitionProgress * rollOscillation )  + ((1-transitionProgress) * rollOscillationLast);

		} else {

			pitchOscillation = currentAnimation.joints[7].pitch * Math.sin(degToRad(( cycle * adjustedFrequency * 2) + currentAnimation.joints[7].pitchPhase))
							   + currentAnimation.joints[7].pitchOffset;

			yawOscillation   = currentAnimation.joints[7].yaw   * Math.sin(degToRad(( cycle * adjustedFrequency    ) + currentAnimation.joints[7].yawPhase))
			                   + currentAnimation.joints[7].yawOffset;

			rollOscillation  = currentAnimation.joints[7].roll  * Math.sin(degToRad(( cycle * adjustedFrequency    ) + currentAnimation.joints[7].rollPhase))
			                   + currentAnimation.joints[7].rollOffset;
		}
		// apply spine2 joint rotations
		MyAvatar.setJointData("Spine2", Quat.fromPitchYawRollDegrees( pitchOscillation, yawOscillation, rollOscillation ));

		if(!armsFree) {

			// shoulders
			if(currentTransition!==null) {

				if(currentTransition.walkingAtStart) {
					pitchOscillation = currentAnimation.joints[8].pitch * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency    )
										+ currentAnimation.joints[8].pitchPhase)) + currentAnimation.joints[8].pitchOffset;

					yawOscillation   = currentAnimation.joints[8].yaw   * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency    )
										+ currentAnimation.joints[8].yawPhase));

					rollOscillation  = currentAnimation.joints[8].roll  * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency * 2)
										+ currentAnimation.joints[8].rollPhase))  + currentAnimation.joints[8].rollOffset;

					yawOffset = currentAnimation.joints[8].yawOffset;


					pitchOscillationLast = currentTransition.lastAnimation.joints[8].pitch
											* Math.sin(degToRad( walkWheelPosition + currentTransition.lastAnimation.joints[8].pitchPhase))
											+ currentTransition.lastAnimation.joints[8].pitchOffset;

					yawOscillationLast   = currentTransition.lastAnimation.joints[8].yaw
											* Math.sin(degToRad( walkWheelPosition + currentTransition.lastAnimation.joints[8].yawPhase))

					rollOscillationLast  = currentTransition.lastAnimation.joints[8].roll
											* Math.sin(degToRad( walkWheelPosition + currentTransition.lastAnimation.joints[8].rollPhase))
											+ currentTransition.lastAnimation.joints[8].rollOffset;

					yawOffsetLast = currentTransition.lastAnimation.joints[8].yawOffset;

				} else {

					pitchOscillation = currentAnimation.joints[8].pitch * Math.sin( degToRad(( cycle * adjustedFrequency    )
										+ currentAnimation.joints[8].pitchPhase)) + currentAnimation.joints[8].pitchOffset;

					yawOscillation   = currentAnimation.joints[8].yaw   * Math.sin( degToRad(( cycle * adjustedFrequency    )
										+ currentAnimation.joints[8].yawPhase));

					rollOscillation  = currentAnimation.joints[8].roll  * Math.sin( degToRad(( cycle * adjustedFrequency * 2)
										+ currentAnimation.joints[8].rollPhase))  + currentAnimation.joints[8].rollOffset;

					yawOffset = currentAnimation.joints[8].yawOffset;

					pitchOscillationLast = currentTransition.lastAnimation.joints[8].pitch
											* Math.sin( degToRad( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency
											+ currentTransition.lastAnimation.joints[8].pitchPhase))
											+ currentTransition.lastAnimation.joints[8].pitchOffset;

					yawOscillationLast   = currentTransition.lastAnimation.joints[8].yaw
											* Math.sin( degToRad( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency
											+ currentTransition.lastAnimation.joints[8].yawPhase))

					rollOscillationLast  = currentTransition.lastAnimation.joints[8].roll
											* Math.sin( degToRad( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency
											+ currentTransition.lastAnimation.joints[8].rollPhase))
											+ currentTransition.lastAnimation.joints[8].rollOffset;

					yawOffsetLast = currentTransition.lastAnimation.joints[8].yawOffset;
				}

				pitchOscillation = (transitionProgress * pitchOscillation) + ((1-transitionProgress) * pitchOscillationLast);
				yawOscillation   = (transitionProgress * yawOscillation)   + ((1-transitionProgress) * yawOscillationLast);
				rollOscillation  = (transitionProgress * rollOscillation)  + ((1-transitionProgress) * rollOscillationLast);

				yawOffset = (transitionProgress * yawOffset) + ((1-transitionProgress) * yawOffsetLast);

			} else {

				pitchOscillation = currentAnimation.joints[8].pitch * Math.sin(degToRad(( cycle * adjustedFrequency    )
									+ currentAnimation.joints[8].pitchPhase)) + currentAnimation.joints[8].pitchOffset;

				yawOscillation   = currentAnimation.joints[8].yaw   * Math.sin(degToRad(( cycle * adjustedFrequency    )
									+ currentAnimation.joints[8].yawPhase));

				rollOscillation  = currentAnimation.joints[8].roll  * Math.sin(degToRad(( cycle * adjustedFrequency * 2)
									+ currentAnimation.joints[8].rollPhase))  + currentAnimation.joints[8].rollOffset;

				yawOffset = currentAnimation.joints[8].yawOffset;
			}

			MyAvatar.setJointData("RightShoulder", Quat.fromPitchYawRollDegrees(pitchOscillation, yawOscillation + yawOffset,  rollOscillation ));
			MyAvatar.setJointData("LeftShoulder",  Quat.fromPitchYawRollDegrees(pitchOscillation, yawOscillation - yawOffset, -rollOscillation ));

			// upper arms
			if(currentTransition!==null) {

				if(currentTransition.walkingAtStart) {

					pitchOscillation = currentAnimation.joints[9].pitch * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency ) + currentAnimation.joints[9].pitchPhase));
					yawOscillation   = currentAnimation.joints[9].yaw   * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency ) + currentAnimation.joints[9].yawPhase));
					rollOscillation  = currentAnimation.joints[9].roll  * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency * 2) + currentAnimation.joints[9].rollPhase)) + currentAnimation.joints[9].rollOffset;

					pitchOffset = currentAnimation.joints[9].pitchOffset;
					yawOffset = currentAnimation.joints[9].yawOffset;

					pitchOscillationLast = currentTransition.lastAnimation.joints[9].pitch
											* Math.sin(degToRad( walkWheelPosition + currentTransition.lastAnimation.joints[9].pitchPhase))

					yawOscillationLast   = currentTransition.lastAnimation.joints[9].yaw
											* Math.sin(degToRad( walkWheelPosition + currentTransition.lastAnimation.joints[9].yawPhase))

					rollOscillationLast  = currentTransition.lastAnimation.joints[9].roll
											* Math.sin(degToRad( walkWheelPosition + currentTransition.lastAnimation.joints[9].rollPhase))
											+ currentTransition.lastAnimation.joints[9].rollOffset;

					pitchOffsetLast = currentTransition.lastAnimation.joints[9].pitchOffset;
					yawOffsetLast = currentTransition.lastAnimation.joints[9].yawOffset;

				} else {

					pitchOscillation = currentAnimation.joints[9].pitch
										* Math.sin( degToRad(( cycle * adjustedFrequency )
										+ currentAnimation.joints[9].pitchPhase));

					yawOscillation   = currentAnimation.joints[9].yaw
										* Math.sin( degToRad(( cycle * adjustedFrequency )
										+ currentAnimation.joints[9].yawPhase));

					rollOscillation  = currentAnimation.joints[9].roll
										* Math.sin(degToRad(( cycle * adjustedFrequency * 2)
										+ currentAnimation.joints[9].rollPhase))
										+ currentAnimation.joints[9].rollOffset;

					pitchOffset = currentAnimation.joints[9].pitchOffset;
					yawOffset = currentAnimation.joints[9].yawOffset;

					pitchOscillationLast = currentTransition.lastAnimation.joints[9].pitch
											* Math.sin( degToRad( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency
											+ currentTransition.lastAnimation.joints[9].pitchPhase))

					yawOscillationLast   = currentTransition.lastAnimation.joints[9].yaw
											* Math.sin( degToRad( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency
											+ currentTransition.lastAnimation.joints[9].yawPhase))

					rollOscillationLast  = currentTransition.lastAnimation.joints[9].roll
											* Math.sin( degToRad( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency
											+ currentTransition.lastAnimation.joints[9].rollPhase))
											+ currentTransition.lastAnimation.joints[9].rollOffset;

					pitchOffsetLast = currentTransition.lastAnimation.joints[9].pitchOffset;
					yawOffsetLast = currentTransition.lastAnimation.joints[9].yawOffset;
				}

				pitchOscillation = (transitionProgress * pitchOscillation) + ((1-transitionProgress) * pitchOscillationLast);
				yawOscillation   = (transitionProgress * yawOscillation)   + ((1-transitionProgress) * yawOscillationLast);
				rollOscillation  = (transitionProgress * rollOscillation)  + ((1-transitionProgress) * rollOscillationLast);

				pitchOffset = (transitionProgress * pitchOffset) + ((1-transitionProgress) * pitchOffsetLast);
				yawOffset   = (transitionProgress * yawOffset)   + ((1-transitionProgress) * yawOffsetLast);

			} else {

				pitchOscillation = currentAnimation.joints[9].pitch
									* Math.sin( degToRad(( cycle * adjustedFrequency )
									+ currentAnimation.joints[9].pitchPhase));

				yawOscillation   = currentAnimation.joints[9].yaw
									* Math.sin( degToRad(( cycle * adjustedFrequency )
									+ currentAnimation.joints[9].yawPhase));

				rollOscillation  = currentAnimation.joints[9].roll
									* Math.sin( degToRad(( cycle * adjustedFrequency * 2)
									+ currentAnimation.joints[9].rollPhase))
									+ currentAnimation.joints[9].rollOffset;

				pitchOffset = currentAnimation.joints[9].pitchOffset;
				yawOffset   = currentAnimation.joints[9].yawOffset;

			}
			MyAvatar.setJointData("RightArm", Quat.fromPitchYawRollDegrees( -pitchOscillation + pitchOffset, yawOscillation - yawOffset,  rollOscillation ));
			MyAvatar.setJointData("LeftArm",  Quat.fromPitchYawRollDegrees(  pitchOscillation + pitchOffset, yawOscillation + yawOffset, -rollOscillation ));

			// forearms
			if(currentTransition!==null) {

				if(currentTransition.walkingAtStart) {

					pitchOscillation = currentAnimation.joints[10].pitch
										* Math.sin( degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency )
										+ currentAnimation.joints[10].pitchPhase ))
										+ currentAnimation.joints[10].pitchOffset;

					yawOscillation   = currentAnimation.joints[10].yaw
										* Math.sin( degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency )
										+ currentAnimation.joints[10].yawPhase ));

					rollOscillation  = currentAnimation.joints[10].roll
										* Math.sin( degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency )
										+ currentAnimation.joints[10].rollPhase ));

					yawOffset = currentAnimation.joints[10].yawOffset;
					rollOffset = currentAnimation.joints[10].rollOffset;

					pitchOscillationLast = currentTransition.lastAnimation.joints[10].pitch
											* Math.sin( degToRad(( walkWheelPosition )
											+ currentTransition.lastAnimation.joints[10].pitchPhase))
											+ currentTransition.lastAnimation.joints[10].pitchOffset;

					yawOscillationLast   = currentTransition.lastAnimation.joints[10].yaw
											* Math.sin( degToRad(( walkWheelPosition )
											+ currentTransition.lastAnimation.joints[10].yawPhase));

					rollOscillationLast  = currentTransition.lastAnimation.joints[10].roll
											* Math.sin( degToRad(( walkWheelPosition )
											+ currentTransition.lastAnimation.joints[10].rollPhase));

					yawOffsetLast  = currentTransition.lastAnimation.joints[10].yawOffset;
					rollOffsetLast = currentTransition.lastAnimation.joints[10].rollOffset;

				} else {

					pitchOscillation = currentAnimation.joints[10].pitch
										* Math.sin( degToRad(( cycle * adjustedFrequency )
										+ currentAnimation.joints[10].pitchPhase ))
										+ currentAnimation.joints[10].pitchOffset;

					yawOscillation   = currentAnimation.joints[10].yaw
										* Math.sin( degToRad(( cycle * adjustedFrequency )
										+ currentAnimation.joints[10].yawPhase ));

					rollOscillation  = currentAnimation.joints[10].roll
										* Math.sin( degToRad(( cycle * adjustedFrequency )
										+ currentAnimation.joints[10].rollPhase ));

					yawOffset  = currentAnimation.joints[10].yawOffset;
					rollOffset = currentAnimation.joints[10].rollOffset;

					pitchOscillationLast = currentTransition.lastAnimation.joints[10].pitch
											* Math.sin( degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
											+ currentTransition.lastAnimation.joints[10].pitchPhase))
											+ currentTransition.lastAnimation.joints[10].pitchOffset;

					yawOscillationLast   = currentTransition.lastAnimation.joints[10].yaw
											* Math.sin( degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
											+ currentTransition.lastAnimation.joints[10].yawPhase));

					rollOscillationLast  = currentTransition.lastAnimation.joints[10].roll
											* Math.sin( degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency )
											+ currentTransition.lastAnimation.joints[10].rollPhase));

					yawOffsetLast  = currentTransition.lastAnimation.joints[10].yawOffset;
					rollOffsetLast = currentTransition.lastAnimation.joints[10].rollOffset;
				}

				// blend the previous and next
				pitchOscillation =  (transitionProgress * pitchOscillation) + ((1-transitionProgress) * pitchOscillationLast);
				yawOscillation   = -(transitionProgress * yawOscillation)   + ((1-transitionProgress) * yawOscillationLast);
				rollOscillation  =  (transitionProgress * rollOscillation)  + ((1-transitionProgress) * rollOscillationLast);

				yawOffset  = (transitionProgress * yawOffset)  + ((1-transitionProgress) * yawOffsetLast);
				rollOffset = (transitionProgress * rollOffset) + ((1-transitionProgress) * rollOffsetLast);

			} else {

				pitchOscillation = currentAnimation.joints[10].pitch
									* Math.sin( degToRad(( cycle * adjustedFrequency )
									+ currentAnimation.joints[10].pitchPhase ))
									+ currentAnimation.joints[10].pitchOffset;

				yawOscillation   = currentAnimation.joints[10].yaw
									* Math.sin( degToRad(( cycle * adjustedFrequency )
									+ currentAnimation.joints[10].yawPhase ));

				rollOscillation  = currentAnimation.joints[10].roll
									* Math.sin( degToRad(( cycle * adjustedFrequency )
									+ currentAnimation.joints[10].rollPhase ));

				yawOffset  = currentAnimation.joints[10].yawOffset;
				rollOffset = currentAnimation.joints[10].rollOffset;
			}

			// apply forearms rotations
			MyAvatar.setJointData("RightForeArm", Quat.fromPitchYawRollDegrees( pitchOscillation,  yawOscillation + yawOffset,  rollOscillation + rollOffset ));
			MyAvatar.setJointData("LeftForeArm",  Quat.fromPitchYawRollDegrees( pitchOscillation,  yawOscillation - yawOffset,  rollOscillation - rollOffset ));

			// hands
			var sideStepSign = 1;
			if(INTERNAL_STATE===SIDE_STEPPING) {
				sideStepSign = 1;
			}
			if(currentTransition!==null) {

				if(currentTransition.walkingAtStart) {

					pitchOscillation = currentAnimation.joints[11].pitch * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency ) + currentAnimation.joints[11].pitchPhase)) + currentAnimation.joints[11].pitchOffset;
					yawOscillation   = currentAnimation.joints[11].yaw   * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency ) + currentAnimation.joints[11].yawPhase))   + currentAnimation.joints[11].yawOffset;
					rollOscillation  = currentAnimation.joints[11].roll  * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency ) + currentAnimation.joints[11].rollPhase))  ;

					pitchOscillationLast = currentTransition.lastAnimation.joints[11].pitch
											* Math.sin(degToRad( walkWheelPosition + currentTransition.lastAnimation.joints[11].pitchPhase))
											+ currentTransition.lastAnimation.joints[11].pitchOffset;

					yawOscillationLast   = currentTransition.lastAnimation.joints[11].yaw
											* Math.sin(degToRad( walkWheelPosition + currentTransition.lastAnimation.joints[11].yawPhase))
											+ currentTransition.lastAnimation.joints[11].yawOffset;

					rollOscillationLast  = currentTransition.lastAnimation.joints[11].roll
											* Math.sin(degToRad( walkWheelPosition + currentTransition.lastAnimation.joints[11].rollPhase))

					rollOffset = currentAnimation.joints[11].rollOffset;
					rollOffsetLast = currentTransition.lastAnimation.joints[11].rollOffset;

				} else {

					pitchOscillation = currentAnimation.joints[11].pitch
										* Math.sin( degToRad(( cycle * adjustedFrequency )
										+ currentAnimation.joints[11].pitchPhase))
										+ currentAnimation.joints[11].pitchOffset;

					yawOscillation   = currentAnimation.joints[11].yaw
										* Math.sin( degToRad(( cycle * adjustedFrequency )
										+ currentAnimation.joints[11].yawPhase))
										+ currentAnimation.joints[11].yawOffset;

					rollOscillation  = currentAnimation.joints[11].roll
										* Math.sin( degToRad(( cycle * adjustedFrequency )
										+ currentAnimation.joints[11].rollPhase));

					rollOffset = currentAnimation.joints[11].rollOffset;

					pitchOscillationLast = currentTransition.lastAnimation.joints[11].pitch
											* Math.sin(degToRad( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency
											+ currentTransition.lastAnimation.joints[11].pitchPhase))
											+ currentTransition.lastAnimation.joints[11].pitchOffset;

					yawOscillationLast   = currentTransition.lastAnimation.joints[11].yaw
											* Math.sin(degToRad( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency
											+ currentTransition.lastAnimation.joints[11].yawPhase))
											+ currentTransition.lastAnimation.joints[11].yawOffset;

					rollOscillationLast  = currentTransition.lastAnimation.joints[11].roll
											* Math.sin(degToRad( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency
											+ currentTransition.lastAnimation.joints[11].rollPhase))

					rollOffset = currentAnimation.joints[11].rollOffset;
					rollOffsetLast = currentTransition.lastAnimation.joints[11].rollOffset;
				}

				pitchOscillation = (transitionProgress * pitchOscillation) + ((1-transitionProgress) * pitchOscillationLast);
				yawOscillation   = (transitionProgress * yawOscillation)   + ((1-transitionProgress) * yawOscillationLast);
				rollOscillation  = (transitionProgress * rollOscillation)  + ((1-transitionProgress) * rollOscillationLast);

				rollOffset = (transitionProgress * rollOffset) + ((1-transitionProgress) * rollOffsetLast);

			} else {

				pitchOscillation = currentAnimation.joints[11].pitch * Math.sin(degToRad(( cycle * adjustedFrequency ) + currentAnimation.joints[11].pitchPhase)) + currentAnimation.joints[11].pitchOffset;
				yawOscillation   = currentAnimation.joints[11].yaw   * Math.sin(degToRad(( cycle * adjustedFrequency ) + currentAnimation.joints[11].yawPhase))   + currentAnimation.joints[11].yawOffset;
				rollOscillation  = currentAnimation.joints[11].roll  * Math.sin(degToRad(( cycle * adjustedFrequency ) + currentAnimation.joints[11].rollPhase));

				rollOffset = currentAnimation.joints[11].rollOffset;
			}

			// set the hand rotations
			MyAvatar.setJointData("RightHand", Quat.fromPitchYawRollDegrees( sideStepSign * pitchOscillation,  yawOscillation,  rollOscillation + rollOffset));
			MyAvatar.setJointData("LeftHand",  Quat.fromPitchYawRollDegrees( pitchOscillation, -yawOscillation,  rollOscillation - rollOffset));

		} // end if(!armsFree)

        // head (includes neck joint) - currently zeroed out in STANDING animation files by request
        if( currentTransition !== null ) {

			if(currentTransition.walkingAtStart) {

				pitchOscillation = 0.5 * currentAnimation.joints[12].pitch * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency * 2)
									+ currentAnimation.joints[12].pitchPhase)) + currentAnimation.joints[12].pitchOffset;

				yawOscillation   = 0.5 * currentAnimation.joints[12].yaw   * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency    )
									+ currentAnimation.joints[12].yawPhase))   + currentAnimation.joints[12].yawOffset;

				rollOscillation  = 0.5 * currentAnimation.joints[12].roll  * Math.sin(degToRad(( cumulativeTime * currentAnimation.settings.baseFrequency    )
									+ currentAnimation.joints[12].rollPhase))  + currentAnimation.joints[12].rollOffset;

				pitchOscillationLast = 0.5 * currentTransition.lastAnimation.joints[12].pitch
										* Math.sin(degToRad(( walkWheelPosition * 2) + currentTransition.lastAnimation.joints[12].pitchPhase))
										+ currentTransition.lastAnimation.joints[12].pitchOffset;

				yawOscillationLast   = 0.5 * currentTransition.lastAnimation.joints[12].yaw
										* Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[12].yawPhase))
										+ currentTransition.lastAnimation.joints[12].yawOffset;

				rollOscillationLast  = 0.5 * currentTransition.lastAnimation.joints[12].roll
										* Math.sin(degToRad(( walkWheelPosition ) + currentTransition.lastAnimation.joints[12].rollPhase))
										+ currentTransition.lastAnimation.joints[12].rollOffset;

			} else {

				pitchOscillation = 0.5 * currentAnimation.joints[12].pitch * Math.sin(degToRad(( cycle * adjustedFrequency * 2) + currentAnimation.joints[12].pitchPhase)) + currentAnimation.joints[12].pitchOffset;
				yawOscillation   = 0.5 * currentAnimation.joints[12].yaw   * Math.sin(degToRad(( cycle * adjustedFrequency    ) + currentAnimation.joints[12].yawPhase))   + currentAnimation.joints[12].yawOffset;
				rollOscillation  = 0.5 * currentAnimation.joints[12].roll  * Math.sin(degToRad(( cycle * adjustedFrequency    ) + currentAnimation.joints[12].rollPhase))  + currentAnimation.joints[12].rollOffset;

				pitchOscillationLast = 0.5 * currentTransition.lastAnimation.joints[12].pitch
										* Math.sin(degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency  * 2)
										+ currentTransition.lastAnimation.joints[12].pitchPhase))
										+ currentTransition.lastAnimation.joints[12].pitchOffset;

				yawOscillationLast   = 0.5 * currentTransition.lastAnimation.joints[12].yaw
										* Math.sin(degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency  )
										+ currentTransition.lastAnimation.joints[12].yawPhase))
										+ currentTransition.lastAnimation.joints[12].yawOffset;

				rollOscillationLast  = 0.5 * currentTransition.lastAnimation.joints[12].roll
										* Math.sin(degToRad(( cumulativeTime * currentTransition.lastAnimation.settings.baseFrequency  )
										+ currentTransition.lastAnimation.joints[12].rollPhase))
										+ currentTransition.lastAnimation.joints[12].rollOffset;
			}

			pitchOscillation = (transitionProgress * pitchOscillation) + ((1-transitionProgress) * pitchOscillationLast);
			yawOscillation   = (transitionProgress * yawOscillation)   + ((1-transitionProgress) * yawOscillationLast);
			rollOscillation  = (transitionProgress * rollOscillation)  + ((1-transitionProgress) * rollOscillationLast);

		} else {

			pitchOscillation = 0.5 * currentAnimation.joints[12].pitch * Math.sin(degToRad(( cycle * adjustedFrequency * 2) + currentAnimation.joints[12].pitchPhase)) + currentAnimation.joints[12].pitchOffset;
			yawOscillation   = 0.5 * currentAnimation.joints[12].yaw   * Math.sin(degToRad(( cycle * adjustedFrequency    ) + currentAnimation.joints[12].yawPhase))   + currentAnimation.joints[12].yawOffset;
			rollOscillation  = 0.5 * currentAnimation.joints[12].roll  * Math.sin(degToRad(( cycle * adjustedFrequency    ) + currentAnimation.joints[12].rollPhase))  + currentAnimation.joints[12].rollOffset;
		}

		MyAvatar.setJointData( "Head", Quat.fromPitchYawRollDegrees( pitchOscillation, yawOscillation, rollOscillation ));
		MyAvatar.setJointData( "Neck", Quat.fromPitchYawRollDegrees( pitchOscillation, yawOscillation, rollOscillation ));
}



// Bezier function for applying to transitions - src: Dan Pupius (www.pupius.net) http://13thparallel.com/archive/bezier-curves/
Coord = function (x,y) {
	if(!x) var x=0;
	if(!y) var y=0;
	return {x: x, y: y};
}

function B1(t) { return t*t*t }
function B2(t) { return 3*t*t*(1-t) }
function B3(t) { return 3*t*(1-t)*(1-t) }
function B4(t) { return (1-t)*(1-t)*(1-t) }

function getBezier(percent,C1,C2,C3,C4) {
	var pos = new Coord();
	pos.x = C1.x*B1(percent) + C2.x*B2(percent) + C3.x*B3(percent) + C4.x*B4(percent);
	pos.y = C1.y*B1(percent) + C2.y*B2(percent) + C3.y*B3(percent) + C4.y*B4(percent);
	return pos;
}

// Butterworth LP filter - coeffs calculated using: http://www-users.cs.york.ac.uk/~fisher/mkfilter/trad.html
var NZEROS = 8;
var NPOLES = 8;
var GAIN =   17.40692157;
var xv = [0,0,0,0,0,0,0,0,0]; //xv.length = NZEROS+1;
var yv = [0,0,0,0,0,0,0,0,0]; //yv.length = NPOLES+1;

function filterButterworth(nextInputValue)
{
	xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8];
	xv[8] = nextInputValue / GAIN;
	yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8];
	yv[8] =   (xv[0] + xv[8]) + 8 * (xv[1] + xv[7]) + 28 * (xv[2] + xv[6])
				 + 56 * (xv[3] + xv[5]) + 70 * xv[4]
				 + ( -0.0033008230 * yv[0]) + ( -0.0440409341 * yv[1])
				 + ( -0.2663485333 * yv[2]) + ( -0.9570250765 * yv[3])
				 + ( -2.2596729000 * yv[4]) + ( -3.6088345059 * yv[5])
				 + ( -3.9148571397 * yv[6]) + ( -2.6527135283 * yv[7]);
	return yv[8];
}

// the faster we go, the further we lean forward. the angle is calcualted here
var leanAngles = []; // smooth out and add damping with simple averaging filter.
leanAngles.length = 15;

function getLeanPitch(velocity) {

	if(velocity>TERMINAL_VELOCITY) velocity=TERMINAL_VELOCITY;
	var leanProgress = velocity / TERMINAL_VELOCITY;
	var responseSharpness = 1.8;
	if(principleDirection==DIRECTION_BACKWARDS) responseSharpness = 3.6; // lean back a bit extra when walking backwards
	var leanProgressBezier = getBezier((1-leanProgress),{x:0,y:0},{x:0,y:responseSharpness},{x:0,y:1},{x:1,y:1}).y;

	// simple averaging filter seems to give best results
    leanAngles.push(leanProgressBezier);
    leanAngles.shift(); // FIFO
    var totalLeanAngles = 0;
    for(ea in leanAngles) totalLeanAngles += leanAngles[ea];
    var leanProgressAverageFiltered = totalLeanAngles / leanAngles.length;

	// calculate final return value
	var leanPitchFinal = 0;
	if(principleDirection===DIRECTION_BACKWARDS) {
		leanPitchFinal = -currentAnimation.settings.flyingHipsPitch * leanProgressAverageFiltered;// + finalAccelerationResponse;
	} else {
		leanPitchFinal = currentAnimation.settings.flyingHipsPitch * leanProgressAverageFiltered;// + finalAccelerationResponse;
	}
    return leanPitchFinal;
}

// calculate the angle at which to bank into corners when turning
var leanRollAngles = [];  // smooth out and add damping with simple averaging filter
leanRollAngles.length = 25;

var angularVelocities = []; // keep a record of the last few so can filter out spurious values
angularVelocities.length = 5;

function getLeanRoll(deltaTime, velocity) {

	// what's our current anglular velocity?
	var angularVelocityMax = 70; // from observation
	var currentOrientationVec3 = Quat.safeEulerAngles(MyAvatar.orientation);
    var lastOrientationVec3 = Quat.safeEulerAngles(lastOrientation);
    var deltaYaw = lastOrientationVec3.y-currentOrientationVec3.y;
    lastOrientation = MyAvatar.orientation;

    var angularVelocity = deltaYaw / deltaTime;
    if(angularVelocity>angularVelocityMax) angularVelocity = angularVelocityMax;
    if(angularVelocity<-angularVelocityMax) angularVelocity = -angularVelocityMax;

    // filter the angular velocity for a nicer response and a bit of wobble (intentional overshoot / ringing)
    angularVelocity = filterButterworth(angularVelocity);

    var turnSign = 1;
    if(angularVelocity<0) turnSign = -1;
    if(principleDirection===DIRECTION_BACKWARDS)
    	turnSign *= -1;

    // calculate the amount of roll based on both angular and linear velocities
	if(velocity>TERMINAL_VELOCITY) velocity = TERMINAL_VELOCITY;
	var leanRollProgress = (velocity / TERMINAL_VELOCITY) * (Math.abs(angularVelocity) / angularVelocityMax);

	// apply our response curve
	var leanRollProgressBezier = getBezier((1-leanRollProgress),{x:0,y:0},{x:0,y:2.5},{x:0,y:1},{x:1,y:1}).y;

	// simple averaging filter
    leanRollAngles.push(turnSign * leanRollProgressBezier);
    leanRollAngles.shift(); // FIFO
    var totalLeanRollAngles = 0;
    for(var ea in leanRollAngles) totalLeanRollAngles += leanRollAngles[ea];
    var leanRollProgressAverageFiltered = totalLeanRollAngles / leanRollAngles.length;

    return currentAnimation.settings.maxBankingAngle * leanRollProgressAverageFiltered;
}

// set up the interface components, update the internal state and kick off any transitions
function setInternalState(newInternalState) {

	switch(newInternalState) {

        case WALKING:

            if(!minimised) doStandardMenu();
            INTERNAL_STATE = WALKING;
            return;

        case FLYING:

            if(!minimised) doStandardMenu();
            INTERNAL_STATE = FLYING;
            return;

        case SIDE_STEPPING:

            if(!minimised) doStandardMenu();
            INTERNAL_STATE = SIDE_STEPPING;
            return;

        case CONFIG_WALK_STYLES:

            INTERNAL_STATE = CONFIG_WALK_STYLES;
            currentAnimation = selectedWalk;
            if(!minimised) {
				hidebuttonOverlays();
				hideJointControls();
				showFrontPanelButtons(false);
				showWalkStyleButtons(true);
				setBackground(controlsBackgroundWalkEditStyles);
				setButtonOverlayVisible(onButton);
				setButtonOverlayVisible(configWalkStylesButtonSelected);
				setButtonOverlayVisible(configWalkTweaksButton);
				setButtonOverlayVisible(configWalkJointsButton);
				setButtonOverlayVisible(backButton);
				setSliderThumbsVisible(false);
			}
            return;

        case CONFIG_WALK_TWEAKS:

            INTERNAL_STATE = CONFIG_WALK_TWEAKS;
            currentAnimation = selectedWalk;
            if(!minimised) {
				hidebuttonOverlays();
				hideJointControls();
				showFrontPanelButtons(false);
				showWalkStyleButtons(false);
				setBackground(controlsBackgroundWalkEditTweaks);
				setButtonOverlayVisible(onButton);
				setButtonOverlayVisible(configWalkStylesButton);
				setButtonOverlayVisible(configWalkTweaksButtonSelected);
				setButtonOverlayVisible(configWalkJointsButton);
				setButtonOverlayVisible(backButton);
				initialiseWalkTweaks();
			}
            return;

        case CONFIG_WALK_JOINTS:

        	INTERNAL_STATE = CONFIG_WALK_JOINTS;
        	currentAnimation = selectedWalk;
        	if(!minimised) {
				hidebuttonOverlays();
				showFrontPanelButtons(false);
				showWalkStyleButtons(false);
				setBackground(controlsBackgroundWalkEditJoints);
				setButtonOverlayVisible(onButton);
				setButtonOverlayVisible(configWalkStylesButton);
				setButtonOverlayVisible(configWalkTweaksButton);
				setButtonOverlayVisible(configWalkJointsButtonSelected);
				setButtonOverlayVisible(backButton);
				initialiseJointsEditingPanel(selectedJointIndex);
			}
            return;

        case CONFIG_STANDING:

            INTERNAL_STATE = CONFIG_STANDING;
            currentAnimation = selectedStand;
            if(!minimised) {
				hidebuttonOverlays();
				showFrontPanelButtons(false);
				showWalkStyleButtons(false);
				setBackground(controlsBackgroundWalkEditJoints);
				setButtonOverlayVisible(onButton);
				setButtonOverlayVisible(configSideStepRightButton);
				setButtonOverlayVisible(configSideStepLeftButton);
				setButtonOverlayVisible(configStandButtonSelected);
				setButtonOverlayVisible(backButton);
				initialiseJointsEditingPanel(selectedJointIndex);
			}
            return;

        case CONFIG_SIDESTEP_LEFT:

            INTERNAL_STATE = CONFIG_SIDESTEP_LEFT;
            currentAnimation = selectedSideStepLeft;
            if(!minimised) {
				hidebuttonOverlays();
				showFrontPanelButtons(false);
				showWalkStyleButtons(false);
				setBackground(controlsBackgroundWalkEditJoints);
				setButtonOverlayVisible(onButton);
				setButtonOverlayVisible(configSideStepRightButton);
				setButtonOverlayVisible(configSideStepLeftButtonSelected);
				setButtonOverlayVisible(configStandButton);
				setButtonOverlayVisible(backButton);
				initialiseJointsEditingPanel(selectedJointIndex);
			}
            return;

        case CONFIG_SIDESTEP_RIGHT:

            INTERNAL_STATE = CONFIG_SIDESTEP_RIGHT;
            currentAnimation = selectedSideStepRight;
            if(!minimised) {
				hidebuttonOverlays();
				showFrontPanelButtons(false);
				showWalkStyleButtons(false);
				setBackground(controlsBackgroundWalkEditJoints);
				setButtonOverlayVisible(onButton);
				setButtonOverlayVisible(configSideStepRightButtonSelected);
				setButtonOverlayVisible(configSideStepLeftButton);
				setButtonOverlayVisible(configStandButton);
				setButtonOverlayVisible(backButton);
				initialiseJointsEditingPanel(selectedJointIndex);
			}
            return;

        case CONFIG_FLYING:

            INTERNAL_STATE = CONFIG_FLYING;
            currentAnimation = selectedFly;
            if(!minimised) {
				hidebuttonOverlays();
				showFrontPanelButtons(false);
				showWalkStyleButtons(false);
				setBackground(controlsBackgroundWalkEditJoints);
				setButtonOverlayVisible(onButton);
				setButtonOverlayVisible(configFlyingUpButton);
				setButtonOverlayVisible(configFlyingDownButton);
				setButtonOverlayVisible(configFlyingButtonSelected);
				setButtonOverlayVisible(backButton);
				initialiseJointsEditingPanel(selectedJointIndex);
			}
            return;

        case CONFIG_FLYING_UP:

            INTERNAL_STATE = CONFIG_FLYING_UP;
            currentAnimation = selectedFlyUp;
            if(!minimised) {
				hidebuttonOverlays();
				showFrontPanelButtons(false);
				showWalkStyleButtons(false);
				setBackground(controlsBackgroundWalkEditJoints);
				setButtonOverlayVisible(onButton);
				setButtonOverlayVisible(configFlyingUpButtonSelected);
				setButtonOverlayVisible(configFlyingDownButton);
				setButtonOverlayVisible(configFlyingButton);
				setButtonOverlayVisible(backButton);
				initialiseJointsEditingPanel(selectedJointIndex);
			}
            return;

        case CONFIG_FLYING_DOWN:

            INTERNAL_STATE = CONFIG_FLYING_DOWN;
            currentAnimation = selectedFlyDown;
            if(!minimised) {
				hidebuttonOverlays();
				showFrontPanelButtons(false);
				showWalkStyleButtons(false);
				setBackground(controlsBackgroundWalkEditJoints);
				setButtonOverlayVisible(onButton);
				setButtonOverlayVisible(configFlyingUpButton);
				setButtonOverlayVisible(configFlyingDownButtonSelected);
				setButtonOverlayVisible(configFlyingButton);
				setButtonOverlayVisible(backButton);
				initialiseJointsEditingPanel(selectedJointIndex);
			}
            return;

        case STANDING:
        default:

            INTERNAL_STATE = STANDING;
            if(!minimised) doStandardMenu();

            // initialisation - runs at script startup only
            if(strideLength===0) {

				if(principleDirection===DIRECTION_BACKWARDS)
					strideLength = selectedWalk.calibration.strideLengthBackwards;
				else
            		strideLength = selectedWalk.calibration.strideLengthForwards;

				curlFingers();
			}
            return;
    }
}

// Main loop

// stabilising vars -  most state changes are preceded by a couple of hints that they are about to happen rather than
// momentarilly switching between states (causes flicker), we count the number of hints in a row before actually changing state
var standHints = 0;
var walkHints = 0;
var flyHints = 0;
var requiredHints = 2; // tweakable - debounce state changes - how many times do we get a state change request in a row before we actually change state? (used to be 4 or 5)
var principleDirection = 0;

// helper function for stats output
function directionAsString(directionEnum) {

	switch(directionEnum) {
		case DIRECTION_UP: return 'Up';
		case DIRECTION_DOWN: return 'Down';
		case DIRECTION_LEFT: return 'Left';
		case DIRECTION_RIGHT: return 'Right';
		case DIRECTION_FORWARDS: return 'Forwards';
		case DIRECTION_BACKWARDS: return 'Backwards';
		default: return 'Unknown';
	}
}
// helper function for stats output
function internalStateAsString(internalState) {

	switch(internalState) {
		case STANDING: return 'Standing';
		case WALKING: return 'Walking';
		case SIDE_STEPPING: return 'Side Stepping';
		case FLYING: return 'Flying';
		default: return 'Editing';
	}
}

Script.update.connect(function(deltaTime) {

	if(powerOn) {

		frameStartTime = new Date().getTime();
        cumulativeTime += deltaTime;
        nFrames++;
        var speed = 0;

        // firstly check for editing modes, as these require no positioning calculations
        var editing = false;
        switch(INTERNAL_STATE) {

            case CONFIG_WALK_STYLES:
	            currentAnimation = selectedWalk;
                animateAvatar(deltaTime, speed, principleDirection);
                editing = true;
                break;
            case CONFIG_WALK_TWEAKS:
                currentAnimation = selectedWalk;
                animateAvatar(deltaTime, speed, DIRECTION_FORWARDS);
                editing = true;
                break;
            case CONFIG_WALK_JOINTS:
                currentAnimation = selectedWalk;
                animateAvatar(deltaTime, speed, DIRECTION_FORWARDS);
                editing = true;
                break;
            case CONFIG_STANDING:
                currentAnimation = selectedStand;
                animateAvatar(deltaTime, speed, DIRECTION_FORWARDS);
                editing = true;
                break;
            case CONFIG_SIDESTEP_LEFT:
                currentAnimation = selectedSideStepLeft;
                animateAvatar(deltaTime, speed, DIRECTION_LEFT);
                editing = true;
                break;
            case CONFIG_SIDESTEP_RIGHT:
                currentAnimation = selectedSideStepRight;
                animateAvatar(deltaTime, speed, DIRECTION_RIGHT);
                editing = true;
                break;
            case CONFIG_FLYING:
                currentAnimation = selectedFly;
                animateAvatar(deltaTime, speed, DIRECTION_FORWARDS);
                editing = true;
                break;
            case CONFIG_FLYING_UP:
                currentAnimation = selectedFlyUp;
                animateAvatar(deltaTime, speed, DIRECTION_UP);
                editing = true;
                break;
            case CONFIG_FLYING_DOWN:
                currentAnimation = selectedFlyDown;
                animateAvatar(deltaTime, speed, DIRECTION_DOWN);
                editing = true;
                break;
            default:
                break;
        }

		// we have to declare these vars here ( outside 'if(!editing)' ), so they are still in scope
		// when we record the frame's data and when we do the stats update at the end
		var deltaX = 0;
		var deltaY = 0;
		var deltaZ = 0;
		var acceleration = { x:0, y:0, z:0 };
		var accelerationJS = MyAvatar.getAcceleration();

		// calculate overriding (local) direction of translation for use later when decide which animation should be played
		var inverseRotation = Quat.inverse(MyAvatar.orientation);
		var localVelocity = Vec3.multiplyQbyV(inverseRotation, MyAvatar.getVelocity());

		if(!editing) {

			// the first thing to do is find out how fast we're going,
			// what our acceleration is and which direction we're principly moving in

			// calcualte (local) change in velocity
			var velocity = MyAvatar.getVelocity();
			speed = Vec3.length(velocity);

			// determine the candidate animation to play
			var actionToTake = 0;
			if( speed < 0.5) {
				actionToTake = STANDING;
				standHints++;
			}
			else if( speed < FLYING_SPEED ) {
				actionToTake = WALKING;
				walkHints++;
			}
			else if( speed >= FLYING_SPEED ) {
				actionToTake = FLYING;
				flyHints++;
			}

			deltaX = localVelocity.x;
			deltaY = localVelocity.y;
			deltaZ = -localVelocity.z;

			// determine the principle direction
			if(Math.abs(deltaX)>Math.abs(deltaY)
			 &&Math.abs(deltaX)>Math.abs(deltaZ)) {
				if(deltaX<0) {
					principleDirection = DIRECTION_RIGHT;
				} else {
					principleDirection = DIRECTION_LEFT;
				}
			}
			else if(Math.abs(deltaY)>Math.abs(deltaX)
				  &&Math.abs(deltaY)>Math.abs(deltaZ)) {
				if(deltaY>0) {
					principleDirection = DIRECTION_UP;
				}
				else {
					principleDirection = DIRECTION_DOWN;
				}
			}
			else if(Math.abs(deltaZ)>Math.abs(deltaX)
				  &&Math.abs(deltaZ)>Math.abs(deltaY)) {
				if(deltaZ>0) {
					principleDirection = DIRECTION_FORWARDS;
				} else {
					principleDirection = DIRECTION_BACKWARDS;
				}
			}

			// NB: this section will change significantly once we are ground plane aware
			//     it will change even more once we have uneven surfaces to deal with

			// maybe at walking speed, but sideways?
			if( actionToTake === WALKING &&
			  ( principleDirection === DIRECTION_LEFT ||
				principleDirection === DIRECTION_RIGHT )) {

					actionToTake = SIDE_STEPPING;
			}

			// maybe at walking speed, but flying up?
			if( actionToTake === WALKING &&
			  ( principleDirection === DIRECTION_UP )) {

					actionToTake = FLYING;
					standHints--;
					flyHints++;
			}

			// maybe at walking speed, but flying down?
			if( actionToTake === WALKING &&
			  ( principleDirection === DIRECTION_DOWN )) {

					actionToTake = FLYING;
					standHints--;
					flyHints++;
			}

			// log this frame's motion for later reference
			var accelerationX = ( framesHistory.recentMotions[0].velocity.x - localVelocity.x ) / deltaTime;
			var accelerationY = ( localVelocity.y - framesHistory.recentMotions[0].velocity.y ) / deltaTime;
			var accelerationZ = ( framesHistory.recentMotions[0].velocity.z - localVelocity.z ) / deltaTime;
			acceleration = {x:accelerationX, y:accelerationY, z:accelerationZ};


			// select appropriate animation and initiate Transition if required
			switch(actionToTake) {

				case STANDING:

					if( standHints > requiredHints || INTERNAL_STATE===STANDING) { // wait for a few consecutive hints (17mS each)

						standHints = 0;
						walkHints = 0;
						flyHints = 0;

						// do we need to change state?
						if( INTERNAL_STATE!==STANDING ) {

							// initiate the transition
							if(currentTransition) {
								delete currentTransition;
								currentTransition = null;
							}

							switch(currentAnimation) {

								case selectedWalk:

									// Walking to Standing
									var timeWalking = new Date().getTime() - framesHistory.lastWalkStartTime;

									var bezierCoeffsOne = {x:0.0, y:1.0};
									var bezierCoeffsTwo = {x:0.0, y:1.0};
									var transitionTime = 0.4;

									// very different curves for incremental steps
									if( timeWalking < 550 ) {
										bezierCoeffsOne = {x:0.63, y:0.17};
										bezierCoeffsTwo = {x:0.77, y:0.3};
										transitionTime = 0.75;
									}
									currentTransition = new Transition( currentAnimation, selectedStand, [], transitionTime, bezierCoeffsOne, bezierCoeffsTwo );
									break;


								case selectedSideStepLeft:
								case selectedSideStepRight:

									break;

								default:

									currentTransition = new Transition(currentAnimation, selectedStand, [], 0.3, {x:0.5,y:0.08}, {x:0.28,y:1});
									break;
							}

							setInternalState(STANDING);
							currentAnimation = selectedStand;

						}
						animateAvatar(1,0,principleDirection);
					}
					break;

				case WALKING:
				case SIDE_STEPPING:
					if( walkHints > requiredHints ||
						INTERNAL_STATE===WALKING ||
						INTERNAL_STATE===SIDE_STEPPING ) { // wait for few consecutive hints (17mS each)

						standHints = 0;
						walkHints = 0;
						flyHints = 0;

						if( actionToTake === WALKING && INTERNAL_STATE !== WALKING) {

							// initiate the transition
							if(currentTransition) {
								delete currentTransition;
								currentTransition = null;
							}

							// set the appropriate start position for the walk wheel
							if( principleDirection === DIRECTION_BACKWARDS ) {

								walkWheelPosition = selectedWalk.settings.startAngleBackwards;

							} else {

								walkWheelPosition = selectedWalk.settings.startAngleForwards;
							}

							switch(currentAnimation) {

								case selectedStand:

									// Standing to Walking
									currentTransition = new Transition(currentAnimation, selectedWalk, [], 0.25, {x:0.5,y:0.08}, {x:0.05,y:0.75});
									break;

								case selectedSideStepLeft:
								case selectedSideStepRight:

									break;

								default:

									currentTransition = new Transition(currentAnimation, selectedWalk, [], 0.3, {x:0.5,y:0.08}, {x:0.05,y:0.75});
									break;
							}
							framesHistory.lastWalkStartTime = new Date().getTime();
							setInternalState(WALKING);
							currentAnimation = selectedWalk;
						}
						else if(actionToTake===SIDE_STEPPING) {

							var selectedSideStep = selectedSideStepRight;
							if( principleDirection === DIRECTION_LEFT ) {

								selectedSideStep = selectedSideStepLeft;

							} else {

								selectedSideStep = selectedSideStepRight;
							}

							if( INTERNAL_STATE !== SIDE_STEPPING ) {

								if( principleDirection === DIRECTION_LEFT ) {

									walkWheelPosition = sideStepCycleStartLeft;

								} else {

									walkWheelPosition = sideStepCycleStartRight;
								}
								switch(currentAnimation) {

									case selectedStand:

										break;

									default:

										break;
								}
								setInternalState(SIDE_STEPPING);
							}

							currentAnimation = selectedSideStep;
						}
						animateAvatar( deltaTime, speed, principleDirection );
					}
					break;

				case FLYING:

					if( flyHints > requiredHints - 1  || INTERNAL_STATE===FLYING ) { // wait for a few consecutive hints (17mS each)

						standHints = 0;
						walkHints = 0;
						flyHints = 0;

						if(INTERNAL_STATE!==FLYING) setInternalState(FLYING);

						// change animation for flying directly up or down. TODO - check RecentMotions, if is a change then put a transition on it
						if(principleDirection===DIRECTION_UP) {

							if(currentAnimation !== selectedFlyUp) {

								// initiate a Transition
								if(currentTransition && currentTransition.nextAnimation!==selectedFlyUp) {
									delete currentTransition;
									currentTransition = null;
								}
								switch(currentAnimation) {

									case selectedStand:

										currentTransition = new Transition(currentAnimation, selectedFlyUp, [], 0.35, {x:0.5,y:0.08}, {x:0.28,y:1});
										break;

									case selectedSideStepLeft:
									case selectedSideStepRight:

										break;

									default:

										currentTransition = new Transition(currentAnimation, selectedFlyUp, [], 0.35, {x:0.5,y:0.08}, {x:0.28,y:1});
										break;
								}
								currentAnimation = selectedFlyUp;
							}

						} else if(principleDirection==DIRECTION_DOWN) {

							if(currentAnimation !== selectedFlyDown) { // TODO: as the locomotion gets cleaner (i.e. less false reports from Interface) this value can be reduced

								// initiate a Transition
								if(currentTransition && currentTransition.nextAnimation!==selectedFlyDown) {
									delete currentTransition;
									currentTransition = null;
								}
								switch(currentAnimation) {

									case selectedStand:

										currentTransition = new Transition(currentAnimation, selectedFlyDown, [], 0.35, {x:0.5,y:0.08}, {x:0.28,y:1});
										break;

									case selectedSideStepLeft:
									case selectedSideStepRight:

										break;

									default:

										currentTransition = new Transition(currentAnimation, selectedFlyDown, [], 0.35, {x:0.5,y:0.08}, {x:0.28,y:1});
										break;
								}
								currentAnimation = selectedFlyDown;
							}

						} else {

							if(currentAnimation !== selectedFly) { // TODO: as the locomotion gets cleaner (i.e. less false reports from Interface) this value can be reduced

								// initiate a Transition
								if(currentTransition && currentTransition.nextAnimation!==selectedFly) {
									delete currentTransition;
									currentTransition = null;
								}
								switch(currentAnimation) {

									case selectedStand:

										currentTransition = new Transition(currentAnimation, selectedFly, [], 0.35, {x:0.5,y:0.08}, {x:0.28,y:1});
										break;

									case selectedSideStepLeft:
									case selectedSideStepRight:

										break;

									default:

										currentTransition = new Transition(currentAnimation, selectedFly, [], 0.35, {x:0.5,y:0.08}, {x:0.28,y:1});
										break;
								}
								currentAnimation = selectedFly;
							}
						}
						animateAvatar(deltaTime, speed, principleDirection);
					}
					break;

			} // end switch(actionToTake)

		} // end if(!editing)


		// record the frame's stats for later reference
        var thisMotion = new RecentMotion(localVelocity, acceleration, principleDirection, INTERNAL_STATE);
		framesHistory.recentMotions.push(thisMotion);
		framesHistory.recentMotions.shift();


		// before we go, populate the stats overlay
		if( statsOn ) {

			var cumulativeTimeMS = Math.floor(cumulativeTime*1000);
			var deltaTimeMS = deltaTime * 1000;
			var frameExecutionTime = new Date().getTime() - frameStartTime;
			if(frameExecutionTime>frameExecutionTimeMax) frameExecutionTimeMax = frameExecutionTime;

			var angluarVelocity = Vec3.length(MyAvatar.getAngularVelocity());

			var debugInfo = '\n \n \n \n                   Stats\n--------------------------------------\n \n \n'
						  + '\nFrame number: '+nFrames
						  + '\nFrame time: '+deltaTimeMS.toFixed(2)
						  + ' mS\nRender time: '+frameExecutionTime.toFixed(0)
						  + ' mS\nLocalised speed: '+speed.toFixed(3)
						  + ' m/s\nCumulative Time '+cumulativeTimeMS.toFixed(0)
						  + ' mS\nState: '+internalStateAsString(INTERNAL_STATE)
						  + ' \nDirection: '+directionAsString(principleDirection)
						  + ' \nAngular Velocity: ' + angluarVelocity.toFixed(3)
						  + ' rad/s';
			Overlays.editOverlay(debugStats, {text: debugInfo});

			// update these every 250 mS (assuming 60 fps)
			if( nFrames % 15 === 0 ) {
				var debugInfo = '           Periodic Stats\n--------------------------------------\n \n \n'
							  + ' \n \nRender time peak hold: '+frameExecutionTimeMax.toFixed(0)
							  + ' mS\n \n \n(L) MyAvatar.getVelocity()'
							  + ' \n \nlocalVelocityX: '+deltaX.toFixed(1)
							  + ' m/s\nlocalVelocityY: '+deltaY.toFixed(1)
							  + ' m/s\nlocalVelocityZ: '+deltaZ.toFixed(1)
							  + ' m/s\n \n(G) MyAvatar.getAcceleration()'
							  + ' \n\nAcceleration X: '+accelerationJS.x.toFixed(1)
							  + ' m/s/s\nAcceleration Y: '+accelerationJS.y.toFixed(1)
							  + ' m/s/s\nAcceleration Z: '+accelerationJS.z.toFixed(1)
							  + ' m/s/s\n \n(L) Acceleration using\nMyAvatar.getVelocity()'
							  + ' \n \nAcceleration X: '+acceleration.x.toFixed(1)
							  + ' m/s/s\nAcceleration Y: '+acceleration.y.toFixed(1)
							  + ' m/s/s\nAcceleration Z: '+acceleration.z.toFixed(1)
						      + ' m/s/s';
				Overlays.editOverlay(debugStatsPeriodic, {text: debugInfo});
				frameExecutionTimeMax = 0;
			}
		}
    }
});


// overlays start

//  controller dimensions
var backgroundWidth = 350;
var backgroundHeight = 700;
var backgroundX = Window.innerWidth-backgroundWidth-58;
var backgroundY = Window.innerHeight/2 - backgroundHeight/2;
var minSliderX = backgroundX + 30;
var maxSliderX = backgroundX + 295;
var sliderRangeX = 295 - 30;
var jointsControlWidth = 200;
var jointsControlHeight = 300;
var jointsControlX = backgroundX + backgroundWidth/2 - jointsControlWidth/2;
var jointsControlY = backgroundY + 242 - jointsControlHeight/2;
var buttonsY = 20;  // distance from top of panel to buttons

// arrays of overlay names
var sliderThumbOverlays = []; // thumb sliders
var backgroundOverlays = [];
var buttonOverlays = [];
var jointsControlOverlays = [];
var bigButtonOverlays = [];


// load UI backgrounds
var controlsBackground = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX, y: backgroundY, width: backgroundWidth, height: backgroundHeight },
                    imageURL: pathToOverlays+"ddao-background.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
backgroundOverlays.push(controlsBackground);

var controlsBackgroundWalkEditStyles = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX, y: backgroundY, width: backgroundWidth, height: backgroundHeight },
                    imageURL: pathToOverlays+"ddao-background-edit-styles.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
backgroundOverlays.push(controlsBackgroundWalkEditStyles);

var controlsBackgroundWalkEditTweaks = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX, y: backgroundY, width: backgroundWidth, height: backgroundHeight },
                    imageURL: pathToOverlays+"ddao-background-edit-tweaks.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
backgroundOverlays.push(controlsBackgroundWalkEditTweaks);

var controlsBackgroundWalkEditJoints = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX, y: backgroundY, width: backgroundWidth, height: backgroundHeight },
                    imageURL: pathToOverlays+"ddao-background-edit-joints.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
backgroundOverlays.push(controlsBackgroundWalkEditJoints);

var controlsBackgroundFlyingEdit = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX, y: backgroundY, width: backgroundWidth, height: backgroundHeight },
                    imageURL: pathToOverlays+"ddao-background-flying-edit.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
backgroundOverlays.push(controlsBackgroundFlyingEdit);

// minimised tab - not put in array, as is a one off
var controlsMinimisedTab = Overlays.addOverlay("image", {
					x: Window.innerWidth-58, y: Window.innerHeight -145, width: 50, height: 50,
					//subImage: { x: 0, y: 50, width: 50, height: 50 },
					imageURL: pathToOverlays + 'ddao-minimise-tab.png',
					visible: minimised,
					alpha: 0.9
					});

// load character joint selection control images
var hipsJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 200, height: 300},
                    imageURL: pathToOverlays+"ddao-background-edit-hips.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
jointsControlOverlays.push(hipsJointControl);

var upperLegsJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 200, height: 300},
                    imageURL: pathToOverlays+"ddao-background-edit-upper-legs.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
jointsControlOverlays.push(upperLegsJointControl);

var lowerLegsJointControl  = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 200, height: 300},
                    imageURL: pathToOverlays+"ddao-background-edit-lower-legs.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
jointsControlOverlays.push(lowerLegsJointControl);

var feetJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 200, height: 300},
                    imageURL: pathToOverlays+"ddao-background-edit-feet.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
jointsControlOverlays.push(feetJointControl);

var toesJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 200, height: 300},
                    imageURL: pathToOverlays+"ddao-background-edit-toes.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
jointsControlOverlays.push(toesJointControl);

var spineJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 200, height: 300},
                    imageURL: pathToOverlays+"ddao-background-edit-spine.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
jointsControlOverlays.push(spineJointControl);

var spine1JointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 200, height: 300},
                    imageURL: pathToOverlays+"ddao-background-edit-spine1.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
jointsControlOverlays.push(spine1JointControl);

var spine2JointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 200, height: 300},
                    imageURL: pathToOverlays+"ddao-background-edit-spine2.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
jointsControlOverlays.push(spine2JointControl);

var shouldersJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 200, height: 300},
                    imageURL: pathToOverlays+"ddao-background-edit-shoulders.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
jointsControlOverlays.push(shouldersJointControl);

var upperArmsJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 200, height: 300},
                    imageURL: pathToOverlays+"ddao-background-edit-upper-arms.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
jointsControlOverlays.push(upperArmsJointControl);

var forearmsJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 200, height: 300},
                    imageURL: pathToOverlays+"ddao-background-edit-forearms.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
jointsControlOverlays.push(forearmsJointControl);

var handsJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 200, height: 300},
                    imageURL: pathToOverlays+"ddao-background-edit-hands.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
jointsControlOverlays.push(handsJointControl);

var headJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 200, height: 300},
                    imageURL: pathToOverlays+"ddao-background-edit-head.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
jointsControlOverlays.push(headJointControl);


// sider thumb overlays
var sliderOne = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 25, height: 25 },
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
sliderThumbOverlays.push(sliderOne);
var sliderTwo = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 25, height: 25 },
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
sliderThumbOverlays.push(sliderTwo);
var sliderThree = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 25, height: 25 },
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
sliderThumbOverlays.push(sliderThree);
var sliderFour = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 25, height: 25 },
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
sliderThumbOverlays.push(sliderFour);
var sliderFive = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 25, height: 25 },
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
sliderThumbOverlays.push(sliderFive);
var sliderSix = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 25, height: 25 },
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
sliderThumbOverlays.push(sliderSix);
var sliderSeven = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 25, height: 25 },
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
sliderThumbOverlays.push(sliderSeven);
var sliderEight = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 25, height: 25 },
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
sliderThumbOverlays.push(sliderEight);
var sliderNine = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 25, height: 25 },
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
sliderThumbOverlays.push(sliderNine);


// button overlays
var onButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+20, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-on-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(onButton);
var offButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+20, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-off-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(offButton);
var configWalkButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+83, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-edit-walk-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configWalkButton);
var configWalkButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+83, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-edit-walk-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configWalkButtonSelected);
var configStandButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+146, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-edit-stand-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configStandButton);
var configStandButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+146, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-edit-stand-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configStandButtonSelected);

var configFlyingButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+209, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-edit-fly-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configFlyingButton);
var configFlyingButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+209, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-edit-fly-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configFlyingButtonSelected);

var configFlyingUpButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+83, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-edit-fly-up-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configFlyingUpButton);
var configFlyingUpButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+83, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-edit-fly-up-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configFlyingUpButtonSelected);

var configFlyingDownButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+146, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-edit-fly-down-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configFlyingDownButton);
var configFlyingDownButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+146, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-edit-fly-down-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configFlyingDownButtonSelected);

var hideButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+272, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-hide-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(hideButton);
var hideButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+272, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-hide-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(hideButtonSelected);
var configWalkStylesButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+83, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-walk-styles-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configWalkStylesButton);
var configWalkStylesButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+83, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-walk-styles-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configWalkStylesButtonSelected);
var configWalkTweaksButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+146, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-walk-tweaks-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configWalkTweaksButton);
var configWalkTweaksButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+146, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-walk-tweaks-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configWalkTweaksButtonSelected);

var configSideStepLeftButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+83, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-edit-sidestep-left-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configSideStepLeftButton);
var configSideStepLeftButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+83, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-edit-sidestep-left-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configSideStepLeftButtonSelected);

var configSideStepRightButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+209, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-edit-sidestep-right-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configSideStepRightButton);
var configSideStepRightButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+209, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-edit-sidestep-right-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configSideStepRightButtonSelected);

var configWalkJointsButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+209, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-bones-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configWalkJointsButton);
var configWalkJointsButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+209, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-bones-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(configWalkJointsButtonSelected);

var backButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+272, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-back-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(backButton);
var backButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+272, y: backgroundY+buttonsY, width: 60, height: 47 },
                    imageURL: pathToOverlays+"ddao-back-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
buttonOverlays.push(backButtonSelected);

// big button overlays - front panel
var bigButtonYOffset = 408;  //  distance from top of panel to top of first button

var femaleBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 230, height: 36},
                    imageURL: pathToOverlays+"ddao-female-big-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
bigButtonOverlays.push(femaleBigButton);

var femaleBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 230, height: 36},
                    imageURL: pathToOverlays+"ddao-female-big-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
bigButtonOverlays.push(femaleBigButtonSelected);

var maleBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset + 60, width: 230, height: 36},
                    imageURL: pathToOverlays+"ddao-male-big-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
bigButtonOverlays.push(maleBigButton);

var maleBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset + 60, width: 230, height: 36},
                    imageURL: pathToOverlays+"ddao-male-big-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
bigButtonOverlays.push(maleBigButtonSelected);

var armsFreeBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset + 120, width: 230, height: 36},
                    imageURL: pathToOverlays+"ddao-arms-free-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
bigButtonOverlays.push(armsFreeBigButton);

var armsFreeBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset + 120, width: 230, height: 36},
                    imageURL: pathToOverlays+"ddao-arms-free-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
bigButtonOverlays.push(armsFreeBigButtonSelected);

var footstepsBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset + 180, width: 230, height: 36},
                    imageURL: pathToOverlays+"ddao-footsteps-big-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
bigButtonOverlays.push(footstepsBigButton);

var footstepsBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset + 180, width: 230, height: 36},
                    imageURL: pathToOverlays+"ddao-footsteps-big-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
bigButtonOverlays.push(footstepsBigButtonSelected);


// walk styles
bigButtonYOffset = 121;
var strutWalkBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 230, height: 36 },
                    imageURL: pathToOverlays+"ddao-walk-select-button-strut.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
bigButtonOverlays.push(strutWalkBigButton);

var strutWalkBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 230, height: 36 },
                    imageURL: pathToOverlays+"ddao-walk-select-button-strut-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
bigButtonOverlays.push(strutWalkBigButtonSelected);

// overlays to show the walk wheel stats
var walkWheelZLine = Overlays.addOverlay("line3d", {
                    position: { x: 0, y: 0, z:hipsToFeetDistance },
                    end: { x: 0, y: 0, z: -hipsToFeetDistance },
                    color: { red: 0, green: 255, blue: 255},
                    alpha: 1,
                    lineWidth: 5,
                    visible: statsOn,
                    anchor: "MyAvatar"
                });
var walkWheelYLine = Overlays.addOverlay("line3d", {
                    position: { x: 0, y: hipsToFeetDistance, z:0 },
                    end: { x: 0, y: -hipsToFeetDistance, z:0 },
                    color: { red: 255, green: 0, blue: 255},
                    alpha: 1,
                    lineWidth: 5,
                    visible: statsOn,
                    anchor: "MyAvatar"
                });
var debugStats = Overlays.addOverlay("text", {
                    x: backgroundX-199, y: backgroundY,
                    width: 200,
                    height: 180,
                    color: { red: 204, green: 204, blue: 204},
                    topMargin: 10,
                    leftMargin: 15,
                    visible: statsOn,
                    backgroundColor: { red: 34, green: 34, blue: 34},
                    alpha: 1.0,
                    text: "Debug Stats\n\n\nNothing to report yet."
                });
var debugStatsPeriodic = Overlays.addOverlay("text", {
                    x: backgroundX-199, y: backgroundY+179,
                    width: 200,
                    height: 392,
                    color: { red: 204, green: 204, blue: 204},
                    topMargin: 5,
                    leftMargin: 15,
                    visible: statsOn,
                    backgroundColor: { red: 34, green: 34, blue: 34},
                    alpha: 1.0,
                    text: "Debug Stats\n\n\nNothing to report yet."
                });
var walkWheelStats = Overlays.addOverlay("text", {
                    x: backgroundX-199, y: backgroundY+510,
                    width: 200,
                    height: 190,
                    color: { red: 204, green: 204, blue: 204},
                    topMargin: 5,
                    leftMargin: 15,
                    visible: statsOn,
                    backgroundColor: { red: 34, green: 34, blue: 34},
                    alpha: 1.0,
                    text: "WalkWheel Stats\n\n\nNothing to report yet.\n\n\nPlease start walking\nto see the walkwheel."
                });

// various show / hide GUI element functions
function doStandardMenu() {
    hidebuttonOverlays();
    hideJointControls();
    setBackground(controlsBackground);
    if(powerOn) setButtonOverlayVisible(onButton);
    else setButtonOverlayVisible(offButton);
    setButtonOverlayVisible(configWalkButton);
    setButtonOverlayVisible(configStandButton);
    setButtonOverlayVisible(configFlyingButton);
    setButtonOverlayVisible(hideButton);
    setSliderThumbsVisible(false);
    showFrontPanelButtons(true);
    showWalkStyleButtons(false);
}
function showFrontPanelButtons(showButtons) {

	if(avatarGender===FEMALE) {
		Overlays.editOverlay(femaleBigButtonSelected, { visible: showButtons } );
		Overlays.editOverlay(femaleBigButton, { visible: false } );
		Overlays.editOverlay(maleBigButtonSelected, { visible: false } );
		Overlays.editOverlay(maleBigButton, { visible: showButtons } );
	} else {
		Overlays.editOverlay(femaleBigButtonSelected, { visible: false } );
		Overlays.editOverlay(femaleBigButton, { visible: showButtons } );
		Overlays.editOverlay(maleBigButtonSelected, { visible: showButtons } );
		Overlays.editOverlay(maleBigButton, { visible: false } );
	}
	if(armsFree) {
		Overlays.editOverlay(armsFreeBigButtonSelected, { visible: showButtons } );
		Overlays.editOverlay(armsFreeBigButton, { visible: false } );
	} else {
		Overlays.editOverlay(armsFreeBigButtonSelected, { visible: false } );
		Overlays.editOverlay(armsFreeBigButton, { visible: showButtons } );
	}
	if(playFootStepSounds) {
		Overlays.editOverlay(footstepsBigButtonSelected, { visible: showButtons } );
		Overlays.editOverlay(footstepsBigButton, { visible: false } );
	} else {
		Overlays.editOverlay(footstepsBigButtonSelected, { visible: false } );
		Overlays.editOverlay(footstepsBigButton, { visible: showButtons } );
	}
}
function minimiseDialog() {

	if(minimised) {
        setBackground();
        hidebuttonOverlays();
        setSliderThumbsVisible(false);
        hideJointControls();
        showFrontPanelButtons(false);
        Overlays.editOverlay(controlsMinimisedTab, { visible: true } );
    } else {
        setInternalState(STANDING); // show all the controls again
        Overlays.editOverlay(controlsMinimisedTab, { visible: false } );
    }
}
function setBackground(backgroundName)  {
    for(var i in backgroundOverlays) {
        if(backgroundOverlays[i] === backgroundName)
            Overlays.editOverlay(backgroundName, { visible: true } );
        else Overlays.editOverlay(backgroundOverlays[i], { visible: false } );
    }
}
function setButtonOverlayVisible(buttonOverlayName) {
    for(var i in buttonOverlays) {
        if(buttonOverlays[i] === buttonOverlayName) {
            Overlays.editOverlay(buttonOverlayName, { visible: true } );
        }
    }
}
// top row menu type buttons (smaller)
function hidebuttonOverlays()  {
    for(var i in buttonOverlays) {
        Overlays.editOverlay(buttonOverlays[i], { visible: false } );
    }
}
function hideJointControls() {
    for(var i in jointsControlOverlays) {
        Overlays.editOverlay(jointsControlOverlays[i], { visible: false } );
    }
}
function setSliderThumbsVisible(thumbsVisible) {
    for(var i = 0 ; i < sliderThumbOverlays.length ; i++) {
        Overlays.editOverlay(sliderThumbOverlays[i], { visible: thumbsVisible } );
    }
}
function initialiseJointsEditingPanel(propertyIndex) {

    selectedJointIndex = propertyIndex;

    // set the image for the selected joint on the character control
    hideJointControls();
    switch (selectedJointIndex) {
        case 0:
            Overlays.editOverlay(hipsJointControl, { visible: true });
            break;
        case 1:
            Overlays.editOverlay(upperLegsJointControl, { visible: true });
            break;
        case 2:
            Overlays.editOverlay(lowerLegsJointControl, { visible: true });
            break;
        case 3:
            Overlays.editOverlay(feetJointControl, { visible: true });
            break;
        case 4:
            Overlays.editOverlay(toesJointControl, { visible: true });
            break;
        case 5:
            Overlays.editOverlay(spineJointControl, { visible: true });
            break;
        case 6:
            Overlays.editOverlay(spine1JointControl, { visible: true });
            break;
        case 7:
            Overlays.editOverlay(spine2JointControl, { visible: true });
            break;
        case 8:
            Overlays.editOverlay(shouldersJointControl, { visible: true });
            break;
        case 9:
            Overlays.editOverlay(upperArmsJointControl, { visible: true });
            break;
        case 10:
            Overlays.editOverlay(forearmsJointControl, { visible: true });
            break;
        case 11:
            Overlays.editOverlay(handsJointControl, { visible: true });
            break;
        case 12:
            Overlays.editOverlay(headJointControl, { visible: true });
            break;
    }

    // set sliders to adjust individual joint properties
    var i = 0;
    var yLocation = backgroundY+359;

	// pitch your role
    var sliderXPos = currentAnimation.joints[selectedJointIndex].pitch
    		/ sliderRanges.joints[selectedJointIndex].pitchRange * sliderRangeX;
    Overlays.editOverlay(sliderThumbOverlays[i], { x: minSliderX + sliderXPos, y: yLocation+=30, visible: true });
    sliderXPos = currentAnimation.joints[selectedJointIndex].yaw
    		/ sliderRanges.joints[selectedJointIndex].yawRange * sliderRangeX;
    Overlays.editOverlay(sliderThumbOverlays[++i], { x: minSliderX + sliderXPos, y: yLocation+=30, visible: true });
    sliderXPos = currentAnimation.joints[selectedJointIndex].roll
    		/ sliderRanges.joints[selectedJointIndex].rollRange * sliderRangeX;
    Overlays.editOverlay(sliderThumbOverlays[++i], { x: minSliderX + sliderXPos, y: yLocation+=30, visible: true });

    // set phases (full range, -180 to 180)
    sliderXPos = (90 + currentAnimation.joints[selectedJointIndex].pitchPhase/2)/180 * sliderRangeX;
    Overlays.editOverlay(sliderThumbOverlays[++i], { x: minSliderX + sliderXPos, y: yLocation+=30, visible: true });
    sliderXPos = (90 + currentAnimation.joints[selectedJointIndex].yawPhase/2)/180 * sliderRangeX;
    Overlays.editOverlay(sliderThumbOverlays[++i], { x: minSliderX + sliderXPos, y: yLocation+=30, visible: true });
    sliderXPos = (90 + currentAnimation.joints[selectedJointIndex].rollPhase/2)/180 * sliderRangeX;
    Overlays.editOverlay(sliderThumbOverlays[++i], { x: minSliderX + sliderXPos, y: yLocation+=30, visible: true });

    // offset ranges are also -ve thr' zero to +ve, so have to offset
    sliderXPos = (((sliderRanges.joints[selectedJointIndex].pitchOffsetRange+currentAnimation.joints[selectedJointIndex].pitchOffset)/2)
    			/sliderRanges.joints[selectedJointIndex].pitchOffsetRange) * sliderRangeX;
    Overlays.editOverlay(sliderThumbOverlays[++i], { x: minSliderX + sliderXPos, y: yLocation+=30, visible: true });
    sliderXPos = (((sliderRanges.joints[selectedJointIndex].yawOffsetRange+currentAnimation.joints[selectedJointIndex].yawOffset)/2)
    			/sliderRanges.joints[selectedJointIndex].yawOffsetRange) * sliderRangeX;
    Overlays.editOverlay(sliderThumbOverlays[++i], { x: minSliderX + sliderXPos, y: yLocation+=30, visible: true });
    sliderXPos = (((sliderRanges.joints[selectedJointIndex].rollOffsetRange+currentAnimation.joints[selectedJointIndex].rollOffset)/2)
    			/sliderRanges.joints[selectedJointIndex].rollOffsetRange) * sliderRangeX;
    Overlays.editOverlay(sliderThumbOverlays[++i], { x: minSliderX + sliderXPos, y: yLocation+=30, visible: true });
}

function initialiseWalkTweaks() {

    // set sliders to adjust walk properties
    var i = 0;
    var yLocation = backgroundY+71;

    var sliderXPos = currentAnimation.settings.baseFrequency / MAX_WALK_SPEED * sliderRangeX;   // walk speed
    Overlays.editOverlay(sliderThumbOverlays[i], { x: minSliderX + sliderXPos, y: yLocation+=60, visible: true });
    sliderXPos = 0 * sliderRangeX;             // start flying speed - depricated
    Overlays.editOverlay(sliderThumbOverlays[++i], { x: minSliderX + sliderXPos, y: yLocation+=60, visible: true });
    sliderXPos = currentAnimation.joints[0].sway / sliderRanges.joints[0].swayRange * sliderRangeX;     // Hips sway
    Overlays.editOverlay(sliderThumbOverlays[++i], { x: minSliderX + sliderXPos, y: yLocation+=60, visible: true });
    sliderXPos = currentAnimation.joints[0].bob / sliderRanges.joints[0].bobRange * sliderRangeX;       // Hips bob
    Overlays.editOverlay(sliderThumbOverlays[++i], { x: minSliderX + sliderXPos, y: yLocation+=60, visible: true });
    sliderXPos = currentAnimation.joints[0].thrust / sliderRanges.joints[0].thrustRange * sliderRangeX; // Hips thrust
    Overlays.editOverlay(sliderThumbOverlays[++i], { x: minSliderX + sliderXPos, y: yLocation+=60, visible: true });
    sliderXPos = (((sliderRanges.joints[1].rollOffsetRange+currentAnimation.joints[1].rollOffset)/2) // legs separation - is upper legs roll offset
    				/ sliderRanges.joints[1].rollOffsetRange) * sliderRangeX;
    Overlays.editOverlay(sliderThumbOverlays[++i], { x: minSliderX + sliderXPos, y: yLocation+=60, visible: true });
    sliderXPos = currentAnimation.joints[1].pitch / sliderRanges.joints[1].pitchRange * sliderRangeX; // stride - is upper legs pitch
    Overlays.editOverlay(sliderThumbOverlays[++i], { x: minSliderX + sliderXPos, y: yLocation+=60, visible: true });
    sliderXPos = currentAnimation.joints[9].yaw / sliderRanges.joints[9].yawRange * sliderRangeX; // arms swing - is upper arms yaw
    Overlays.editOverlay(sliderThumbOverlays[++i], { x: minSliderX + sliderXPos, y: yLocation+=60, visible: true });
    sliderXPos = (((sliderRanges.joints[9].pitchOffsetRange-currentAnimation.joints[9].pitchOffset)/2)
    				/ sliderRanges.joints[9].pitchOffsetRange) * sliderRangeX; // arms out - is upper arms pitch offset
    Overlays.editOverlay(sliderThumbOverlays[++i], { x: minSliderX + sliderXPos, y: yLocation+=60, visible: true });
}

function showWalkStyleButtons(showButtons) {

	// set all big buttons to hidden, but skip the first 8, as are for the front panel
	for(var i = 8 ; i < bigButtonOverlays.length ; i++) {
		Overlays.editOverlay(bigButtonOverlays[i], { visible: false });
	}

	if(!showButtons) return;

	// set all the non-selected ones to showing
	for(var i = 8 ; i < bigButtonOverlays.length ; i+=2) {
		Overlays.editOverlay(bigButtonOverlays[i], { visible: showButtons });
	}

	// set the currently selected one
	if(selectedWalk === femaleStrutWalk || selectedWalk === maleStrutWalk) {
		Overlays.editOverlay(strutWalkBigButtonSelected, { visible: showButtons });
		Overlays.editOverlay(strutWalkBigButton, {visible: false});
	}
}

// mouse event handlers
var movingSliderOne = false;
var movingSliderTwo = false;
var movingSliderThree = false;
var movingSliderFour = false;
var movingSliderFive = false;
var movingSliderSix = false;
var movingSliderSeven = false;
var movingSliderEight = false;
var movingSliderNine = false;

function mousePressEvent(event) {

    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});

    // check for a character joint control click
    switch (clickedOverlay) {

        case hideButton:
            Overlays.editOverlay(hideButton, { visible: false } );
            Overlays.editOverlay(hideButtonSelected, { visible: true } );
            return;

        case backButton:
            Overlays.editOverlay(backButton, { visible: false } );
            Overlays.editOverlay(backButtonSelected, { visible: true } );
            return;

        case controlsMinimisedTab:
            // TODO: add visual user feedback for tab click
            return;

		case footstepsBigButton:
			playFootStepSounds = true;
			Overlays.editOverlay(footstepsBigButtonSelected, { visible: true } );
			Overlays.editOverlay(footstepsBigButton, { visible: false } );
			return;

		case footstepsBigButtonSelected:
			playFootStepSounds = false;
			Overlays.editOverlay(footstepsBigButton, { visible: true } );
			Overlays.editOverlay(footstepsBigButtonSelected, { visible: false } );
			return;

		case femaleBigButton:
		case maleBigButtonSelected:
			avatarGender = FEMALE;
			selectedWalk = femaleStrutWalk;
			selectedStand = femaleStandOne;
			selectedFlyUp  = femaleFlyingUp;
			selectedFly = femaleFlying;
			selectedFlyDown = femaleFlyingDown;
			selectedSideStepLeft = femaleSideStepLeft;
			selectedSideStepRight = femaleSideStepRight;
			Overlays.editOverlay(femaleBigButtonSelected, { visible: true } );
			Overlays.editOverlay(femaleBigButton, { visible: false } );
			Overlays.editOverlay(maleBigButton, { visible: true } );
			Overlays.editOverlay(maleBigButtonSelected, { visible: false } );
			return;

		case armsFreeBigButton:
			armsFree = true;
			Overlays.editOverlay(armsFreeBigButtonSelected, { visible: true } );
			Overlays.editOverlay(armsFreeBigButton, { visible: false } );
			return;

		case armsFreeBigButtonSelected:
			armsFree = false;
			Overlays.editOverlay(armsFreeBigButtonSelected, { visible: false } );
			Overlays.editOverlay(armsFreeBigButton, { visible: true } );
			return;

		case maleBigButton:
		case femaleBigButtonSelected:
			avatarGender = MALE;
			selectedWalk = maleStrutWalk;
			selectedStand = maleStandOne;
			selectedFlyUp  = maleFlyingUp;
			selectedFly = maleFlying;
			selectedFlyDown = maleFlyingDown;
			selectedSideStepLeft = maleSideStepLeft;
			selectedSideStepRight = maleSideStepRight;
			Overlays.editOverlay(femaleBigButton, { visible: true } );
			Overlays.editOverlay(femaleBigButtonSelected, { visible: false } );
			Overlays.editOverlay(maleBigButtonSelected, { visible: true } );
			Overlays.editOverlay(maleBigButton, { visible: false } );
			return;


		case strutWalkBigButton:
			if(avatarGender===FEMALE) selectedWalk = femaleStrutWalk;
			else selectedWalk = maleStrutWalk;
			currentAnimation = selectedWalk;
			showWalkStyleButtons(true);
			return;

		case strutWalkBigButtonSelected:

			// toggle forwards / backwards walk display
			if(principleDirection===DIRECTION_FORWARDS) {
				principleDirection=DIRECTION_BACKWARDS;
			} else principleDirection=DIRECTION_FORWARDS;
			return;

        case sliderOne:
            movingSliderOne = true;
            return;

        case sliderTwo:
            movingSliderTwo = true;
            return;

        case sliderThree:
            movingSliderThree = true;
            return;

        case sliderFour:
            movingSliderFour = true;
            return;

        case sliderFive:
            movingSliderFive = true;
            return;

        case sliderSix:
            movingSliderSix = true;
            return;

        case sliderSeven:
            movingSliderSeven = true;
            return;

        case sliderEight:
            movingSliderEight = true;
            return;

        case sliderNine:
            movingSliderNine = true;
            return;
    }

    if( INTERNAL_STATE===CONFIG_WALK_JOINTS ||
	    INTERNAL_STATE===CONFIG_STANDING ||
	    INTERNAL_STATE===CONFIG_FLYING ||
	    INTERNAL_STATE===CONFIG_FLYING_UP ||
	    INTERNAL_STATE===CONFIG_FLYING_DOWN  ||
	    INTERNAL_STATE===CONFIG_SIDESTEP_LEFT ||
	    INTERNAL_STATE===CONFIG_SIDESTEP_RIGHT ) {

		// check for new joint selection and update display accordingly
        var clickX = event.x - backgroundX - 75;
        var clickY = event.y - backgroundY - 92;

        if(clickX>60&&clickX<120&&clickY>123&&clickY<155) {
            initialiseJointsEditingPanel(0);
            return;
        }
        else if(clickX>63&&clickX<132&&clickY>156&&clickY<202) {
            initialiseJointsEditingPanel(1);
            return;
        }
        else if(clickX>58&&clickX<137&&clickY>203&&clickY<250) {
            initialiseJointsEditingPanel(2);
            return;
        }
        else if(clickX>58&&clickX<137&&clickY>250&&clickY<265) {
            initialiseJointsEditingPanel(3);
            return;
        }
        else if(clickX>58&&clickX<137&&clickY>265&&clickY<280) {
            initialiseJointsEditingPanel(4);
            return;
        }
        else if(clickX>78&&clickX<121&&clickY>111&&clickY<128) {
            initialiseJointsEditingPanel(5);
            return;
        }
        else if(clickX>78&&clickX<128&&clickY>89&&clickY<111) {
            initialiseJointsEditingPanel(6);
            return;
        }
        else if(clickX>85&&clickX<118&&clickY>77&&clickY<94) {
            initialiseJointsEditingPanel(7);
            return;
        }
        else if(clickX>64&&clickX<125&&clickY>55&&clickY<77) {
            initialiseJointsEditingPanel(8);
            return;
        }
        else if((clickX>44&&clickX<73&&clickY>71&&clickY<94)
              ||(clickX>125&&clickX<144&&clickY>71&&clickY<94)) {
            initialiseJointsEditingPanel(9);
            return;
        }
        else if((clickX>28&&clickX<57&&clickY>94&&clickY<119)
              ||(clickX>137&&clickX<170&&clickY>97&&clickY<114)) {
            initialiseJointsEditingPanel(10);
            return;
        }
        else if((clickX>18&&clickX<37&&clickY>115&&clickY<136)
              ||(clickX>157&&clickX<182&&clickY>115&&clickY<136)) {
            initialiseJointsEditingPanel(11);
            return;
        }
        else if(clickX>81&&clickX<116&&clickY>12&&clickY<53) {
            initialiseJointsEditingPanel(12);
            return;
        }
    }
}
function mouseMoveEvent(event) {

    // only need deal with slider changes
    if(powerOn) {

        if( INTERNAL_STATE===CONFIG_WALK_JOINTS ||
            INTERNAL_STATE===CONFIG_STANDING ||
            INTERNAL_STATE===CONFIG_FLYING ||
            INTERNAL_STATE===CONFIG_FLYING_UP ||
            INTERNAL_STATE===CONFIG_FLYING_DOWN ||
            INTERNAL_STATE===CONFIG_SIDESTEP_LEFT ||
            INTERNAL_STATE===CONFIG_SIDESTEP_RIGHT ) {

            var thumbClickOffsetX = event.x - minSliderX;
            var thumbPositionNormalised = thumbClickOffsetX / sliderRangeX;
            if(thumbPositionNormalised<0) thumbPositionNormalised = 0;
            if(thumbPositionNormalised>1) thumbPositionNormalised = 1;
            var sliderX = thumbPositionNormalised * sliderRangeX ; // sets range

            if(movingSliderOne) { // currently selected joint pitch
                Overlays.editOverlay(sliderOne, { x: sliderX + minSliderX} );
                currentAnimation.joints[selectedJointIndex].pitch = thumbPositionNormalised * sliderRanges.joints[selectedJointIndex].pitchRange;
            }
            else if(movingSliderTwo) { // currently selected joint yaw
                Overlays.editOverlay(sliderTwo, { x: sliderX + minSliderX} );
                currentAnimation.joints[selectedJointIndex].yaw = thumbPositionNormalised * sliderRanges.joints[selectedJointIndex].yawRange;
            }
            else if(movingSliderThree) { // currently selected joint roll
                Overlays.editOverlay(sliderThree, { x: sliderX + minSliderX} );
                currentAnimation.joints[selectedJointIndex].roll = thumbPositionNormalised * sliderRanges.joints[selectedJointIndex].rollRange;
            }
            else if(movingSliderFour) { // currently selected joint pitch phase
                Overlays.editOverlay(sliderFour, { x: sliderX + minSliderX} );
                var newPhase = 360 * thumbPositionNormalised - 180;
                currentAnimation.joints[selectedJointIndex].pitchPhase = newPhase;
            }
            else if(movingSliderFive) { // currently selected joint yaw phase;
                Overlays.editOverlay(sliderFive, { x: sliderX + minSliderX} );
                var newPhase = 360 * thumbPositionNormalised - 180;
                currentAnimation.joints[selectedJointIndex].yawPhase = newPhase;
            }
            else if(movingSliderSix) { // currently selected joint roll phase
                Overlays.editOverlay(sliderSix, { x: sliderX + minSliderX} );
                var newPhase = 360 * thumbPositionNormalised - 180;
                currentAnimation.joints[selectedJointIndex].rollPhase = newPhase;
            }
            else if(movingSliderSeven) { // currently selected joint pitch offset
                Overlays.editOverlay(sliderSeven, { x: sliderX + minSliderX} ); // currently selected joint pitch offset
                var newOffset = (thumbPositionNormalised-0.5) * 2 * sliderRanges.joints[selectedJointIndex].pitchOffsetRange;
                currentAnimation.joints[selectedJointIndex].pitchOffset = newOffset;
            }
            else if(movingSliderEight) { // currently selected joint yaw offset
                Overlays.editOverlay(sliderEight, { x: sliderX + minSliderX} ); // currently selected joint yaw offset
                var newOffset = (thumbPositionNormalised-0.5) * 2 * sliderRanges.joints[selectedJointIndex].yawOffsetRange;
                currentAnimation.joints[selectedJointIndex].yawOffset = newOffset;
            }
            else if(movingSliderNine) { // currently selected joint roll offset
                Overlays.editOverlay(sliderNine, { x: sliderX + minSliderX} ); // currently selected joint roll offset
                var newOffset = (thumbPositionNormalised-0.5) * 2 * sliderRanges.joints[selectedJointIndex].rollOffsetRange;
                currentAnimation.joints[selectedJointIndex].rollOffset = newOffset;
            }
        }
        else if(INTERNAL_STATE===CONFIG_WALK_TWEAKS) {

            var thumbClickOffsetX = event.x - minSliderX;
            var thumbPositionNormalised = thumbClickOffsetX / sliderRangeX;
            if(thumbPositionNormalised<0) thumbPositionNormalised = 0;
            if(thumbPositionNormalised>1) thumbPositionNormalised = 1;
            var sliderX = thumbPositionNormalised * sliderRangeX ; // sets range

            if(movingSliderOne) { // walk speed
                paused = true; // avoid nasty jittering
                Overlays.editOverlay(sliderOne, { x: sliderX + minSliderX} );
                currentAnimation.settings.baseFrequency = thumbPositionNormalised * MAX_WALK_SPEED;
            }
            else if(movingSliderTwo) {  // take flight speed
                Overlays.editOverlay(sliderTwo, { x: sliderX + minSliderX} );
                //currentAnimation.settings.takeFlightVelocity = thumbPositionNormalised * 300;
            }
            else if(movingSliderThree) { // hips sway
                Overlays.editOverlay(sliderThree, { x: sliderX + minSliderX} );
                currentAnimation.joints[0].sway = thumbPositionNormalised * sliderRanges.joints[0].swayRange;
            }
            else if(movingSliderFour) { // hips bob
                Overlays.editOverlay(sliderFour, { x: sliderX + minSliderX} );
                currentAnimation.joints[0].bob = thumbPositionNormalised * sliderRanges.joints[0].bobRange;
            }
            else if(movingSliderFive) { // hips thrust
				Overlays.editOverlay(sliderFive, { x: sliderX + minSliderX} );
                currentAnimation.joints[0].thrust = thumbPositionNormalised * sliderRanges.joints[0].thrustRange;
            }
            else if(movingSliderSix) { // legs separation
                Overlays.editOverlay(sliderSix, { x: sliderX + minSliderX} );
                currentAnimation.joints[1].rollOffset = (thumbPositionNormalised-0.5) * 2 * sliderRanges.joints[1].rollOffsetRange;
            }
            else if(movingSliderSeven) { // stride
                Overlays.editOverlay(sliderSeven, { x: sliderX + minSliderX} );
                currentAnimation.joints[1].pitch = thumbPositionNormalised * sliderRanges.joints[1].pitchRange;
            }
            else if(movingSliderEight) { // arms swing = upper arms yaw
                Overlays.editOverlay(sliderEight, { x: sliderX + minSliderX} );
                currentAnimation.joints[9].yaw = thumbPositionNormalised * sliderRanges.joints[9].yawRange;
            }
            else if(movingSliderNine) { // arms out = upper arms pitch offset
                Overlays.editOverlay(sliderNine, { x: sliderX + minSliderX} );
                currentAnimation.joints[9].pitchOffset = (thumbPositionNormalised-0.5) * -2 * sliderRanges.joints[9].pitchOffsetRange;
            }
        }
    }
}
function mouseReleaseEvent(event) {

    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});

    if(paused) paused = false;

    if(clickedOverlay === offButton) {
        powerOn = true;
        Overlays.editOverlay(offButton, { visible: false } );
        Overlays.editOverlay(onButton,  { visible: true } );
        stand();
    }
    else if(clickedOverlay === hideButton || clickedOverlay === hideButtonSelected){
        Overlays.editOverlay(hideButton, { visible: true } );
        Overlays.editOverlay(hideButtonSelected, { visible: false } );
        minimised = true;
        minimiseDialog();
    }
    else if(clickedOverlay === controlsMinimisedTab) {
        minimised = false;
        minimiseDialog();
    }
    else if(powerOn) {

        if(movingSliderOne) movingSliderOne = false;
        else if(movingSliderTwo) movingSliderTwo = false;
        else if(movingSliderThree) movingSliderThree = false;
        else if(movingSliderFour) movingSliderFour = false;
        else if(movingSliderFive) movingSliderFive = false;
        else if(movingSliderSix) movingSliderSix = false;
        else if(movingSliderSeven) movingSliderSeven = false;
        else if(movingSliderEight) movingSliderEight = false;
        else if(movingSliderNine) movingSliderNine = false;
        else {

            switch(clickedOverlay) {

                case configWalkButtonSelected:
                case configStandButtonSelected:
                case configSideStepLeftButtonSelected:
                case configSideStepRightButtonSelected:
                case configFlyingButtonSelected:
                case configFlyingUpButtonSelected:
                case configFlyingDownButtonSelected:
                case configWalkStylesButtonSelected:
                case configWalkTweaksButtonSelected:
                case configWalkJointsButtonSelected:
                    setInternalState(STANDING);
                    break;

                case onButton:
                    powerOn = false;
                    setInternalState(STANDING);
                    Overlays.editOverlay(offButton, { visible: true } );
                    Overlays.editOverlay(onButton,  { visible: false } );
                    resetJoints();
                    break;

                case backButton:
                case backButtonSelected:
                    Overlays.editOverlay(backButton, { visible: false } );
                    Overlays.editOverlay(backButtonSelected, { visible: false } );
                    setInternalState(STANDING);
                    break;

                case configWalkStylesButton:
                    setInternalState(CONFIG_WALK_STYLES);
                    break;

                case configWalkTweaksButton:
                    setInternalState(CONFIG_WALK_TWEAKS);
                    break;

                case configWalkJointsButton:
                    setInternalState(CONFIG_WALK_JOINTS);
                    break;

                case configWalkButton:
                    setInternalState(CONFIG_WALK_STYLES); //  set the default walk adjustment panel here (i.e. first panel shown when Walk button clicked)
                    break;

                case configStandButton:
                    setInternalState(CONFIG_STANDING);
                    break;

                case configSideStepLeftButton:
                    setInternalState(CONFIG_SIDESTEP_LEFT);
                    break;

                case configSideStepRightButton:
                    setInternalState(CONFIG_SIDESTEP_RIGHT);
                    break;

                case configFlyingButton:
                    setInternalState(CONFIG_FLYING);
                    break;

                case configFlyingUpButton:
                    setInternalState(CONFIG_FLYING_UP);
                    break;

                case configFlyingDownButton:
                    setInternalState(CONFIG_FLYING_DOWN);
                    break;
            }
        }
    }
}
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);

// Script ending
Script.scriptEnding.connect(function() {

    // remove the background overlays
    for(var i in backgroundOverlays) {
        Overlays.deleteOverlay(backgroundOverlays[i]);
    }
    // remove the button overlays
    for(var i in buttonOverlays) {
        Overlays.deleteOverlay(buttonOverlays[i]);
    }
    // remove the slider thumb overlays
    for(var i in sliderThumbOverlays) {
        Overlays.deleteOverlay(sliderThumbOverlays[i]);
    }
    // remove the character joint control overlays
    for(var i in bigButtonOverlays) {
        Overlays.deleteOverlay(jointsControlOverlays[i]);
    }
    // remove the big button overlays
    for(var i in bigButtonOverlays) {
        Overlays.deleteOverlay(bigButtonOverlays[i]);
    }
    // remove the mimimised tab
    Overlays.deleteOverlay(controlsMinimisedTab);

    // remove the walk wheel overlays
    Overlays.deleteOverlay(walkWheelYLine);
    Overlays.deleteOverlay(walkWheelZLine);
    Overlays.deleteOverlay(walkWheelStats);

    // remove the debug stats overlays
    Overlays.deleteOverlay(debugStats);
    Overlays.deleteOverlay(debugStatsPeriodic);
});

var sideStep = 0.002; // i.e. 2mm increments whilst sidestepping - JS movement keys don't work well :-(
function keyPressEvent(event) {

	if (event.text == "q") {
		// export currentAnimation as json string when q key is pressed. reformat result at http://www.freeformatter.com/json-formatter.html
        print('\n');
        print('walk.js dumping animation: '+currentAnimation.name+'\n');
        print('\n');
        print(JSON.stringify(currentAnimation), null, '\t');
    }
    else if (event.text == "t") {
        statsOn = !statsOn;
		Overlays.editOverlay(debugStats,         {visible: statsOn});
		Overlays.editOverlay(debugStatsPeriodic, {visible: statsOn});
		Overlays.editOverlay(walkWheelStats,     {visible: statsOn});
		Overlays.editOverlay(walkWheelYLine,     {visible: statsOn});
		Overlays.editOverlay(walkWheelZLine,     {visible: statsOn});
    }
}
Controller.keyPressEvent.connect(keyPressEvent);

// get the list of joint names
var jointList = MyAvatar.getJointNames();

// clear the joint data so can calculate hips to feet distance
for(var i = 0 ; i < 5 ; i++) {
    MyAvatar.setJointData(i, Quat.fromPitchYawRollDegrees(0,0,0));
}
// used to position the visual representation of the walkwheel only
var hipsToFeetDistance = MyAvatar.getJointPosition("Hips").y - MyAvatar.getJointPosition("RightFoot").y;

// This script is designed around the Finite State Machine (FSM) model, so to start things up we just select the STANDING state.
setInternalState(STANDING);