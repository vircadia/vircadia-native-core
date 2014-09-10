//
//  walk.js
//
//
//  Created by Davedub, August / September 2014
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


// Set asset paths here:

// path to the animation files
//var pathToAnimFiles = 'http://localhost/downloads/hf/scripts/animation-files/';									// loads fine (files must be present on localhost)
//var pathToAnimFiles = 'http://highfidelity.davedub.co.uk/procedural/walk/animation-files/';         				// files present, but load with errors - weird
var pathToAnimFiles = 'http://s3-us-west-1.amazonaws.com/highfidelity-public/procedural-animator/files/';			// working (but only without https)

// path to the images used for the overlays
//var pathToOverlays = 'http://localhost/downloads/hf/overlays/';													// loads fine (files must be present on localhost)
//var pathToOverlays = 'http://highfidelity.davedub.co.uk/procedural/walk/overlays/';								// files present, but won't load - weird
var pathToOverlays = 'http://s3-us-west-1.amazonaws.com/highfidelity-public/procedural-animator/images/';			// working (but only without https)

// path to the sounds used for the footsteps
var pathToSounds = 'http://highfidelity-public.s3-us-west-1.amazonaws.com/sounds/Footsteps/';
//var pathToSounds = 'http://localhost/downloads/hf/sounds/Footsteps/';


// load all the animation datafiles ( 15 female, 15 male ~ 240k )
Script.include(pathToAnimFiles+"dd-female-cool-walk-animation.js");
Script.include(pathToAnimFiles+"dd-female-elderly-walk-animation.js");
Script.include(pathToAnimFiles+"dd-female-power-walk-animation.js");
Script.include(pathToAnimFiles+"dd-female-run-animation.js");
Script.include(pathToAnimFiles+"dd-female-sexy-walk-animation.js");
Script.include(pathToAnimFiles+"dd-female-shuffle-animation.js");
Script.include(pathToAnimFiles+"dd-female-random-walk-animation.js");
Script.include(pathToAnimFiles+"dd-female-strut-walk-animation.js");
Script.include(pathToAnimFiles+"dd-female-tough-walk-animation.js");
Script.include(pathToAnimFiles+"dd-female-flying-up-animation.js");
Script.include(pathToAnimFiles+"dd-female-flying-animation.js");
Script.include(pathToAnimFiles+"dd-female-flying-down-animation.js");
Script.include(pathToAnimFiles+"dd-female-standing-one-animation.js");
Script.include(pathToAnimFiles+"dd-female-standing-two-animation.js");
Script.include(pathToAnimFiles+"dd-female-standing-three-animatiom.js");
Script.include(pathToAnimFiles+"dd-male-cool-walk-animation.js");
Script.include(pathToAnimFiles+"dd-male-elderly-walk-animation.js");
Script.include(pathToAnimFiles+"dd-male-power-walk-animation.js");
Script.include(pathToAnimFiles+"dd-male-run-animation.js");
Script.include(pathToAnimFiles+"dd-male-sexy-walk-animation.js");
Script.include(pathToAnimFiles+"dd-male-shuffle-animation.js");
Script.include(pathToAnimFiles+"dd-male-random-walk-animation.js");
Script.include(pathToAnimFiles+"dd-male-strut-walk-animation.js");
Script.include(pathToAnimFiles+"dd-male-tough-walk-animation.js");
Script.include(pathToAnimFiles+"dd-male-flying-up-animation.js");
Script.include(pathToAnimFiles+"dd-male-flying-animation.js");
Script.include(pathToAnimFiles+"dd-male-flying-down-animation.js");
Script.include(pathToAnimFiles+"dd-male-standing-one-animation.js");
Script.include(pathToAnimFiles+"dd-male-standing-two-animation.js");
Script.include(pathToAnimFiles+"dd-male-standing-three-animation.js");

// read in the data from the animation files
var FemaleCoolWalkFile = new FemaleCoolWalk();
var femaleCoolWalk = FemaleCoolWalkFile.loadAnimation();
var FemaleElderlyWalkFile = new FemaleElderlyWalk();
var femaleElderlyWalk = FemaleElderlyWalkFile.loadAnimation();
var FemalePowerWalkFile = new FemalePowerWalk();
var femalePowerWalk = FemalePowerWalkFile.loadAnimation();
var FemaleRunFile = new FemaleRun();
var femaleRun = FemaleRunFile.loadAnimation();
var FemaleSexyWalkFile = new FemaleSexyWalk();
var femaleSexyWalk = FemaleSexyWalkFile.loadAnimation();
var FemaleShuffleFile = new FemaleShuffle();
var femaleShuffle = FemaleShuffleFile.loadAnimation();
var FemaleRandomWalkFile = new FemaleRandomWalk();
var femaleRandomWalk = FemaleRandomWalkFile.loadAnimation();
var FemaleStrutWalkFile = new FemaleStrutWalk();
var femaleStrutWalk = FemaleStrutWalkFile.loadAnimation();
var FemaleToughWalkFile = new FemaleToughWalk();
var femaleToughWalk = FemaleToughWalkFile.loadAnimation();
var FemaleFlyingUpFile = new FemaleFlyingUp();
var femaleFlyingUp = FemaleFlyingUpFile.loadAnimation();
var FemaleFlyingFile = new FemaleFlying();
var femaleFlying = FemaleFlyingFile.loadAnimation();
var FemaleFlyingDownFile = new FemaleFlyingDown();
var femaleFlyingDown = FemaleFlyingDownFile.loadAnimation();
var FemaleStandOneFile = new FemaleStandingOne();
var femaleStandOne = FemaleStandOneFile.loadAnimation();
var FemaleStandTwoFile = new FemaleStandingTwo();
var femaleStandTwo = FemaleStandTwoFile.loadAnimation();
var FemaleStandThreeFile = new FemaleStandingThree();
var femaleStandThree = FemaleStandThreeFile.loadAnimation();
var MaleCoolWalkFile = new MaleCoolWalk();
var maleCoolWalk = MaleCoolWalkFile.loadAnimation();
var MaleElderlyWalkFile = new MaleElderlyWalk();
var maleElderlyWalk = MaleElderlyWalkFile.loadAnimation();
var MalePowerWalkFile = new MalePowerWalk();
var malePowerWalk = MalePowerWalkFile.loadAnimation();
var MaleRunFile = new MaleRun();
var maleRun = MaleRunFile.loadAnimation();
var MaleSexyWalkFile = new MaleSexyWalk();
var maleSexyWalk = MaleSexyWalkFile.loadAnimation();
var MaleShuffleFile = new MaleShuffle();
var maleShuffle = MaleShuffleFile.loadAnimation();
var MaleRandomWalkFile = new MaleRandomWalk();
var maleRandomWalk = MaleRandomWalkFile.loadAnimation();
var MaleStrutWalkFile = new MaleStrutWalk();
var maleStrutWalk = MaleStrutWalkFile.loadAnimation();
var MaleToughWalkFile = new MaleToughWalk();
var maleToughWalk = MaleToughWalkFile.loadAnimation();
var MaleFlyingUpFile = new MaleFlyingUp();
var maleFlyingUp = MaleFlyingUpFile.loadAnimation();
var MaleFlyingFile = new MaleFlying();
var maleFlying = MaleFlyingFile.loadAnimation();
var MaleFlyingDownFile = new MaleFlyingDown();
var maleFlyingDown = MaleFlyingDownFile.loadAnimation();
var MaleStandOneFile = new MaleStandingOne();
var maleStandOne = MaleStandOneFile.loadAnimation();
var MaleStandTwoFile = new MaleStandingTwo();
var maleStandTwo = MaleStandTwoFile.loadAnimation();
var MaleStandThreeFile = new MaleStandingThree();
var maleStandThree = MaleStandThreeFile.loadAnimation();

// read in the sounds
var footsteps = [];
footsteps.push(new Sound(pathToSounds+"FootstepW2Left-12db.wav"));
footsteps.push(new Sound(pathToSounds+"FootstepW2Right-12db.wav"));
footsteps.push(new Sound(pathToSounds+"FootstepW3Left-12db.wav"));
footsteps.push(new Sound(pathToSounds+"FootstepW3Right-12db.wav"));
footsteps.push(new Sound(pathToSounds+"FootstepW5Left-12db.wav"));
footsteps.push(new Sound(pathToSounds+"FootstepW5Right-12db.wav"));

// all slider controls have a range (with the exception of phase controls (always +-180))
var sliderRanges =
{
   "joints":[
      {
         "name":"hips",
         "pitchRange":25,
         "yawRange":25,
         "rollRange":25,
         "pitchOffsetRange":25,
         "yawOffsetRange":25,
         "rollOffsetRange":25,
         "thrustRange":0.1,
         "bobRange":0.5,
         "swayRange":0.08
      },
      {
         "name":"upperLegs",
         "pitchRange":90,
         "yawRange":35,
         "rollRange":35,
         "pitchOffsetRange":60,
         "yawOffsetRange":20,
         "rollOffsetRange":20
      },
      {
         "name":"lowerLegs",
         "pitchRange":90,
         "yawRange":20,
         "rollRange":20,
         "pitchOffsetRange":90,
         "yawOffsetRange":20,
         "rollOffsetRange":20
      },
      {
         "name":"feet",
         "pitchRange":60,
         "yawRange":20,
         "rollRange":20,
         "pitchOffsetRange":60,
         "yawOffsetRange":50,
         "rollOffsetRange":50
      },
      {
         "name":"toes",
         "pitchRange":90,
         "yawRange":20,
         "rollRange":20,
         "pitchOffsetRange":90,
         "yawOffsetRange":20,
         "rollOffsetRange":20
      },
      {
         "name":"spine",
         "pitchRange":40,
         "yawRange":40,
         "rollRange":40,
         "pitchOffsetRange":90,
         "yawOffsetRange":50,
         "rollOffsetRange":50
      },
      {
         "name":"spine1",
         "pitchRange":20,
         "yawRange":40,
         "rollRange":20,
         "pitchOffsetRange":90,
         "yawOffsetRange":50,
         "rollOffsetRange":50
      },
      {
         "name":"spine2",
         "pitchRange":20,
         "yawRange":40,
         "rollRange":20,
         "pitchOffsetRange":90,
         "yawOffsetRange":50,
         "rollOffsetRange":50
      },
      {
         "name":"shoulders",
         "pitchRange":35,
         "yawRange":40,
         "rollRange":20,
         "pitchOffsetRange":180,
         "yawOffsetRange":180,
         "rollOffsetRange":180
      },
      {
         "name":"upperArms",
         "pitchRange":90,
         "yawRange":90,
         "rollRange":90,
         "pitchOffsetRange":180,
         "yawOffsetRange":180,
         "rollOffsetRange":180
      },
      {
         "name":"lowerArms",
         "pitchRange":90,
         "yawRange":90,
         "rollRange":120,
         "pitchOffsetRange":180,
         "yawOffsetRange":180,
         "rollOffsetRange":180
      },
      {
         "name":"hands",
         "pitchRange":90,
         "yawRange":180,
         "rollRange":90,
         "pitchOffsetRange":180,
         "yawOffsetRange":180,
         "rollOffsetRange":180
      },
      {
         "name":"head",
         "pitchRange":20,
         "yawRange":20,
         "rollRange":20,
         "pitchOffsetRange":90,
         "yawOffsetRange":90,
         "rollOffsetRange":90
      }
   ]
}

//  internal state (FSM based) constants
var STANDING = 2;
var WALKING = 4;
var FLYING = 8;
var CONFIG_WALK_STYLES = 16;
var CONFIG_WALK_TWEAKS = 32;
var CONFIG_WALK_JOINTS = 64;
var CONFIG_STANDING = 128;
var CONFIG_FLYING = 256;
var INTERNAL_STATE = STANDING;

// status
var powerOn = true;
var paused = false; // pause animation playback whilst adjusting certain parameters
var minimised = false;
var armsFree = false; // set true for hydra support - experimental
var statsOn = false;

// constants
var MAX_WALK_SPEED = 1257; // max oscillation speed
var FLYING_SPEED = 12.5;  // m/s - real humans can't run any faster
var TERMINAL_VELOCITY = 300;
var DIRECTION_UP = 1;
var DIRECTION_DOWN = 2;
var DIRECTION_LEFT = 4;
var DIRECTION_RIGHT = 8;
var DIRECTION_FORWARDS = 16;
var DIRECTION_BACKWARDS = 32;
var MALE = 64;
var FEMALE = 128;

// start of animation control section
var cumulativeTime = 0.0;
var lastOrientation;
var movementDirection = DIRECTION_FORWARDS;
var playFootStepSounds = true;
var avatarGender = FEMALE;
var selectedWalk = femaleStrutWalk; // the currently selected animation walk file
var selectedStand = femaleStandOne;
var selectedFlyUp = femaleFlyingUp;
var selectedFly = femaleFlying;
var selectedFlyDown = femaleFlyingDown;
var currentAnimation = selectedStand; // the current animation
var selectedJointIndex = 0; // the index of the joint currently selected for editing
// stride calibration
var maxFootForward = 0;
var maxFootBackwards = 0;
var strideLength = 0;
// walkwheel (foot / ground speed matching)
var walkCycleStart = 105; // best foot forwards - TODO: if different for different anims, add as setting to anim files
var walkWheelPosition = walkCycleStart;


// for showing walk wheel stats
var nFrames = 0;

// convert hips translations to global (i.e. take account of avi orientation)
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

        //MyAvatar.addThrust(AviTranslationOffset * 10);
        MyAvatar.position = {x: MyAvatar.position.x + AviTranslationOffset.x,
                             y: MyAvatar.position.y + AviTranslationOffset.y,
                             z: MyAvatar.position.z + AviTranslationOffset.z };
}

// convert a local (to the avi) translation to a global one
function globalToLocal(localTranslation) {

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

// zero out all joints
function resetJoints() {
    var avatarJointNames = MyAvatar.getJointNames();
    for (var i = 0; i < avatarJointNames.length; i++) {
        //MyAvatar.setJointData(avatarJointNames[i], Quat.fromPitchYawRollDegrees(0,0,0));
        MyAvatar.clearJointData(avatarJointNames[i]);
    }
}
// play footstep sound
function playFootstep(side) {
    var options = new AudioInjectionOptions();
    options.position = Camera.getPosition();
    options.volume = 0.7;
    var walkNumber = 2; // 0 to 2
    if(side===DIRECTION_RIGHT && playFootStepSounds) {
		//print('playing right footstep - if you can not hear sound, try turning your mic on then off again.');
		Audio.playSound(footsteps[walkNumber+1], options);
	}
    else if(side===DIRECTION_LEFT && playFootStepSounds) {
		//print('playing left footstep - if you can not hear sound, try turning your mic on then off again.');
		Audio.playSound(footsteps[walkNumber], options);
	}
}

// this is work in progress
// currently, it's not known if there are working finger joints on the avi
function curlFingers() {
    MyAvatar.setJointData("RightHandMiddle1", Quat.fromPitchYawRollDegrees(90,0,0));
    for(var i = 24 ; i < 44 ; i++) {
        MyAvatar.setJointData(jointList[i], Quat.fromPitchYawRollDegrees(0,90,0));
    }
    for(var i = 48 ; i < 68 ; i++) {
        MyAvatar.setJointData(jointList[i], Quat.fromPitchYawRollDegrees(10,0,90));
    }
}

// maths functions
function toRadians(degreesValue) {
    return degreesValue * Math.PI / 180;
}
function toDegrees(radiansValue) {
	return radiansValue * 180 / Math.PI;
}
function cubicRoot(x) {
    var y = Math.pow(Math.abs(x), 1/3);
    return x < 0 ? -y : y;
}

// animateAvatar - animates the avatar - TODO doxygen comments?
var nextStep = DIRECTION_RIGHT; // first step is always right, because the sine waves say so

function animateAvatar(deltaTime, velocity, principleDirection) {

	    // some slider adjustemnts cause a nasty flicker when adjusting
	    // pausing the animation stops this
        if(paused) return;

        var adjustedFrequency = currentAnimation.settings.baseFrequency;  // now only relevant for standing and flying

		// simple legs phase reversal for walking backwards
        var forwardModifier = 1;
        if(principleDirection===DIRECTION_BACKWARDS) {
			forwardModifier = -1;
		}

        // no need to lean forwards with speed increase if going directly upwards
        var leanPitchModifier = 1;
        if(principleDirection===DIRECTION_UP) {
			leanPitchModifier = 0;
		}


        if(currentAnimation === selectedWalk) {

			if(INTERNAL_STATE===CONFIG_WALK_STYLES ||
               INTERNAL_STATE===CONFIG_WALK_TWEAKS ||
               INTERNAL_STATE===CONFIG_WALK_JOINTS) {

				var footPos = globalToLocal(MyAvatar.getJointPosition("RightFoot"));
				var hipsPos = globalToLocal(MyAvatar.getJointPosition("Hips"));

				// calibrate stride length whilst not actually moving (for accuracy)
				if((hipsPos.z - footPos.z)<maxFootBackwards) maxFootBackwards = (hipsPos.z - footPos.z);
				else if ((hipsPos.z - footPos.z)>maxFootForward) maxFootForward = (hipsPos.z - footPos.z);

				strideLength = 2 * (maxFootForward-maxFootBackwards);

				// TODO: take note of the avi's stride length (can store in anim file or recalculate each time worn or edited)
				print('Stride length calibration: Your stride length is ' + strideLength + ' metres');  //  ~ 0.8211
			}
			else {

				if(strideLength===0) strideLength = 1.6422; // default for if not calibrated yet

				// wrap the stride length around a 'surveyor's wheel' twice and calculate the angular velocity at the given (linear) velocity
				// omega = v / r , where r = circumference / 2 PI , where circumference = 2 * stride
				var wheelRadius = strideLength / Math.PI;
				var angularVelocity = velocity / wheelRadius;

				// calculate the degrees turned (at this angular velocity) since last frame
				var radiansTurnedSinceLastFrame = deltaTime * angularVelocity;
				var degreesTurnedSinceLastFrame = toDegrees(radiansTurnedSinceLastFrame);

				// advance the walk wheel the appropriate amount
				// TODO: use radians, as Math.sin needs radians below anyway!
				walkWheelPosition += degreesTurnedSinceLastFrame;
				if( walkWheelPosition >= 360 )
				    walkWheelPosition = walkWheelPosition % 360;

				// set the new value for the exact correct walking speed for this velocity
				adjustedFrequency = 1;
				cumulativeTime = walkWheelPosition;

				// show stats and walk wheel?
				if(statsOn) {

					nFrames++;
					var distanceTravelled = velocity * deltaTime;
					var deltaTimeMS = deltaTime * 1000;

					// draw the walk wheel
					var yOffset = hipsToFeetDistance - wheelRadius;
					var sinWalkWheelPosition = wheelRadius * Math.sin(toRadians((forwardModifier*-1) * walkWheelPosition));
					var cosWalkWheelPosition = wheelRadius * Math.cos(toRadians((forwardModifier*-1) * -walkWheelPosition));
					var wheelZPos = {x:0, y:-sinWalkWheelPosition - yOffset, z: cosWalkWheelPosition};
					var wheelZEnd = {x:0, y:sinWalkWheelPosition - yOffset, z: -cosWalkWheelPosition};
					sinWalkWheelPosition = wheelRadius * Math.sin(toRadians(forwardModifier * walkWheelPosition+90));
					cosWalkWheelPosition = wheelRadius * Math.cos(toRadians(forwardModifier * walkWheelPosition+90));
					var wheelYPos = {x:0, y:sinWalkWheelPosition - yOffset, z: cosWalkWheelPosition};
					var wheelYEnd = {x:0, y:-sinWalkWheelPosition - yOffset, z: -cosWalkWheelPosition};
					Overlays.editOverlay(walkWheelYLine, {position:wheelYPos, end:wheelYEnd});
					Overlays.editOverlay(walkWheelZLine, {position:wheelZPos, end:wheelZEnd});

					// populate debug overlay
					var debugInfo = 'Frame number: '+nFrames
								  + '\nFrame time: '+deltaTimeMS.toFixed(2)
								  + ' mS\nVelocity: '+velocity.toFixed(2)
								  + ' m/s\nDistance: '+distanceTravelled.toFixed(3)
								  + ' m\nOmega: '+angularVelocity.toFixed(3)
								  + ' rad / s\nDeg to turn: '+degreesTurnedSinceLastFrame.toFixed(2)
								  + ' deg\nStride: '+strideLength.toFixed(3)
								  + ' m\nWheel position: '+cumulativeTime.toFixed(1)
								  + ' deg\n';
					Overlays.editOverlay(debugText, {text: debugInfo});

					//print('strideLength '+strideLength.toFixed(4)+' deltaTime '+deltaTimeMS.toFixed(4)+' distanceTravelled '+distanceTravelled.toFixed(3)+' velocity '+velocity.toFixed(3)+' angularVelocity '+angularVelocity.toFixed(3)+' degreesTurnedSinceLastFrame '+degreesTurnedSinceLastFrame.toFixed(3) + ' cumulativeTime '+cumulativeTime.toFixed(3));
				}
			}

		} else {
			nFrames = 0;
			walkWheelPosition = walkCycleStart; // best foot forwards for next time we walk
		}

        // TODO: optimise by precalculating and re-using Math.sin((cumulativeTime * femaleSexyWalk.settings.baseFrequency) when there is no phase to be applied
        // TODO: optimise by 'baking' offsets and phases after editing and use during normal playback

        // calcualte hips translation
        //var motorOscillation  = Math.sin(toRadians((cumulativeTime * adjustedFrequency * 2) + currentAnimation.joints[0].thrustPhase)) + currentAnimation.joints[0].thrustOffset;
        //var swayOscillation   = Math.sin(toRadians((cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[0].swayPhase)) + currentAnimation.joints[0].swayOffset;
        //var bobOscillation    = Math.sin(toRadians((cumulativeTime * adjustedFrequency * 2) + currentAnimation.joints[0].bobPhase)) + currentAnimation.joints[0].bobOffset;

        // calculate hips rotation
        var pitchOscillation = currentAnimation.joints[0].pitch * Math.sin(toRadians((cumulativeTime * adjustedFrequency * 2)
                               + currentAnimation.joints[0].pitchPhase)) + currentAnimation.joints[0].pitchOffset;
        var yawOscillation   = currentAnimation.joints[0].yaw   * Math.sin(toRadians((cumulativeTime * adjustedFrequency )
                               + currentAnimation.joints[0].yawPhase))   + currentAnimation.joints[0].yawOffset;
        var rollOscillation  = (currentAnimation.joints[0].roll  * Math.sin(toRadians((cumulativeTime * adjustedFrequency )
                               + currentAnimation.joints[0].rollPhase))  + currentAnimation.joints[0].rollOffset);

        // apply hips translation TODO: get this working!
        //translateHips({x:swayOscillation*currentAnimation.joints[0].sway, y:motorOscillation*currentAnimation.joints[0].thrust, z:bobOscillation*currentAnimation.joints[0].bob});

        // apply hips rotation
        MyAvatar.setJointData("Hips", Quat.fromPitchYawRollDegrees(-pitchOscillation + (forwardModifier * (leanPitchModifier*getLeanPitch(velocity))),   // getLeanPitch - lean forwards as velocity increased
                                                                    yawOscillation,                                       // Yup, that's correct ;-)
                                                                    rollOscillation + getLeanRoll(deltaTime,velocity))); // getLeanRoll - banking on cornering

        // calculate upper leg rotations

		// TODO: clean up here - increase stride a bit as velocity increases
        var runningModifier = velocity / currentAnimation.settings.takeFlightVelocity;
        if(runningModifier>1) {
			runningModifier *= 2;
			runningModifier += 0.5;
		}
        else if(runningModifier>0) {
			runningModifier *= 2;
			runningModifier += 0.5;
		}
		else runningModifier = 1; // standing

		runningModifier = 1; // TODO - remove this little disabling hack!

        pitchOscillation = runningModifier * currentAnimation.joints[1].pitch * Math.sin(toRadians((cumulativeTime * adjustedFrequency ) + (forwardModifier * currentAnimation.joints[1].pitchPhase)));
        yawOscillation   = currentAnimation.joints[1].yaw   * Math.sin(toRadians((cumulativeTime * adjustedFrequency ) + currentAnimation.joints[1].yawPhase));
        rollOscillation  = currentAnimation.joints[1].roll  * Math.sin(toRadians((cumulativeTime * adjustedFrequency ) + currentAnimation.joints[1].rollPhase));

        // apply upper leg rotations
        var strideAdjusterPitch = 0;
        var strideAdjusterPitchOffset = 0;
        var strideAdjusterSeparationAngle = 0;
        if(INTERNAL_STATE===WALKING ||
           INTERNAL_STATE===CONFIG_WALK_STYLES ||
           INTERNAL_STATE===CONFIG_WALK_TWEAKS ||
           INTERNAL_STATE===CONFIG_WALK_JOINTS) {
            strideAdjusterPitch = currentAnimation.adjusters.stride.strength*currentAnimation.adjusters.stride.upperLegsPitch;
            strideAdjusterPitchOffset = currentAnimation.adjusters.stride.strength*currentAnimation.adjusters.stride.upperLegsPitchOffset;
            strideAdjusterSeparationAngle = currentAnimation.adjusters.legsSeparation.strength * currentAnimation.adjusters.legsSeparation.separationAngle;
        }
        MyAvatar.setJointData("RightUpLeg", Quat.fromPitchYawRollDegrees(
            pitchOscillation + currentAnimation.joints[1].pitchOffset + strideAdjusterPitch * (Math.sin(toRadians((cumulativeTime * adjustedFrequency) + currentAnimation.joints[1].pitchPhase)) + strideAdjusterPitchOffset),
            yawOscillation + currentAnimation.joints[1].yawOffset,
           -rollOscillation - strideAdjusterSeparationAngle - currentAnimation.joints[1].rollOffset ));
        MyAvatar.setJointData("LeftUpLeg",  Quat.fromPitchYawRollDegrees(
           - pitchOscillation + currentAnimation.joints[1].pitchOffset - strideAdjusterPitch * (Math.sin(toRadians((cumulativeTime * adjustedFrequency) + currentAnimation.joints[1].pitchPhase)) - strideAdjusterPitchOffset),
            yawOscillation - currentAnimation.joints[1].yawOffset,
           -rollOscillation + strideAdjusterSeparationAngle + currentAnimation.joints[1].rollOffset ));

        // calculate lower leg joint rotations
        strideAdjusterPitch = 0;
        strideAdjusterPitchOffset = 0;
        if(INTERNAL_STATE===WALKING ||
           INTERNAL_STATE===CONFIG_WALK_STYLES ||
           INTERNAL_STATE===CONFIG_WALK_TWEAKS ||
           INTERNAL_STATE===CONFIG_WALK_JOINTS) {
            strideAdjusterPitch = currentAnimation.adjusters.stride.strength * currentAnimation.adjusters.stride.lowerLegsPitch;
            strideAdjusterPitchOffset = currentAnimation.adjusters.stride.strength*currentAnimation.adjusters.stride.lowerLegsPitchOffset;
        }
        pitchOscillation = currentAnimation.joints[2].pitch * Math.sin(toRadians((cumulativeTime * adjustedFrequency ) + currentAnimation.joints[2].pitchPhase));
        yawOscillation   = currentAnimation.joints[2].yaw   * Math.sin(toRadians((cumulativeTime * adjustedFrequency ) + currentAnimation.joints[2].yawPhase));
        rollOscillation  = currentAnimation.joints[2].roll  * Math.sin(toRadians((cumulativeTime * adjustedFrequency ) + currentAnimation.joints[2].rollPhase));

        // apply lower leg joint rotations
        MyAvatar.setJointData("RightLeg", Quat.fromPitchYawRollDegrees(
            -pitchOscillation + currentAnimation.joints[2].pitchOffset - strideAdjusterPitch * (Math.sin(toRadians((cumulativeTime * adjustedFrequency) + currentAnimation.joints[2].pitchPhase)) + strideAdjusterPitchOffset),
             yawOscillation - currentAnimation.joints[2].yawOffset,
             rollOscillation - currentAnimation.joints[2].rollOffset)); // TODO: needs a kick just before fwd peak
        MyAvatar.setJointData("LeftLeg",  Quat.fromPitchYawRollDegrees(
             pitchOscillation + currentAnimation.joints[2].pitchOffset + strideAdjusterPitch * (Math.sin(toRadians((cumulativeTime * adjustedFrequency) + currentAnimation.joints[2].pitchPhase)) - strideAdjusterPitchOffset),
             yawOscillation + currentAnimation.joints[2].yawOffset,
             rollOscillation + currentAnimation.joints[2].rollOffset));

        // foot joint oscillation is a hard curve to replicate
        var wave = 1;//(baseOscillation + 1)/2; // TODO: finish this - +ve num between 0 and 1 gives a kick at the forward part of the swing
        pitchOscillation = wave * currentAnimation.joints[3].pitch * Math.sin(toRadians((cumulativeTime * adjustedFrequency ) + currentAnimation.joints[3].pitchPhase));
        yawOscillation   = currentAnimation.joints[3].yaw  * Math.sin(toRadians((cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[3].yawPhase));
        rollOscillation  = currentAnimation.joints[3].roll * Math.sin(toRadians((cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[3].rollPhase));
        MyAvatar.setJointData("RightFoot", Quat.fromPitchYawRollDegrees( pitchOscillation + currentAnimation.joints[3].pitchOffset, yawOscillation + currentAnimation.joints[3].yawOffset, rollOscillation + currentAnimation.joints[3].rollOffset));
        MyAvatar.setJointData("LeftFoot",  Quat.fromPitchYawRollDegrees(-pitchOscillation + currentAnimation.joints[3].pitchOffset, yawOscillation - currentAnimation.joints[3].yawOffset, rollOscillation - currentAnimation.joints[3].rollOffset));

        if(INTERNAL_STATE===WALKING ||
           INTERNAL_STATE===CONFIG_WALK_STYLES ||
           INTERNAL_STATE===CONFIG_WALK_TWEAKS ||
           INTERNAL_STATE===CONFIG_WALK_JOINTS) {
            // play footfall sound yet? To determine this, we take the differential of the foot's pitch curve to decide
            // when the foot hits the ground. As luck would have it, we're using a sine wave, so finding dy/dx is as
            // simple as determining the cosine wave for the foot's pitch function...
            var feetPitchDifferential = Math.cos(toRadians((cumulativeTime * adjustedFrequency ) + currentAnimation.joints[3].pitchPhase));
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

        // toes joint oscillation
        pitchOscillation = currentAnimation.joints[4].pitch * Math.sin(toRadians((cumulativeTime * adjustedFrequency) + currentAnimation.joints[4].pitchPhase));
        yawOscillation   = currentAnimation.joints[4].yaw   * Math.sin(toRadians((cumulativeTime * adjustedFrequency) + currentAnimation.joints[4].yawPhase)) + currentAnimation.joints[4].yawOffset;
        rollOscillation  = currentAnimation.joints[4].roll  * Math.sin(toRadians((cumulativeTime * adjustedFrequency) + currentAnimation.joints[4].rollPhase)) + currentAnimation.joints[4].rollOffset;
        MyAvatar.setJointData("RightToeBase", Quat.fromPitchYawRollDegrees(-pitchOscillation + currentAnimation.joints[4].pitchOffset, yawOscillation, rollOscillation));
        MyAvatar.setJointData("LeftToeBase",  Quat.fromPitchYawRollDegrees( pitchOscillation + currentAnimation.joints[4].pitchOffset, yawOscillation, rollOscillation));

		// calculate spine joint rotations
		pitchOscillation = currentAnimation.joints[5].pitch * Math.sin(toRadians(( cumulativeTime * adjustedFrequency * 2) + currentAnimation.joints[5].pitchPhase)) + currentAnimation.joints[5].pitchOffset;
		yawOscillation   = currentAnimation.joints[5].yaw   * Math.sin(toRadians(( cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[5].yawPhase)) + currentAnimation.joints[5].yawOffset;
		rollOscillation  = currentAnimation.joints[5].roll  * Math.sin(toRadians(( cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[5].rollPhase)) + currentAnimation.joints[5].rollOffset;

		// apply spine joint rotations
		MyAvatar.setJointData("Spine", Quat.fromPitchYawRollDegrees(-pitchOscillation, yawOscillation, rollOscillation));

		// calcualte spine 1 rotatations
		pitchOscillation = currentAnimation.joints[6].pitch * Math.sin(toRadians(( cumulativeTime * adjustedFrequency * 2) + currentAnimation.joints[6].pitchPhase)) + currentAnimation.joints[6].pitchOffset;
		yawOscillation   = currentAnimation.joints[6].yaw   * Math.sin(toRadians(( cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[6].yawPhase))   + currentAnimation.joints[6].yawOffset;
		rollOscillation  = currentAnimation.joints[6].roll  * Math.sin(toRadians(( cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[6].rollPhase))  + currentAnimation.joints[6].rollOffset;

		// apply spine1 joint rotations
		MyAvatar.setJointData("Spine1", Quat.fromPitchYawRollDegrees( pitchOscillation, -yawOscillation, rollOscillation));

		// spine 2
		pitchOscillation = currentAnimation.joints[7].pitch * Math.sin(toRadians(( cumulativeTime * adjustedFrequency * 2) + currentAnimation.joints[7].pitchPhase)) + currentAnimation.joints[7].pitchOffset;
		yawOscillation   = currentAnimation.joints[7].yaw   * Math.sin(toRadians(( cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[7].yawPhase))   + currentAnimation.joints[7].yawOffset;
		rollOscillation  = currentAnimation.joints[7].roll  * Math.sin(toRadians(( cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[7].rollPhase))  + currentAnimation.joints[7].rollOffset;

		// apply spine2 joint rotations
		MyAvatar.setJointData("Spine2", Quat.fromPitchYawRollDegrees(-pitchOscillation, yawOscillation, -rollOscillation));

		if(!armsFree) {

			// shoulders
			pitchOscillation = currentAnimation.joints[8].pitch * Math.sin(toRadians(( cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[8].pitchPhase)) + currentAnimation.joints[8].pitchOffset;
			yawOscillation   = currentAnimation.joints[8].yaw   * Math.sin(toRadians(( cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[8].yawPhase));
			rollOscillation  = currentAnimation.joints[8].roll  * Math.sin(toRadians(( cumulativeTime * adjustedFrequency * 2) + currentAnimation.joints[8].rollPhase))  + currentAnimation.joints[8].rollOffset;
			MyAvatar.setJointData("RightShoulder", Quat.fromPitchYawRollDegrees(pitchOscillation, yawOscillation + currentAnimation.joints[8].yawOffset,  rollOscillation ));
			MyAvatar.setJointData("LeftShoulder",  Quat.fromPitchYawRollDegrees(pitchOscillation, yawOscillation - currentAnimation.joints[8].yawOffset, -rollOscillation ));

			// upper arms
			pitchOscillation = currentAnimation.joints[9].pitch * Math.sin(toRadians(( cumulativeTime * adjustedFrequency  ) + currentAnimation.joints[9].pitchPhase)) + currentAnimation.joints[9].pitchOffset;
			yawOscillation   = currentAnimation.joints[9].yaw * Math.sin(toRadians(( cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[9].yawPhase));
			rollOscillation  = currentAnimation.joints[9].roll * Math.sin(toRadians(( cumulativeTime * adjustedFrequency * 2) + currentAnimation.joints[9].rollPhase))   + currentAnimation.joints[9].rollOffset;
			MyAvatar.setJointData("RightArm", Quat.fromPitchYawRollDegrees( pitchOscillation,  yawOscillation - currentAnimation.joints[9].yawOffset,  rollOscillation ));
			MyAvatar.setJointData("LeftArm",  Quat.fromPitchYawRollDegrees( pitchOscillation,  yawOscillation + currentAnimation.joints[9].yawOffset, -rollOscillation ));

			// forearms
			pitchOscillation = currentAnimation.joints[10].pitch * Math.sin(toRadians(( cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[10].pitchPhase)) + currentAnimation.joints[10].pitchOffset;
			yawOscillation   = currentAnimation.joints[10].yaw   * Math.sin(toRadians(( cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[10].yawPhase));
			rollOscillation  = currentAnimation.joints[10].roll  * Math.sin(toRadians(( cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[10].rollPhase));
			MyAvatar.setJointData("RightForeArm", Quat.fromPitchYawRollDegrees( pitchOscillation,  yawOscillation + currentAnimation.joints[10].yawOffset,  rollOscillation + currentAnimation.joints[10].rollOffset ));
			MyAvatar.setJointData("LeftForeArm",  Quat.fromPitchYawRollDegrees( pitchOscillation,  yawOscillation - currentAnimation.joints[10].yawOffset,  rollOscillation - currentAnimation.joints[10].rollOffset ));

			// hands
			pitchOscillation = currentAnimation.joints[11].pitch * Math.sin(toRadians(( cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[11].pitchPhase)) + currentAnimation.joints[11].pitchOffset;
			yawOscillation   = currentAnimation.joints[11].yaw   * Math.sin(toRadians(( cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[11].yawPhase))   + currentAnimation.joints[11].yawOffset;
			rollOscillation  = currentAnimation.joints[11].roll  * Math.sin(toRadians(( cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[11].rollPhase))  ;
			MyAvatar.setJointData("RightHand", Quat.fromPitchYawRollDegrees( pitchOscillation,  yawOscillation,  rollOscillation + currentAnimation.joints[11].rollOffset));
			MyAvatar.setJointData("LeftHand",  Quat.fromPitchYawRollDegrees( pitchOscillation, -yawOscillation,  rollOscillation - currentAnimation.joints[11].rollOffset));

		} // if(!armsFree)

        // head
        pitchOscillation = 0.5 * currentAnimation.joints[12].pitch * Math.sin(toRadians(( cumulativeTime * adjustedFrequency * 2) + currentAnimation.joints[12].pitchPhase)) + currentAnimation.joints[12].pitchOffset;
        yawOscillation   = 0.5 * currentAnimation.joints[12].yaw   * Math.sin(toRadians(( cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[12].yawPhase))   + currentAnimation.joints[12].yawOffset;
        rollOscillation  = 0.5 * currentAnimation.joints[12].roll  * Math.sin(toRadians(( cumulativeTime * adjustedFrequency    ) + currentAnimation.joints[12].rollPhase))  + currentAnimation.joints[12].rollOffset;
        MyAvatar.setJointData("Head", Quat.fromPitchYawRollDegrees( pitchOscillation, yawOscillation, rollOscillation));
        MyAvatar.setJointData("Neck", Quat.fromPitchYawRollDegrees( pitchOscillation, yawOscillation, rollOscillation));
}



// getBezier from: http://13thparallel.com/archive/bezier-curves/
//====================================\\
// 13thParallel.org Beziér Curve Code \\
//   by Dan Pupius (www.pupius.net)   \\
//====================================\\
/*
coord = function (x,y) {
	if(!x) var x=0;
	if(!y) var y=0;
	return {x: x, y: y};
}

function B1(t) { return t*t*t }
function B2(t) { return 3*t*t*(1-t) }
function B3(t) { return 3*t*(1-t)*(1-t) }
function B4(t) { return (1-t)*(1-t)*(1-t) }

function getBezier(percent,C1,C2,C3,C4) {
	var pos = new coord();
	pos.x = C1.x*B1(percent) + C2.x*B2(percent) + C3.x*B3(percent) + C4.x*B4(percent);
	pos.y = C1.y*B1(percent) + C2.y*B2(percent) + C3.y*B3(percent) + C4.y*B4(percent);
	return pos;
}
*/


// the faster we go, the further we lean forward. the angle is calcualted here
var leanAngles = [0,0,0,0,0,0,0,0,0,0]; // smooth out and add damping with simple averaging filter. 20 tap = too much damping, 10 pretty good
function getLeanPitch(velocity) {

	if(velocity>TERMINAL_VELOCITY) velocity=TERMINAL_VELOCITY;

	var leanAngle = velocity / TERMINAL_VELOCITY * currentAnimation.settings.flyingHipsPitch;

	// simple averaging filter
    leanAngles.push(leanAngle);
    leanAngles.shift(); // FIFO
    var totalLeanAngles = 0;
    for(ea in leanAngles) totalLeanAngles += leanAngles[ea];
    var finalLeanAngle = totalLeanAngles / leanAngles.length;

    //print('final lean angle '+finalLeanAngle);  // we found that native support already follows a curve - see graph in forum post

    return finalLeanAngle;

	// work in progress - apply bezier curve to lean / velocity response

	/*var percentTotalLean = velocity / TERMINAL_VELOCITY;

	 no curve applied - used for checking results only
	var linearLeanAngle = percentTotalLean * currentAnimation.settings.flyingHipsPitch;

	// make the hips pitch  /  velocity curve follow a nice bezier
    // bezier control points
	Q1 = coord(0,0);
	Q2 = coord(0.2,0.8);
	Q3 = coord(0.8,0.2);
	Q4 = coord(1,1);

	var easedLean = getBezier(percentTotalLean, Q1, Q2, Q3, Q4);
    var leanAngle = (1-easedLean.x) * currentAnimation.settings.flyingHipsPitch;
    //print('before bezier: '+linearLeanAngle.toFixed(4)+' after bezier '+leanAngle.toFixed(4));

	// simple averaging filter
    leanAngles.push(leanAngle);
    leanAngles.shift(); // FIFO
    var totalLeanAngles = 0;
    for(ea in leanAngles) totalLeanAngles += leanAngles[ea];
    var finalLeanAngle = totalLeanAngles / leanAngles.length;

    print('before bezier: '+linearLeanAngle.toFixed(4));//+' after bezier '+leanAngle.toFixed(4));

	return finalLeanAngle;
	*/
}

// calculate the angle at which to bank into corners when turning
var angularVelocities = [0,0,0,0,0,0,0,0,0,0];  // smooth out and add damping with simple averaging filter
function getLeanRoll(deltaTime,velocity) {

	var angularVelocityMax = 70;
	var currentOrientationVec3 = Quat.safeEulerAngles(MyAvatar.orientation);
    var lastOrientationVec3 = Quat.safeEulerAngles(lastOrientation);
    var deltaYaw = lastOrientationVec3.y-currentOrientationVec3.y;
    var angularVelocity = deltaYaw / deltaTime;
	if(angularVelocity>70) angularVelocity = angularVelocityMax;
    if(angularVelocity<-70) angularVelocity = -angularVelocityMax;
    angularVelocities.push(angularVelocity);
    angularVelocities.shift(); // FIFO
    var totalAngularVelocities = 0;
    for(ea in angularVelocities) totalAngularVelocities += angularVelocities[ea];
    var averageAngularVelocity = totalAngularVelocities / angularVelocities.length;
    var velocityAdjuster = Math.sqrt(velocity/TERMINAL_VELOCITY); // put a little curvature on our otherwise linear velocity modifier
    if(velocityAdjuster>1) velocityAdjuster = 1;
    if(velocityAdjuster<0) velocityAdjuster = 0;
    var leanRoll = velocityAdjuster * (averageAngularVelocity/angularVelocityMax) * currentAnimation.settings.maxBankingAngle;
    //print('delta time is '+deltaTime.toFixed(4)+' and delta yaw is '+deltaYaw+' angular velocity is '+angularVelocity+' and average angular velocity is '+averageAngularVelocity+' and velocityAdjuster is '+velocityAdjuster+' and final value is '+leanRoll);
    //print('array: '+angularVelocities.toString());
    lastOrientation = MyAvatar.orientation;
    return leanRoll;
}

// sets up the interface componenets and updates the internal state
function setInternalState(newInternalState) {

    switch(newInternalState) {

        case WALKING:
        	print('WALKING');
            if(!minimised) doStandardMenu();
            INTERNAL_STATE = WALKING;
            currentAnimation = selectedWalk;
            break;

        case FLYING:
        	print('FLYING');
            if(!minimised) doStandardMenu();
            INTERNAL_STATE = FLYING;
            currentAnimation = selectedFly;
            break;

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
				setSliderthumbsVisible(false);
			}
            break;

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
            break;

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
				Overlays.editOverlay(hipsJointControl, { bounds: { x: backgroundX+75, y: backgroundY+92, width: 200, height: 300}} );
				initialiseWalkJointsPanel(selectedJointIndex);
			}
            break;

        case CONFIG_STANDING:
            INTERNAL_STATE = CONFIG_STANDING;
            currentAnimation = selectedStand;
            if(!minimised) {
				hidebuttonOverlays();
				hideJointControls();
				doStandardMenu();
				showFrontPanelButtons(false);
				showWalkStyleButtons(false);
				setBackground(controlsBackgroundWalkEditJoints);
				setButtonOverlayVisible(configStandButtonSelected);
				Overlays.editOverlay(hipsJointControl, { bounds: { x: backgroundX+75, y: backgroundY+92, width: 200, height: 300}} );
				initialiseWalkJointsPanel(selectedJointIndex);
			}
            break;

        case CONFIG_FLYING:
            INTERNAL_STATE = CONFIG_FLYING;
            currentAnimation = selectedFly;
            if(!minimised) {
				hidebuttonOverlays();
				hideJointControls();
				doStandardMenu();
				showFrontPanelButtons(false);
				showWalkStyleButtons(false);
				setBackground(controlsBackgroundWalkEditJoints);
				setButtonOverlayVisible(configFlyingButtonSelected);
				Overlays.editOverlay(hipsJointControl, { bounds: { x: backgroundX+75, y: backgroundY+92, width: 200, height: 300}} );
				initialiseWalkJointsPanel(selectedJointIndex);
			}
            break;

        case STANDING:
        default:
        	print('STANDING');
            INTERNAL_STATE = STANDING;
            if(!minimised) doStandardMenu();
            currentAnimation = selectedStand;
            break;
    }
}

// Main loop

// stabilising vars -  most state changes are preceded by a couple of hints that they are about to happen
// rather than momentarilly switching between states (causes flicker), we count the number of hints in a row before
// actually changing state - a system very similar to switch debouncing in electronics design
var standHints = 0;
var walkHints = 0;
var flyHints = 0;
var requiredHints = 3; // debounce state changes - how many times do we get a state change request before we actually change state?
var lastDirection = DIRECTION_FORWARDS;

Script.update.connect(function(deltaTime) {

	if(powerOn) {

        cumulativeTime += deltaTime;

        // firstly test for user configuration states
        switch(INTERNAL_STATE) {

            case CONFIG_WALK_STYLES:
	            currentAnimation = selectedWalk;
                animateAvatar(deltaTime, 0, DIRECTION_FORWARDS);
                return;
            case CONFIG_WALK_TWEAKS:
                currentAnimation = selectedWalk;
                animateAvatar(deltaTime, 0, DIRECTION_FORWARDS);
                return;
            case CONFIG_WALK_JOINTS:
                currentAnimation = selectedWalk;
                animateAvatar(deltaTime, 0, DIRECTION_FORWARDS);
                return;
            case CONFIG_STANDING:
                currentAnimation = selectedStand;
                animateAvatar(deltaTime, 0, DIRECTION_FORWARDS);
                return;
            case CONFIG_FLYING:
                currentAnimation = selectedFly;
                animateAvatar(deltaTime, 0, DIRECTION_FORWARDS);
                return;
            default:
                break;
        }

        // calcualte (local) change in position and velocity
        var velocityVector = MyAvatar.getVelocity();
        var velocity = Vec3.length(velocityVector);

        // determine the candidate animation to play
        var actionToTake = 0;
        if( velocity < 0.1) {
            actionToTake = STANDING;
            standHints++;
        }
        else if(velocity<FLYING_SPEED) {
            actionToTake = WALKING;
            walkHints++;
        }
        else if(velocity>=FLYING_SPEED) {
            actionToTake = FLYING;
            flyHints++;
        }

        // calculate overriding (local) direction of translation for use later when decide which animation should be played
        var principleDirection = 0;
        var localVelocity = globalToLocal(velocityVector);
        var deltaX = localVelocity.x;
        var deltaY = -localVelocity.y;
        var deltaZ = -localVelocity.z;

		// TODO: find out why there is a reported high up / down velocity as we near walking -> standing...
        var directionChangeThreshold = 0.3; // this little hack makes it a bit better, but at the cost of delayed updated chagne in direction etc :-(
        if(velocity<directionChangeThreshold)
		   		principleDirection = lastDirection;
		else {
			// who's the biggest out of the x, y and z components taken from the difference vector?
			if(Math.abs(deltaX)>Math.abs(deltaY)
			 &&Math.abs(deltaX)>Math.abs(deltaZ)) {
				if(deltaX<0) {
					principleDirection = DIRECTION_RIGHT;//print('velocity = '+velocity + ' DIRECTION_RIGHT: deltaX '+deltaX+'  deltaY '+deltaY+'  deltaZ '+deltaZ);
				} else {
					principleDirection = DIRECTION_LEFT;//print('velocity = '+velocity + ' DIRECTION_LEFT: deltaX '+deltaX+'  deltaY '+deltaY+'  deltaZ '+deltaZ);
				}
			}
			else if(Math.abs(deltaY)>Math.abs(deltaX)
				  &&Math.abs(deltaY)>Math.abs(deltaZ)) {
				if(deltaY>0) {
					principleDirection = DIRECTION_DOWN;//print('velocity = '+velocity + ' DIRECTION_DOWN: deltaX '+deltaX+'  deltaY '+deltaY+'  deltaZ '+deltaZ);
				}
				else {
					principleDirection = DIRECTION_UP;//print('velocity = '+velocity + ' DIRECTION_UP: deltaX '+deltaX+'  deltaY '+deltaY+'  deltaZ '+deltaZ);
				}
			}
			else if(Math.abs(deltaZ)>Math.abs(deltaX)
				  &&Math.abs(deltaZ)>Math.abs(deltaY)) {
				if(deltaZ>0) {
					principleDirection = DIRECTION_BACKWARDS;//print('velocity = '+velocity + ' DIRECTION_BACKWARDS: deltaX '+deltaX+'  deltaY '+deltaY+'  deltaZ '+deltaZ);
				} else {
					principleDirection = DIRECTION_FORWARDS;//print('velocity = '+velocity + ' DIRECTION_FORWARDS: deltaX '+deltaX+'  deltaY '+deltaY+'  deltaZ '+deltaZ);
				}
			}
		}
        lastDirection = principleDirection;

        // select appropriate animation
        switch(actionToTake) {

            case STANDING:
                if( standHints > requiredHints || INTERNAL_STATE===STANDING) { // wait for a few consecutive hints (17mS each)

                    standHints = 0;
                    walkHints = 0;
                    flyHints = 0;
					if(INTERNAL_STATE!==STANDING) setInternalState(STANDING);
					currentAnimation = selectedStand;
                    animateAvatar(1,0,principleDirection);
                }
                return;

            case WALKING:
                if( walkHints > requiredHints || INTERNAL_STATE===WALKING) { // wait for few consecutive hints (17mS each)

                    standHints = 0;
                    walkHints = 0;
                    flyHints = 0;
                    if(INTERNAL_STATE!==WALKING) setInternalState(WALKING);

					// change animation for flying directly up or down
					if(principleDirection===DIRECTION_UP) {
						currentAnimation = selectedFlyUp;
					}
					else if(principleDirection===DIRECTION_DOWN) {
						currentAnimation = selectedFlyDown;
					}
                    else {
						currentAnimation = selectedWalk;
					}
                    animateAvatar(deltaTime, velocity, principleDirection);
                }
                return;

            case FLYING:
                if( flyHints > requiredHints - 1  || INTERNAL_STATE===FLYING ) { // wait for a few consecutive hints (17mS each)

                    standHints = 0;
                    walkHints = 0;
                    flyHints = 0;
					if(INTERNAL_STATE!==FLYING) setInternalState(FLYING);

					// change animation for flying directly up or down
					if(principleDirection===DIRECTION_UP) {
						currentAnimation = selectedFlyUp;
					}
					else if(principleDirection===DIRECTION_DOWN) {
						currentAnimation = selectedFlyDown;
					}
                    else currentAnimation = selectedFly;
                    animateAvatar(deltaTime, velocity, principleDirection);
                }
                return;
        }
    }
});


// overlays start

//  controller dimensions
var backgroundWidth = 350;
var backgroundHeight = 700;
var backgroundX = Window.innerWidth-backgroundWidth-50;
var backgroundY = Window.innerHeight/2 - backgroundHeight/2;
var minSliderX = backgroundX + 30;
var maxSliderX = backgroundX + 295;
var sliderRangeX = 295 - 30;
var jointsControlWidth = 200;
var jointsControlHeight = 300;
var jointsControlX = backgroundX/2 - jointsControlWidth/2;
var jointsControlY = backgroundY/2 - jointsControlHeight/2;
var buttonsY = 20;  // distance from top of panel to buttons

// arrays of overlay names
var sliderthumbOverlays = []; // thumb sliders
var backgroundOverlays = [];
var buttonOverlays = [];
var jointsControlOverlays = [];
var bigButtonOverlays = [];


// take a deep breath then load up the overlays
// UI backgrounds
var controlsBackground = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX, y: backgroundY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
backgroundOverlays.push(controlsBackground);

var controlsBackgroundWalkEditStyles = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX, y: backgroundY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background-edit-styles.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
backgroundOverlays.push(controlsBackgroundWalkEditStyles);

var controlsBackgroundWalkEditTweaks = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX, y: backgroundY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background-edit-tweaks.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
backgroundOverlays.push(controlsBackgroundWalkEditTweaks);

var controlsBackgroundWalkEditJoints = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX, y: backgroundY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background-edit-joints.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
backgroundOverlays.push(controlsBackgroundWalkEditJoints);

var controlsBackgroundFlyingEdit = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX, y: backgroundY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background-flying-edit.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
backgroundOverlays.push(controlsBackgroundFlyingEdit);


// minimised tab - not put in array, as is a one off
var controlsMinimisedTab = Overlays.addOverlay("image", {
                    bounds: { x: Window.innerWidth - 35, y: Window.innerHeight/2 - 175, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-tab.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });

// load character joint selection control images
var hipsJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background-edit-hips.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
jointsControlOverlays.push(hipsJointControl);

var upperLegsJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background-edit-upper-legs.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
jointsControlOverlays.push(upperLegsJointControl);

var lowerLegsJointControl  = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background-edit-lower-legs.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
jointsControlOverlays.push(lowerLegsJointControl);

var feetJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background-edit-feet.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
jointsControlOverlays.push(feetJointControl);

var toesJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background-edit-toes.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
jointsControlOverlays.push(toesJointControl);

var spineJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background-edit-spine.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
jointsControlOverlays.push(spineJointControl);

var spine1JointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background-edit-spine1.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
jointsControlOverlays.push(spine1JointControl);

var spine2JointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background-edit-spine2.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
jointsControlOverlays.push(spine2JointControl);

var shouldersJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background-edit-shoulders.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
jointsControlOverlays.push(shouldersJointControl);

var upperArmsJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background-edit-upper-arms.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
jointsControlOverlays.push(upperArmsJointControl);

var forearmsJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background-edit-forearms.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
jointsControlOverlays.push(forearmsJointControl);

var handsJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background-edit-hands.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
jointsControlOverlays.push(handsJointControl);

var headJointControl = Overlays.addOverlay("image", {
                    bounds: { x: jointsControlX, y: jointsControlY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-background-edit-head.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
jointsControlOverlays.push(headJointControl);


// sider thumb overlays
var sliderOne = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
sliderthumbOverlays.push(sliderOne);
var sliderTwo = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
sliderthumbOverlays.push(sliderTwo);
var sliderThree = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
sliderthumbOverlays.push(sliderThree);
var sliderFour = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
sliderthumbOverlays.push(sliderFour);
var sliderFive = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
sliderthumbOverlays.push(sliderFive);
var sliderSix = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
sliderthumbOverlays.push(sliderSix);
var sliderSeven = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
sliderthumbOverlays.push(sliderSeven);
var sliderEight = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
sliderthumbOverlays.push(sliderEight);
var sliderNine = Overlays.addOverlay("image", {
                    bounds: { x: 0, y: 0, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
sliderthumbOverlays.push(sliderNine);


// button overlays
var onButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+20, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-on-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(onButton);
var offButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+20, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-off-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(offButton);
var configWalkButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+83, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-edit-walk-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(configWalkButton);
var configWalkButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+83, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-edit-walk-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(configWalkButtonSelected);
var configStandButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+146, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-edit-stand-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(configStandButton);
var configStandButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+146, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-edit-stand-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(configStandButtonSelected);
var configFlyingButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+209, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-edit-fly-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(configFlyingButton);
var configFlyingButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+209, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-edit-fly-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(configFlyingButtonSelected);
var hideButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+272, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-hide-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(hideButton);
var hideButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+272, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-hide-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(hideButtonSelected);
var configWalkStylesButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+83, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-styles-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(configWalkStylesButton);
var configWalkStylesButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+83, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-styles-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(configWalkStylesButtonSelected);
var configWalkTweaksButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+146, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-tweaks-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(configWalkTweaksButton);
var configWalkTweaksButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+146, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-tweaks-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(configWalkTweaksButtonSelected);
var configWalkJointsButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+209, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-bones-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(configWalkJointsButton);
var configWalkJointsButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+209, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-bones-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(configWalkJointsButtonSelected);
var backButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+272, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-back-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(backButton);
var backButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX+272, y: backgroundY+buttonsY, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-back-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
buttonOverlays.push(backButtonSelected);

// big button overlays - front panel
var bigButtonYOffset = 408;  //  distance from top of panel to top of first button

var femaleBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-female-big-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(femaleBigButton);

var femaleBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 230, height: 36},
                    imageURL: pathToOverlays+"ddao-female-big-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(femaleBigButtonSelected);

var maleBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset + 60, width: 230, height: 36},
                    imageURL: pathToOverlays+"ddao-male-big-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(maleBigButton);

var maleBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset + 60, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-male-big-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(maleBigButtonSelected);

var armsFreeBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset + 120, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-arms-free-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(armsFreeBigButton);

var armsFreeBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset + 120, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-arms-free-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(armsFreeBigButtonSelected);

var footstepsBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset + 180, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-footsteps-big-button.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(footstepsBigButton);

var footstepsBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset + 180, width: 230, height: 36},
                    imageURL: pathToOverlays+"ddao-footsteps-big-button-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(footstepsBigButtonSelected);


// walk styles
bigButtonYOffset = 121;
var strutWalkBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-strut.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(strutWalkBigButton);

var strutWalkBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-strut-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(strutWalkBigButtonSelected);

bigButtonYOffset += 60
var sexyWalkBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-sexy.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(sexyWalkBigButton);

var sexyWalkBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-sexy-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(sexyWalkBigButtonSelected);

bigButtonYOffset += 60;
var powerWalkBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-power-walk.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(powerWalkBigButton);

var powerWalkBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-power-walk-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(powerWalkBigButtonSelected);

bigButtonYOffset += 60;
var shuffleBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-shuffle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(shuffleBigButton);

var shuffleBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-shuffle-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(shuffleBigButtonSelected);

bigButtonYOffset += 60;
var runBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-run.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(runBigButton);

var runBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-run-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(runBigButtonSelected);

bigButtonYOffset += 60;
var sneakyWalkBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-sneaky.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(sneakyWalkBigButton);

var sneakyWalkBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-sneaky-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(sneakyWalkBigButtonSelected);

bigButtonYOffset += 60;
var toughWalkBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-tough.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(toughWalkBigButton);

var toughWalkBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-tough-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(toughWalkBigButtonSelected);

bigButtonYOffset += 60;
var coolWalkBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-cool.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(coolWalkBigButton);

var coolWalkBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-cool-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(coolWalkBigButtonSelected);

bigButtonYOffset += 60;
var elderlyWalkBigButton = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-elderly.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(elderlyWalkBigButton);

var elderlyWalkBigButtonSelected = Overlays.addOverlay("image", {
                    bounds: { x: backgroundX + backgroundWidth/2 - 115, y: backgroundY + bigButtonYOffset, width: 1, height: 1},
                    imageURL: pathToOverlays+"ddao-walk-select-button-elderly-selected.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
bigButtonOverlays.push(elderlyWalkBigButtonSelected);

// overlays to show the walk wheel stats
var walkWheelZLine = Overlays.addOverlay("line3d", {
                    position: { x: 0, y: 0, z:hipsToFeetDistance },
                    end: { x: 0, y: 0, z: -hipsToFeetDistance },
                    color: { red: 0, green: 255, blue: 255},
                    alpha: 1,
                    lineWidth: 5,
                    visible: false,
                    anchor: "MyAvatar"
                });
var walkWheelYLine = Overlays.addOverlay("line3d", {
                    position: { x: 0, y: hipsToFeetDistance, z:0 },
                    end: { x: 0, y: -hipsToFeetDistance, z:0 },
                    color: { red: 255, green: 0, blue: 255},
                    alpha: 1,
                    lineWidth: 5,
                    visible: false,
                    anchor: "MyAvatar"
                });

var debugText = Overlays.addOverlay("text", {
                    x: Window.innerWidth/2 + 200,
                    y: Window.innerHeight/2 - 300,
                    width: 200,
                    height: 130,
                    color: { red: 255, green: 255, blue: 255},
                    textColor: { red: 255, green: 255, blue: 255},
                    topMargin: 5,
                    leftMargin: 5,
                    visible: false,
                    backgroundColor: { red: 255, green: 255, blue: 255},
                    text: "Debug area\nNothing to report yet."
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
    setSliderthumbsVisible(false);
    showFrontPanelButtons(true);
    showWalkStyleButtons(false);
}
function showFrontPanelButtons(showButtons) {

	var bigButtonWidth = 1;
	var bigButtonHeight = 1;

	if(showButtons) {
		var bigButtonWidth = 230;
		var bigButtonHeight = 36;
	}
	if(avatarGender===FEMALE) {
		Overlays.editOverlay(femaleBigButtonSelected, { width: bigButtonWidth, height: bigButtonHeight } );
		Overlays.editOverlay(femaleBigButton, { width: 1, height: 1 } );
		Overlays.editOverlay(maleBigButtonSelected, { width: 1, height: 1 } );
		Overlays.editOverlay(maleBigButton, { width: bigButtonWidth, height: bigButtonHeight } );
	} else {
		Overlays.editOverlay(femaleBigButtonSelected, { width: 1, height: 1 } );
		Overlays.editOverlay(femaleBigButton, { width: bigButtonWidth, height: bigButtonHeight } );
		Overlays.editOverlay(maleBigButtonSelected, { width: bigButtonWidth, height: bigButtonHeight } );
		Overlays.editOverlay(maleBigButton, { width: 1, height: 1 } );
	}
	if(armsFree) {
		Overlays.editOverlay(armsFreeBigButtonSelected, { width: bigButtonWidth, height: bigButtonHeight } );
		Overlays.editOverlay(armsFreeBigButton, { width: 1, height: 1 } );
	} else {
		Overlays.editOverlay(armsFreeBigButtonSelected, { width: 1, height: 1 } );
		Overlays.editOverlay(armsFreeBigButton, { width: bigButtonWidth, height: bigButtonHeight } );
	}
	if(playFootStepSounds) {
		Overlays.editOverlay(footstepsBigButtonSelected, { width: bigButtonWidth, height: bigButtonHeight } );
		Overlays.editOverlay(footstepsBigButton, { width: 1, height: 1 } );
	} else {
		Overlays.editOverlay(footstepsBigButtonSelected, { width: 1, height: 1 } );
		Overlays.editOverlay(footstepsBigButton, { width: bigButtonWidth, height: bigButtonHeight } );
	}
}
function minimiseDialog() {

	if(minimised) {
        setBackground();
        hidebuttonOverlays();
        setSliderthumbsVisible(false);
        hideJointControls();
        showFrontPanelButtons(false);
        Overlays.editOverlay(controlsMinimisedTab, { width: 36, height: 351 } );
    } else {
        setInternalState(STANDING); // show all the controls again
        Overlays.editOverlay(controlsMinimisedTab, { width: 1, height: 1 } );
    }
}
function setBackground(backgroundName)  {
    for(var i in backgroundOverlays) {
        if(backgroundOverlays[i] === backgroundName)
            Overlays.editOverlay(backgroundName, {width: backgroundWidth, height: backgroundHeight } );
        else Overlays.editOverlay(backgroundOverlays[i], { width: 1, height: 1} );
    }
}
function setButtonOverlayVisible(buttonOverlayName) {
    for(var i in buttonOverlays) {
        if(buttonOverlays[i] === buttonOverlayName) {
            Overlays.editOverlay(buttonOverlayName, { width: 60, height: 47 } );
        }
    }
}
// top row menu type buttons (smaller)
function hidebuttonOverlays()  {
    for(var i in buttonOverlays) {
        Overlays.editOverlay(buttonOverlays[i], { width: 1, height: 1 } );
    }
}
function hideJointControls() {
    for(var i in jointsControlOverlays) {
        Overlays.editOverlay(jointsControlOverlays[i], { width: 1, height: 1 } );
    }
}
function setSliderthumbsVisible(visible) {
    var sliderThumbSize = 0;
    if(visible) sliderThumbSize = 25;
    for(var i = 0 ; i < sliderthumbOverlays.length ; i++) {
        Overlays.editOverlay(sliderthumbOverlays[i], { width: sliderThumbSize, height: sliderThumbSize} );
    }
}
function initialiseWalkJointsPanel(propertyIndex) {

    selectedJointIndex = propertyIndex;

    // set the image for the selected joint on the character control
    hideJointControls();
    switch (selectedJointIndex) {
        case 0:
            Overlays.editOverlay(hipsJointControl, { bounds: { x: backgroundX+75, y: backgroundY+92, width: 200, height: 300}} );
            break;
        case 1:
            Overlays.editOverlay(upperLegsJointControl, { bounds: { x: backgroundX+75, y: backgroundY+92, width: 200, height: 300}} );
            break;
        case 2:
            Overlays.editOverlay(lowerLegsJointControl, { bounds: { x: backgroundX+75, y: backgroundY+92, width: 200, height: 300}} );
            break;
        case 3:
            Overlays.editOverlay(feetJointControl, { bounds: { x: backgroundX+75, y: backgroundY+92, width: 200, height: 300}} );
            break;
        case 4:
            Overlays.editOverlay(toesJointControl, { bounds: { x: backgroundX+75, y: backgroundY+92, width: 200, height: 300}} );
            break;
        case 5:
            Overlays.editOverlay(spineJointControl, { bounds: { x: backgroundX+75, y: backgroundY+92, width: 200, height: 300}} );
            break;
        case 6:
            Overlays.editOverlay(spine1JointControl, { bounds: { x: backgroundX+75, y: backgroundY+92, width: 200, height: 300}} );
            break;
        case 7:
            Overlays.editOverlay(spine2JointControl, { bounds: { x: backgroundX+75, y: backgroundY+92, width: 200, height: 300}} );
            break;
        case 8:
            Overlays.editOverlay(shouldersJointControl, { bounds: { x: backgroundX+75, y: backgroundY+92, width: 200, height: 300}} );
            break;
        case 9:
            Overlays.editOverlay(upperArmsJointControl, { bounds: { x: backgroundX+75, y: backgroundY+92, width: 200, height: 300}} );
            break;
        case 10:
            Overlays.editOverlay(forearmsJointControl, { bounds: { x: backgroundX+75, y: backgroundY+92, width: 200, height: 300}} );
            break;
        case 11:
            Overlays.editOverlay(handsJointControl, { bounds: { x: backgroundX+75, y: backgroundY+92, width: 200, height: 300}} );
            break;
        case 12:
            Overlays.editOverlay(headJointControl, { bounds: { x: backgroundX+75, y: backgroundY+92, width: 200, height: 300}} );
            break;
    }

    // set sliders to adjust individual joint properties
    var i = 0;
    var yLocation = backgroundY+359;

	// pitch your role
    var sliderXPos = currentAnimation.joints[selectedJointIndex].pitch / sliderRanges.joints[selectedJointIndex].pitchRange * sliderRangeX;
    Overlays.editOverlay(sliderthumbOverlays[i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=30, width: 25, height: 25}} );
    sliderXPos = currentAnimation.joints[selectedJointIndex].yaw / sliderRanges.joints[selectedJointIndex].yawRange * sliderRangeX;
    Overlays.editOverlay(sliderthumbOverlays[++i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=30, width: 25, height: 25}} );
    sliderXPos = currentAnimation.joints[selectedJointIndex].roll / sliderRanges.joints[selectedJointIndex].rollRange * sliderRangeX;
    Overlays.editOverlay(sliderthumbOverlays[++i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=30, width: 25, height: 25}} );

    // set phases (full range, -180 to 180)
    sliderXPos = (90 + currentAnimation.joints[selectedJointIndex].pitchPhase/2)/180 * sliderRangeX;
    Overlays.editOverlay(sliderthumbOverlays[++i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=30, width: 25, height: 25}} );
    sliderXPos = (90 + currentAnimation.joints[selectedJointIndex].yawPhase/2)/180 * sliderRangeX;
    Overlays.editOverlay(sliderthumbOverlays[++i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=30, width: 25, height: 25}} );
    sliderXPos = (90 + currentAnimation.joints[selectedJointIndex].rollPhase/2)/180 * sliderRangeX;
    Overlays.editOverlay(sliderthumbOverlays[++i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=30, width: 25, height: 25}} );

    // offset ranges are also -ve thr' zero to +ve, so have to offset
    sliderXPos = (((sliderRanges.joints[selectedJointIndex].pitchOffsetRange+currentAnimation.joints[selectedJointIndex].pitchOffset)/2)/sliderRanges.joints[selectedJointIndex].pitchOffsetRange) * sliderRangeX;
    Overlays.editOverlay(sliderthumbOverlays[++i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=30, width: 25, height: 25}} );
    sliderXPos = (((sliderRanges.joints[selectedJointIndex].yawOffsetRange+currentAnimation.joints[selectedJointIndex].yawOffset)/2)/sliderRanges.joints[selectedJointIndex].yawOffsetRange) * sliderRangeX;
    Overlays.editOverlay(sliderthumbOverlays[++i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=30, width: 25, height: 25}} );
    sliderXPos = (((sliderRanges.joints[selectedJointIndex].rollOffsetRange+currentAnimation.joints[selectedJointIndex].rollOffset)/2)/sliderRanges.joints[selectedJointIndex].rollOffsetRange) * sliderRangeX;
    Overlays.editOverlay(sliderthumbOverlays[++i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=30, width: 25, height: 25}} );
}

function initialiseWalkTweaks() {

    // set sliders to adjust walk properties
    var i = 0;
    var yLocation = backgroundY+71;

    var sliderXPos = currentAnimation.settings.baseFrequency / MAX_WALK_SPEED * sliderRangeX;   // walk speed
    Overlays.editOverlay(sliderthumbOverlays[i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=60, width: 25, height: 25}} );
    sliderXPos = currentAnimation.settings.takeFlightVelocity / 300 * sliderRangeX;             // start flying speed
    Overlays.editOverlay(sliderthumbOverlays[++i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=60, width: 25, height: 25}} );
    sliderXPos = currentAnimation.joints[0].sway / sliderRanges.joints[0].swayRange * sliderRangeX;     // Hips sway
    Overlays.editOverlay(sliderthumbOverlays[++i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=60, width: 25, height: 25}} );
    sliderXPos = currentAnimation.joints[0].bob / sliderRanges.joints[0].bobRange * sliderRangeX;       // Hips bob
    Overlays.editOverlay(sliderthumbOverlays[++i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=60, width: 25, height: 25}} );
    sliderXPos = currentAnimation.joints[0].thrust / sliderRanges.joints[0].thrustRange * sliderRangeX; // Hips thrust
    Overlays.editOverlay(sliderthumbOverlays[++i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=60, width: 25, height: 25}} );
    sliderXPos = (0.5+(currentAnimation.adjusters.legsSeparation.strength/2)) * sliderRangeX;   // legs separation
    Overlays.editOverlay(sliderthumbOverlays[++i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=60, width: 25, height: 25}} );
    sliderXPos = currentAnimation.adjusters.stride.strength * sliderRangeX;                     // stride
    Overlays.editOverlay(sliderthumbOverlays[++i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=60, width: 25, height: 25}} );
    sliderXPos = currentAnimation.joints[9].yaw / sliderRanges.joints[9].yawRange * sliderRangeX; // arms swing - is just upper arms yaw
    Overlays.editOverlay(sliderthumbOverlays[++i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=60, width: 25, height: 25}} );
    sliderXPos = (((sliderRanges.joints[9].pitchOffsetRange-currentAnimation.joints[9].pitchOffset)/2)/sliderRanges.joints[9].pitchOffsetRange) * sliderRangeX; // arms out - is just upper arms pitch offset
    Overlays.editOverlay(sliderthumbOverlays[++i], { bounds: { x: minSliderX + sliderXPos, y: yLocation+=60, width: 25, height: 25}} );
}

function showWalkStyleButtons(showButtons) {

	var bigButtonWidth = 230;
	var bigButtonHeight = 36;

	if(!showButtons) {
		bigButtonWidth = 1;
		bigButtonHeight = 1;
	}

	// set all big buttons to hidden, but skip the first 8, as are for the front panel
	for(var i = 8 ; i < bigButtonOverlays.length ; i++) {
		Overlays.editOverlay(bigButtonOverlays[i], {width: 1, height: 1});
	}

	if(!showButtons) return;

	// set all the non-selected ones to showing
	for(var i = 8 ; i < bigButtonOverlays.length ; i+=2) {
		Overlays.editOverlay(bigButtonOverlays[i], {width: bigButtonWidth, height: bigButtonHeight});
	}

	// set the currently selected one
	if(selectedWalk === femaleSexyWalk || selectedWalk === maleSexyWalk) {
		Overlays.editOverlay(sexyWalkBigButtonSelected, {width: bigButtonWidth, height: bigButtonHeight});
		Overlays.editOverlay(sexyWalkBigButton, {width: 1, height: 1});
	}
	else if(selectedWalk === femaleStrutWalk || selectedWalk === maleStrutWalk) {
		Overlays.editOverlay(strutWalkBigButtonSelected, {width: bigButtonWidth, height: bigButtonHeight});
		Overlays.editOverlay(strutWalkBigButton, {width: 1, height: 1});
	}
	else if(selectedWalk === femalePowerWalk || selectedWalk === malePowerWalk) {
		Overlays.editOverlay(powerWalkBigButtonSelected, {width: bigButtonWidth, height: bigButtonHeight});
		Overlays.editOverlay(powerWalkBigButton, {width: 1, height: 1});
	}
	else if(selectedWalk === femaleShuffle || selectedWalk === maleShuffle) {
		Overlays.editOverlay(shuffleBigButtonSelected, {width: bigButtonWidth, height: bigButtonHeight});
		Overlays.editOverlay(shuffleBigButton, {width: 1, height: 1});
	}
	else if(selectedWalk === femaleRun || selectedWalk === maleRun) {
		Overlays.editOverlay(runBigButtonSelected, {width: bigButtonWidth, height: bigButtonHeight});
		Overlays.editOverlay(runBigButton, {width: 1, height: 1});
	}
	else if(selectedWalk === femaleRandomWalk || selectedWalk === maleRandomWalk) {
		Overlays.editOverlay(sneakyWalkBigButtonSelected, {width: bigButtonWidth, height: bigButtonHeight});
		Overlays.editOverlay(sneakyWalkBigButton, {width: 1, height: 1});
	}
	else if(selectedWalk === femaleToughWalk || selectedWalk === maleToughWalk) {
		Overlays.editOverlay(toughWalkBigButtonSelected, {width: bigButtonWidth, height: bigButtonHeight});
		Overlays.editOverlay(toughWalkBigButton, {width: 1, height: 1});
	}
	else if(selectedWalk === femaleCoolWalk || selectedWalk === maleCoolWalk) {
		Overlays.editOverlay(coolWalkBigButtonSelected, {width: bigButtonWidth, height: bigButtonHeight});
		Overlays.editOverlay(coolWalkBigButton, {width: 1, height: 1});
	}
	else if(selectedWalk === femaleElderlyWalk || selectedWalk === maleElderlyWalk) {
		Overlays.editOverlay(elderlyWalkBigButtonSelected, {width: bigButtonWidth, height: bigButtonHeight});
		Overlays.editOverlay(elderlyWalkBigButton, {width: 1, height: 1});
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
            Overlays.editOverlay(hideButton, { width: 1, height: 1 } );
            Overlays.editOverlay(hideButtonSelected, { width: 60, height: 47 } );
            return;

        case backButton:
            Overlays.editOverlay(backButton, { width: 1, height: 1 } );
            Overlays.editOverlay(backButtonSelected, { width: 60, height: 47 } );
            return;

        case controlsMinimisedTab:
            // TODO: add visual user feedback for tab click
            return;

		case footstepsBigButton:
			playFootStepSounds = true;
			Overlays.editOverlay(footstepsBigButtonSelected, { width: 230, height: 36 } );
			Overlays.editOverlay(footstepsBigButton, { width: 1, height: 1 } );
			return;

		case footstepsBigButtonSelected:
			playFootStepSounds = false;
			Overlays.editOverlay(footstepsBigButton, { width: 230, height: 36 } );
			Overlays.editOverlay(footstepsBigButtonSelected, { width: 1, height: 1 } );
			return;

		case femaleBigButton:
		case maleBigButtonSelected:
			avatarGender = FEMALE;
			selectedWalk = femaleStrutWalk;
			selectedStand = femaleStandOne;
			selectedFlyUp  = femaleFlyingUp;
			selectedFly = femaleFlying;
			selectedFlyDown = femaleFlyingDown;
			Overlays.editOverlay(femaleBigButtonSelected, { width: 230, height: 36 } );
			Overlays.editOverlay(femaleBigButton, { width: 1, height: 1 } );
			Overlays.editOverlay(maleBigButton, { width: 230, height: 36 } );
			Overlays.editOverlay(maleBigButtonSelected, { width: 1, height: 1 } );
			return;

		case armsFreeBigButton:
			armsFree = true;
			Overlays.editOverlay(armsFreeBigButtonSelected, { width: 230, height: 36 } );
			Overlays.editOverlay(armsFreeBigButton, { width: 1, height: 1 } );
			return;

		case armsFreeBigButtonSelected:
			armsFree = false;
			Overlays.editOverlay(armsFreeBigButtonSelected, { width: 1, height: 1 } );
			Overlays.editOverlay(armsFreeBigButton, { width: 230, height: 36 } );
			return;

		case maleBigButton:
		case femaleBigButtonSelected:
			avatarGender = MALE;
			selectedWalk = maleStrutWalk;
			selectedStand = maleStandOne;
			selectedFlyUp  = maleFlyingUp;
			selectedFly = maleFlying;
			selectedFlyDown = maleFlyingDown;
			Overlays.editOverlay(femaleBigButton, { width: 230, height: 36 } );
			Overlays.editOverlay(femaleBigButtonSelected, { width: 1, height: 1 } );
			Overlays.editOverlay(maleBigButtonSelected, { width: 230, height: 36 } );
			Overlays.editOverlay(maleBigButton, { width: 1, height: 1 } );
			return;

		case coolWalkBigButton:
			if(avatarGender===FEMALE) selectedWalk = femaleCoolWalk;
			else selectedWalk = maleCoolWalk;
			currentAnimation = selectedWalk;
			showWalkStyleButtons(true);
			maxFootForward = 0;
			maxFootBackwards = 0;
			break;

		case coolWalkBigButton:
			if(avatarGender===FEMALE) selectedWalk = femaleCoolWalk;
			else selectedWalk = maleCoolWalk;
			currentAnimation = selectedWalk;
			showWalkStyleButtons(true);
			maxFootForward = 0;
			maxFootBackwards = 0;
			break;

		case elderlyWalkBigButton:
			if(avatarGender===FEMALE) selectedWalk = femaleElderlyWalk;
			else selectedWalk = maleElderlyWalk;
			currentAnimation = selectedWalk;
			showWalkStyleButtons(true);
			maxFootForward = 0;
			maxFootBackwards = 0;
			break;

		case powerWalkBigButton:
			if(avatarGender===FEMALE) selectedWalk = femalePowerWalk;
			else selectedWalk = malePowerWalk;
			currentAnimation = selectedWalk;
			showWalkStyleButtons(true);
			maxFootForward = 0;
			maxFootBackwards = 0;
			break;

		case runBigButton:
			if(avatarGender===FEMALE) selectedWalk = femaleRun;
			else selectedWalk = maleRun;
			currentAnimation = selectedWalk;
			showWalkStyleButtons(true);
			maxFootForward = 0;
			maxFootBackwards = 0;
			break;

		case sexyWalkBigButton:
			if(avatarGender===FEMALE) selectedWalk = femaleSexyWalk;
			else selectedWalk = maleSexyWalk;
			currentAnimation = selectedWalk;
			showWalkStyleButtons(true);
			maxFootForward = 0;
			maxFootBackwards = 0;
			break;

		case shuffleBigButton:
			if(avatarGender===FEMALE) selectedWalk = femaleShuffle;
			else selectedWalk = maleShuffle;
			currentAnimation = selectedWalk;
			showWalkStyleButtons(true);
			maxFootForward = 0;
			maxFootBackwards = 0;
			break;

		case sneakyWalkBigButton:
			if(avatarGender===FEMALE) selectedWalk = femaleRandomWalk;
			else selectedWalk = maleRandomWalk;
			currentAnimation = selectedWalk;
			showWalkStyleButtons(true);
			maxFootForward = 0;
			maxFootBackwards = 0;
			break;

		case strutWalkBigButton:
			if(avatarGender===FEMALE) selectedWalk = femaleStrutWalk;
			else selectedWalk = maleStrutWalk;
			currentAnimation = selectedWalk;
			showWalkStyleButtons(true);
			maxFootForward = 0;
			maxFootBackwards = 0;
			break;

		case toughWalkBigButton:
			if(avatarGender===FEMALE) selectedWalk = femaleToughWalk;
			else selectedWalk = maleToughWalk;
			currentAnimation = selectedWalk;
			showWalkStyleButtons(true);
			maxFootForward = 0;
			maxFootBackwards = 0;
			break;

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

    if(INTERNAL_STATE===CONFIG_WALK_JOINTS ||
	   INTERNAL_STATE===CONFIG_STANDING ||
	   INTERNAL_STATE===CONFIG_FLYING) {

        // check for new joint selection and update display accordingly
        var clickX = event.x - backgroundX - 75;
        var clickY = event.y - backgroundY - 92;

        if(clickX>60&&clickX<120&&clickY>123&&clickY<155) {
            initialiseWalkJointsPanel(0);
            return;
        }
        else if(clickX>63&&clickX<132&&clickY>156&&clickY<202) {
            initialiseWalkJointsPanel(1);
            return;
        }
        else if(clickX>58&&clickX<137&&clickY>203&&clickY<250) {
            initialiseWalkJointsPanel(2);
            return;
        }
        else if(clickX>58&&clickX<137&&clickY>250&&clickY<265) {
            initialiseWalkJointsPanel(3);
            return;
        }
        else if(clickX>58&&clickX<137&&clickY>265&&clickY<280) {
            initialiseWalkJointsPanel(4);
            return;
        }
        else if(clickX>78&&clickX<121&&clickY>111&&clickY<128) {
            initialiseWalkJointsPanel(5);
            return;
        }
        else if(clickX>78&&clickX<128&&clickY>89&&clickY<111) {
            initialiseWalkJointsPanel(6);
            return;
        }
        else if(clickX>85&&clickX<118&&clickY>77&&clickY<94) {
            initialiseWalkJointsPanel(7);
            return;
        }
        else if(clickX>64&&clickX<125&&clickY>55&&clickY<77) {
            initialiseWalkJointsPanel(8);
            return;
        }
        else if((clickX>44&&clickX<73&&clickY>71&&clickY<94)
              ||(clickX>125&&clickX<144&&clickY>71&&clickY<94)) {
            initialiseWalkJointsPanel(9);
            return;
        }
        else if((clickX>28&&clickX<57&&clickY>94&&clickY<119)
              ||(clickX>137&&clickX<170&&clickY>97&&clickY<114)) {
            initialiseWalkJointsPanel(10);
            return;
        }
        else if((clickX>18&&clickX<37&&clickY>115&&clickY<136)
              ||(clickX>157&&clickX<182&&clickY>115&&clickY<136)) {
            initialiseWalkJointsPanel(11);
            return;
        }
        else if(clickX>81&&clickX<116&&clickY>12&&clickY<53) {
            initialiseWalkJointsPanel(12);
            return;
        }
    }
}
function mouseMoveEvent(event) {
    // only need deal with slider changes
    if(powerOn) {

        if(INTERNAL_STATE===CONFIG_WALK_JOINTS ||
           INTERNAL_STATE===CONFIG_STANDING ||
           INTERNAL_STATE===CONFIG_FLYING) {

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
                currentAnimation.settings.takeFlightVelocity = thumbPositionNormalised * 300;
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
                currentAnimation.adjusters.legsSeparation.strength = (thumbPositionNormalised-0.5)/2;
            }
            else if(movingSliderSeven) { // stride
                Overlays.editOverlay(sliderSeven, { x: sliderX + minSliderX} );
                currentAnimation.adjusters.stride.strength = thumbPositionNormalised;
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
        Overlays.editOverlay(offButton, {width: 0, height: 0} );
        Overlays.editOverlay(onButton, {width: 60, height: 47 } );
        stand();
    }
    else if(clickedOverlay === hideButton || clickedOverlay === hideButtonSelected){
        Overlays.editOverlay(hideButton, { width: 60, height: 47 } );
        Overlays.editOverlay(hideButtonSelected, { width: 1, height: 1 } );
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
                case configFlyingButtonSelected:
                case configWalkStylesButtonSelected:
                case configWalkTweaksButtonSelected:
                case configWalkJointsButtonSelected:
                    setInternalState(STANDING);
                    break;

                case onButton:
                    powerOn = false;
                    setInternalState(STANDING);
                    Overlays.editOverlay(offButton, {width: 60, height: 47 } );
                    Overlays.editOverlay(onButton, {width: 0, height: 0} );
                    resetJoints();
                    break;

                case backButton:
                case backButtonSelected:
                    Overlays.editOverlay(backButton, { width: 1, height: 1 } );
                    Overlays.editOverlay(backButtonSelected, { width: 1, height: 1 } );
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
                    setInternalState(CONFIG_WALK_STYLES); //  set the default walk adjustment panel here
                    break;

                case configStandButton:
                    setInternalState(CONFIG_STANDING);
                    break;

                case configFlyingButton:
                    setInternalState(CONFIG_FLYING);
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
    for(var i in sliderthumbOverlays) {
        Overlays.deleteOverlay(sliderthumbOverlays[i]);
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
    Overlays.deleteOverlay(debugText);
});


function keyPressEvent(event) {

	//print('keyPressEvent: '+event.text);

    if (event.text == "q") {
		// export currentAnimation as json string when q key is pressed.
		// reformat result at http://www.freeformatter.com/json-formatter.html
        print('\n');
        print('walk.js dumping animation: '+currentAnimation.name+'\n');
        print('\n');
        print(JSON.stringify(currentAnimation), null, '\t');
    }
    if (event.text == "t") {
        statsOn = !statsOn;
        if(statsOn) {
			print('wheel stats on (t to turn off again)');
			Overlays.editOverlay(debugText, {visible: true});
			Overlays.editOverlay(walkWheelYLine, {visible: true});
			Overlays.editOverlay(walkWheelZLine, {visible: true});
		} else {
			print('wheel stats off (t to turn on again)');
			Overlays.editOverlay(debugText, {visible: false});
			Overlays.editOverlay(walkWheelYLine, {visible: false});
			Overlays.editOverlay(walkWheelZLine, {visible: false});
		}
    }
}
Controller.keyPressEvent.connect(keyPressEvent);





// debug and other info
var VERBOSE = false;

// TODO: implement joint mapping using reg expressions to cover a wide range of avi bone structures
var jointList = MyAvatar.getJointNames();
var jointMappings = "\n# Avatar joint list start";
for (var i = 0; i < jointList.length; i++) {
    jointMappings = jointMappings + "\njointIndex = " + jointList[i] + " = " + i;
}
print(jointMappings + "\n# walk.js avatar joint list end");

// clear the joint data so can calculate hips to feet distance
for(var i = 0 ; i < 5 ; i++) {
    //MyAvatar.setJointData(i, Quat.fromPitchYawRollDegrees(0,0,0));
    MyAvatar.clearJointData(jointList[i]);
}
var hipsToFeetDistance = MyAvatar.getJointPosition("Hips").y - MyAvatar.getJointPosition("RightFoot").y;
print('\nwalk.js: Hips to feet: '+hipsToFeetDistance);


////////////////////////////////////////////
// begin by setting the to state STANDING //
////////////////////////////////////////////

//curlFingers();
setInternalState(STANDING);