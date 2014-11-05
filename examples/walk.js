//
//  walk.js
//
//  version 1.1
//
//  Created by David Wooldridge, Autumn 2014
//
//  Animates an avatar using procedural animation techniques
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// constants
var MALE = 1;
var FEMALE = 2;
var MAX_WALK_SPEED = 2.5;
var TAKE_FLIGHT_SPEED = 4.55;
var TOP_SPEED = 300;
var UP = 1;
var DOWN = 2;
var LEFT = 4;
var RIGHT = 8;
var FORWARDS = 16;
var BACKWARDS = 32;

// ovelay images location
var pathToAssets = 'http://s3.amazonaws.com/hifi-public/WalkScript/';

// load the UI
Script.include("./libraries/walkInterface.js");

// load filters (Bezier, Butterworth, add harmonics, averaging)
Script.include("./libraries/walkFilters.js");

// load objects, constructors and assets (state, Motion, Transition, walkAssets)
Script.include("./libraries/walkApi.js");

// initialise the motion state / history object
var motion = new Motion();

// initialise Transitions
var nullTransition = new Transition();
motion.curTransition = nullTransition;

// initialise the UI
walkInterface.initialise(state, motion, walkAssets);

// wave shapes
var SAWTOOTH = 1;
var TRIANGLE = 2;
var SQUARE = 4;

// filters for synthesising more complex, natural waveforms
var leanPitchFilter = filter.createAveragingFilter(15);
var leanRollFilter = filter.createAveragingFilter(15);
var hipsYawShaper = filter.createWaveSynth(TRIANGLE, 3, 2);
var hipsBobLPFilter = filter.createButterworthFilter(5);


// Main loop
Script.update.connect(function(deltaTime) {

	if (state.powerOn) {

		motion.frameStartTime = new Date().getTime();
		motion.cumulativeTime += deltaTime;
		motion.nFrames++;
		var speed = 0;

		// check for editing modes first, as these require no positioning calculations
		switch (state.currentState) {

			case state.EDIT_WALK_STYLES:
				motion.curAnim = motion.selWalk;
				animateAvatar(deltaTime, speed);
				break;

			case state.EDIT_WALK_TWEAKS:
				motion.curAnim = motion.selWalk;
				animateAvatar(deltaTime, speed);
				break;

			case state.EDIT_WALK_JOINTS:
				motion.curAnim = motion.selWalk;
				animateAvatar(deltaTime, speed);
				break;

			case state.EDIT_STANDING:
				motion.curAnim = motion.selStand;
				motion.direction = FORWARDS;
				animateAvatar(deltaTime, speed);
				break;

			case state.EDIT_SIDESTEP_LEFT:
				motion.curAnim = motion.selSideStepLeft;
				motion.direction = LEFT;
				animateAvatar(deltaTime, speed);
				break;

			case state.EDIT_SIDESTEP_RIGHT:
				motion.curAnim = motion.selSideStepRight;
				motion.direction = RIGHT;
				animateAvatar(deltaTime, speed);
				break;

			case state.EDIT_FLYING:
				motion.curAnim = motion.selFly;
				motion.direction = FORWARDS;
				animateAvatar(deltaTime, speed);
				break;

			case state.EDIT_FLYING_UP:
				motion.curAnim = motion.selFlyUp;
				motion.direction = UP;
				animateAvatar(deltaTime, speed);
				break;

			case state.EDIT_FLYING_DOWN:
				motion.curAnim = motion.selFlyDown;
				motion.direction = DOWN;
				animateAvatar(deltaTime, speed);
				break;

			default:
				break;
		}

		// calcualte velocity and speed
		var velocity = MyAvatar.getVelocity();
		speed = Vec3.length(velocity);

		if (motion.curTransition !== nullTransition) {

			// finish any live transition before changing state
			animateAvatar(deltaTime, speed);
			return;
		}
		var localVelocity = {x: 0, y: 0, z: 0};
		if (speed > 0)
			localVelocity = Vec3.multiplyQbyV(Quat.inverse(MyAvatar.orientation), velocity);

		if (!state.editing) {

			// determine the candidate animation state
			var actionToTake = undefined;
			if (speed < 0.05) actionToTake = state.STANDING; // as per MIN_AVATAR_SPEED, MyAvatar.cpp
			else if (speed < TAKE_FLIGHT_SPEED) actionToTake = state.WALKING;
			else if (speed >= TAKE_FLIGHT_SPEED) actionToTake = state.FLYING;

			// determine the principle direction
			if (Math.abs(localVelocity.x) > Math.abs(localVelocity.y) &&
				Math.abs(localVelocity.x) > Math.abs(localVelocity.z)) {

				if (localVelocity.x < 0) motion.direction = LEFT;
				else motion.direction = RIGHT;

			} else if (Math.abs(localVelocity.y) > Math.abs(localVelocity.x) &&
					   Math.abs(localVelocity.y) > Math.abs(localVelocity.z)) {

				if (localVelocity.y > 0) motion.direction = UP;
				else motion.direction = DOWN;

			} else if (Math.abs(localVelocity.z) > Math.abs(localVelocity.x) &&
					   Math.abs(localVelocity.z) > Math.abs(localVelocity.y)) {

				if (localVelocity.z < 0) motion.direction = FORWARDS;
				else motion.direction = BACKWARDS;
			}

			// maybe at walking speed, but sideways?
			if (actionToTake === state.WALKING &&
				(motion.direction === LEFT ||
					motion.direction === RIGHT))
						actionToTake = state.SIDE_STEP;

			// maybe at walking speed, but flying up or down?
			if (actionToTake === state.WALKING &&
				(motion.direction === UP))// ||
					//motion.direction === DOWN))
						actionToTake = state.FLYING;

			// select appropriate animation and initiate Transition if required
			switch (actionToTake) {

				case state.STANDING:

					// do we need to change state?
					if (state.currentState !== state.STANDING) {

						switch (motion.curAnim) {

							case motion.selWalk:

								// Walking to standing
								motion.curTransition = new Transition(
									motion.curAnim,
									motion.selWalk,
									[], 0.25,
									{x: 0.1, y: 0.5},
									{x: -0.25, y: 1.22});
								break;

							case motion.selSideStepLeft:
							case motion.selSideStepRight:

								break;

							default:

								// flying to standing
								motion.curTransition = new Transition(
									motion.curAnim,
									motion.selStand,
									[], 0.25,
									{x: 0.5, y: 0.08},
									{x: 0.28, y: 1});
								break;
						}
						state.setInternalState(state.STANDING);
						motion.curAnim = motion.selStand;
					}
					animateAvatar(deltaTime, speed);
					break;

				case state.WALKING:

					if (state.currentState !== state.WALKING) {

						if (motion.direction === BACKWARDS)
								 motion.walkWheelPos = motion.selWalk.calibration.startAngleBackwards;

						else motion.walkWheelPos = motion.selWalk.calibration.startAngleForwards;

						switch (motion.curAnim) {

							case motion.selStand:

								// Standing to Walking
								motion.curTransition = new Transition(
									motion.curAnim,
									motion.selWalk,
									[], 0.1,
									{x: 0.5, y: 0.1},
									{x: 0.5, y: 0.9});
								break;

							case motion.selSideStepLeft:
							case motion.selSideStepRight:

								break;

							default:

								// Flying to Walking
								motion.curTransition = new Transition(
									motion.curAnim,
									motion.selWalk,
									[], 0.1,
									{x: 0.24, y: 0.03},
									{x: 0.42, y: 1.0});
								break;
						}
						state.setInternalState(state.WALKING);
					}
					motion.curAnim = motion.selWalk;
					animateAvatar(deltaTime, speed);
					break;

				case state.SIDE_STEP:

					var selSideStep = 0;
					if (motion.direction === LEFT) {

						if (motion.lastDirection !== LEFT)
							motion.walkWheelPos = motion.selSideStepLeft.calibration.cycleStart;
						selSideStep = motion.selSideStepLeft;

					} else {

						if (motion.lastDirection !== RIGHT)
							motion.walkWheelPos = motion.selSideStepRight.calibration.cycleStart;
						selSideStep = motion.selSideStepRight;
					}

					if (state.currentState !== state.SIDE_STEP) {

						if (motion.direction === LEFT) {

							motion.walkWheelPos = motion.selSideStepLeft.calibration.cycleStart;
							switch (motion.curAnim) {

								case motion.selStand:

									break;

								default:

									break;
							}

						} else {

							motion.walkWheelPos = motion.selSideStepRight.calibration.cycleStart;
							switch (motion.curAnim) {

								case motion.selStand:

									break;

								default:

									break;
							}
						}
						state.setInternalState(state.SIDE_STEP);
					}
					motion.curAnim = selSideStep;
					animateAvatar(deltaTime, speed);
					break;

				case state.FLYING:

					if (state.currentState !== state.FLYING)
						state.setInternalState(state.FLYING);

					// change animation for flying directly up or down
					if (motion.direction === UP) {

						if (motion.curAnim !== motion.selFlyUp) {

							switch (motion.curAnim) {

								case motion.selStand:
								case motion.selWalk:

									// standing | walking to flying up
									motion.curTransition = new Transition(
										motion.curAnim,
										motion.selFlyUp,
										[], 0.25,
										{x: 0.5, y: 0.08},
										{x: 0.28, y: 1});
									break;

								case motion.selSideStepLeft:
								case motion.selSideStepRight:

									break;

								default:

									motion.curTransition = new Transition(
										motion.curAnim,
										motion.selFlyUp,
										[], 0.35,
										{x: 0.5, y: 0.08},
										{x: 0.28, y: 1});
									break;
							}
							motion.curAnim = motion.selFlyUp;
						}

					} else if (motion.direction == DOWN) {

						if (motion.curAnim !== motion.selFlyDown) {

							switch (motion.curAnim) {

								case motion.selStand:
								case motion.selWalk:

									motion.curTransition = new Transition(
										motion.curAnim,
										motion.selFlyDown,
										[], 0.35,
										{x: 0.5, y: 0.08},
										{x: 0.28, y: 1});
									break;

								case motion.selSideStepLeft:
								case motion.selSideStepRight:

									break;

								default:

									motion.curTransition = new Transition(
										motion.curAnim,
										motion.selFlyDown,
										[], 0.5,
										{x: 0.5, y: 0.08},
										{x: 0.28, y: 1});
									break;
							}
							motion.curAnim = motion.selFlyDown;
						}

					} else {

						if (motion.curAnim !== motion.selFly) {

							switch (motion.curAnim) {

								case motion.selStand:
								case motion.selWalk:

									motion.curTransition = new Transition(
										motion.curAnim,
										motion.selFly,
										[], 0.35,
										{x: 1.44, y:0.24},
										{x: 0.61, y:0.92});
									break;

								case motion.selSideStepLeft:
								case motion.selSideStepRight:

									break;

								default:

									motion.curTransition = new Transition(
										motion.curAnim,
										motion.selFly,
										[], 0.75,
										{x: 0.5, y: 0.08},
										{x: 0.28, y: 1});
									break;
							}
							motion.curAnim = motion.selFly;
						}
					}
					animateAvatar(deltaTime, speed);
					break;

			} // end switch(actionToTake)

		} // end if (!state.editing)

		// record the frame's direction and local avatar's local velocity for future reference
		motion.lastDirection = motion.direction;
		motion.lastVelocity = localVelocity;
	}
});

// the faster we go, the further we lean forward. the angle is calcualted here
function getLeanPitch(speed) {

	if (speed > TOP_SPEED) speed = TOP_SPEED;
	var leanProgress = speed / TOP_SPEED;

	if (motion.direction === LEFT ||
	   motion.direction === RIGHT)
		leanProgress = 0;

	else {

		var responseSharpness = 1.5;
		if (motion.direction == BACKWARDS) responseSharpness = 3.0;

		leanProgress = filter.bezier((1 - leanProgress),
										{x: 0, y: 0.0},
										{x: 0, y: responseSharpness},
										{x: 0, y: 1.5},
										{x: 1, y: 1}).y;

		// determine final pitch and adjust for direction of momentum
		if (motion.direction === BACKWARDS)
			leanProgress = -motion.motionPitchMax * leanProgress;
		else
			leanProgress = motion.motionPitchMax * leanProgress;
	}

	// return the smoothed response
	return leanPitchFilter.process(leanProgress);
}

// calculate the angle at which to bank into corners when turning
function getLeanRoll(deltaTime, speed) {

	var leanRollProgress = 0;
	if (speed > TOP_SPEED) speed = TOP_SPEED;

	// what's our our anglular velocity?
	var angularVelocityMax = 70; // from observation
	var angularVelocity = filter.radToDeg(MyAvatar.getAngularVelocity().y);
	if (angularVelocity > angularVelocityMax) angularVelocity = angularVelocityMax;
	if (angularVelocity < -angularVelocityMax) angularVelocity = -angularVelocityMax;

	leanRollProgress = speed / TOP_SPEED;

	if (motion.direction !== LEFT &&
	   motion.direction !== RIGHT)
			leanRollProgress *= (Math.abs(angularVelocity) / angularVelocityMax);

	// apply our response curve
	leanRollProgress = filter.bezier((1 - leanRollProgress),
											   {x: 0, y: 0},
											   {x: 0, y: 1},
											   {x: 0, y: 1},
											   {x: 1, y: 1}).y;
	// which way to lean?
	var turnSign = -1;
	if (angularVelocity < 0.001) turnSign = 1;
	if (motion.direction === BACKWARDS ||
	   motion.direction === LEFT)
		turnSign *= -1;

	if (motion.direction === LEFT ||
	   motion.direction === RIGHT)
	   		leanRollProgress *= 2;

	// add damping with simple averaging filter
	leanRollProgress = leanRollFilter.process(turnSign * leanRollProgress);
	return motion.motionRollMax * leanRollProgress;
}

function playFootstep(side) {

	var options = new AudioInjectionOptions();
	options.position = Camera.getPosition();
	options.volume = 0.2;
	var soundNumber = 2; // 0 to 2
	if (side === RIGHT && motion.makesFootStepSounds)
		Audio.playSound(walkAssets.footsteps[soundNumber + 1], options);
	else if (side === LEFT && motion.makesFootStepSounds)
		Audio.playSound(walkAssets.footsteps[soundNumber], options);
}

// animate the avatar using sine wave generators. inspired by Victorian clockwork dolls
function animateAvatar(deltaTime, speed) {

	var cycle = motion.cumulativeTime;
	var transProgress = 1;
	var adjFreq = motion.curAnim.calibration.frequency;

	// legs phase and cycle reversal for walking backwards
	var reverseModifier = 0;
	var reverseSignModifier = 1;
	if (motion.direction === BACKWARDS) {
		reverseModifier = -180;
		reverseSignModifier = -1;
	}

	// don't lean into the direction of travel if going up
	var leanMod = 1;
	if (motion.direction === UP)
		leanMod = 0;

	// adjust leaning direction for flying
	var flyingModifier = 1;
	if (state.currentState.FLYING)
		flyingModifier = -1;

	if (motion.curTransition !== nullTransition) {

		// new transiton?
		if (motion.curTransition.progress === 0 &&
			motion.curTransition.walkingAtStart) {

			if (state.currentState !== state.SIDE_STEP) {

				// work out where we want the walk cycle to stop
				var leftStop = motion.selWalk.calibration.stopAngleForwards + 180;
				var rightStop = motion.selWalk.calibration.stopAngleForwards;

				if (motion.direction === BACKWARDS) {
					leftStop = motion.selWalk.calibration.stopAngleBackwards + 180;
					rightStop = motion.selWalk.calibration.stopAngleBackwards;
				}

				// find the closest stop point from the walk wheel's angle
				var angleToLeftStop = 180 - Math.abs(Math.abs(motion.walkWheelPos - leftStop) - 180);
				var angleToRightStop = 180 - Math.abs(Math.abs(motion.walkWheelPos - rightStop) - 180);
				if (motion.walkWheelPos > angleToLeftStop) angleToLeftStop = 360 - angleToLeftStop;
				if (motion.walkWheelPos > angleToRightStop) angleToRightStop = 360 - angleToRightStop;


				motion.curTransition.walkWheelIncrement = 3;

				// keep the walkwheel turning by setting the walkWheelIncrement
				// until our feet are tucked nicely underneath us.
				if (angleToLeftStop < angleToRightStop)

					motion.curTransition.walkStopAngle = leftStop;

				else
					motion.curTransition.walkStopAngle = rightStop;

			} else {

				// freeze wheel for sidestepping transitions (for now)
				motion.curTransition.walkWheelIncrement = 0;
			}
		} // end if (new transition and curTransition.walkingAtStart)

		// update the Transition progress
		var elapasedTime = (new Date().getTime() - motion.curTransition.startTime) / 1000;
		motion.curTransition.progress = elapasedTime / motion.curTransition.transitionDuration;
		transProgress = filter.bezier((1 - motion.curTransition.progress), {x: 0, y: 0},
							motion.curTransition.easingLower,
							motion.curTransition.easingUpper, {x: 1, y: 1}).y;

		if (motion.curTransition.progress >= 1) {

			// time to kill off the transition
			delete motion.curTransition;
			motion.curTransition = nullTransition;

		} else {

			if (motion.curTransition.walkingAtStart) {

				if (state.currentState !== state.SIDE_STEP) {

					// if at a stop angle, hold the walk wheel position for remainder of transition
					var tolerance = 7; // must be greater than the walkWheel increment
					if ((motion.walkWheelPos > (motion.curTransition.walkStopAngle - tolerance)) &&
						(motion.walkWheelPos < (motion.curTransition.walkStopAngle + tolerance))) {

						motion.curTransition.walkWheelIncrement = 0;
					}
					// keep turning walk wheel until both feet are below the avi

					motion.advanceWalkWheel(motion.curTransition.walkWheelIncrement);
					//motion.curTransition.walkWheelAdvance += motion.curTransition.walkWheelIncrement;
				}
				else motion.curTransition.walkWheelIncrement = 0; // sidestep
			}
		} } // end motion.curTransition !== nullTransition


	// walking? then get the stride length
	if (motion.curAnim === motion.selWalk) {

		// if the timing's right, take a snapshot of the stride max and recalibrate
		var strideMaxAt = motion.curAnim.calibration.forwardStrideMaxAt;
		if (motion.direction === BACKWARDS)
			strideMaxAt = motion.curAnim.calibration.backwardsStrideMaxAt;

		var tolerance = 1.0;
		if (motion.walkWheelPos < (strideMaxAt + tolerance) &&
			motion.walkWheelPos > (strideMaxAt - tolerance)) {

			// measure and save stride length
			var footRPos = MyAvatar.getJointPosition("RightFoot");
			var footLPos = MyAvatar.getJointPosition("LeftFoot");
			motion.strideLength = Vec3.distance(footRPos, footLPos);

			if (motion.direction === FORWARDS)
				motion.curAnim.calibration.strideLengthForwards = motion.strideLength;
			else if (motion.direction === BACKWARDS)
				motion.curAnim.calibration.strideLengthBackwards = motion.strideLength;

		} else {

			// use the saved value for stride length
			if (motion.direction === FORWARDS)
				motion.strideLength = motion.curAnim.calibration.strideLengthForwards;
			else if (motion.direction === BACKWARDS)
				motion.strideLength = motion.curAnim.calibration.strideLengthBackwards;
		}
	} // end get walk stride length

	// sidestepping? get the stride length
	if (motion.curAnim === motion.selSideStepLeft ||
		motion.curAnim === motion.selSideStepRight) {

		// if the timing's right, take a snapshot of the stride max and recalibrate the stride length
		var tolerance = 1.0;
		if (motion.direction === LEFT) {

			if (motion.walkWheelPos < motion.curAnim.calibration.strideMaxAt + tolerance &&
				motion.walkWheelPos > motion.curAnim.calibration.strideMaxAt - tolerance) {

				var footRPos = MyAvatar.getJointPosition("RightFoot");
				var footLPos = MyAvatar.getJointPosition("LeftFoot");
				motion.strideLength = Vec3.distance(footRPos, footLPos);
				motion.curAnim.calibration.strideLength = motion.strideLength;

			} else motion.strideLength = motion.selSideStepLeft.calibration.strideLength;

		} else if (motion.direction === RIGHT) {

			if (motion.walkWheelPos < motion.curAnim.calibration.strideMaxAt + tolerance &&
				motion.walkWheelPos > motion.curAnim.calibration.strideMaxAt - tolerance) {

				var footRPos = MyAvatar.getJointPosition("RightFoot");
				var footLPos = MyAvatar.getJointPosition("LeftFoot");
				motion.strideLength = Vec3.distance(footRPos, footLPos);
				motion.curAnim.calibration.strideLength = motion.strideLength;

			} else motion.strideLength = motion.selSideStepRight.calibration.strideLength;
		}
	} // end get sidestep stride length

	// turn the walk wheel
	if (motion.curAnim === motion.selWalk ||
	   motion.curAnim === motion.selSideStepLeft ||
	   motion.curAnim === motion.selSideStepRight ||
	   motion.curTransition.walkingAtStart) {

		// wrap the stride length around a 'surveyor's wheel' twice and calculate
		// the angular speed at the given (linear) speed:
		// omega = v / r , where r = circumference / 2 PI , where circumference = 2 * stride length
		var strideLength = motion.strideLength;
		var wheelRadius = strideLength / Math.PI;
		var angularVelocity = speed / wheelRadius;

		// calculate the degrees turned (at this angular speed) since last frame
		var radiansTurnedSinceLastFrame = deltaTime * angularVelocity;
		var degreesTurnedSinceLastFrame = filter.radToDeg(radiansTurnedSinceLastFrame);

		// if we are in an edit mode, we will need fake time to turn the wheel
		if (state.currentState !== state.WALKING &&
			state.currentState !== state.SIDE_STEP)
			degreesTurnedSinceLastFrame = motion.curAnim.calibration.frequency / 70;

		// advance the walk wheel the appropriate amount
		motion.advanceWalkWheel(degreesTurnedSinceLastFrame);

		// set the new values for the exact correct walk cycle speed
		adjFreq = 1;
		cycle = motion.walkWheelPos;

	} // end of walk wheel and stride length calculation

	// motion vars
	var pitchOsc = 0;
	var pitchOscLeft = 0;
	var pitchOscRight = 0;
	var yawOsc = 0;
	var yawOscLeft = 0;
	var yawOscRight = 0;
	var rollOsc = 0;
	var pitchOffset = 0;
	var yawOffset = 0;
	var rollOffset = 0;
	var swayOsc = 0;
	var bobOsc = 0;
	var thrustOsc = 0;
	var swayOscLast = 0;
	var bobOscLast = 0;
	var thrustOscLast = 0;

	// historical (for transitions)
	var pitchOscLast = 0;
	var pitchOscLeftLast = 0;
	var pitchOscRightLast = 0;
	var yawOscLast = 0;
	var rollOscLast = 0;
	var pitchOffsetLast = 0;
	var yawOffsetLast = 0;
	var rollOffsetLast = 0;

	// feet
	var sideStepFootPitchModifier = 1;
	var sideStepHandPitchSign = 1;

	// The below code should probably be optimised into some sort of loop, where
	// the joints are iterated through. However, this has not been done yet, as there
	// are still some quite fundamental changes to be made (e.g. turning on the spot
	// animation and sidestepping transitions) so it's been left as is for ease of
	// understanding and editing.

	// calculate hips translation
	if (motion.curTransition !== nullTransition) {

		if (motion.curTransition.walkingAtStart) {

			swayOsc = motion.curAnim.joints[0].sway *
					  Math.sin(filter.degToRad(motion.cumulativeTime * motion.curAnim.calibration.frequency +
					  motion.curAnim.joints[0].swayPhase)) + motion.curAnim.joints[0].swayOffset;

			var bobPhase = motion.curAnim.joints[0].bobPhase;
			if (motion.direction === motion.BACKWARDS) bobPhase += 90;
			bobOsc = motion.curAnim.joints[0].bob *
					 Math.sin(filter.degToRad(motion.cumulativeTime *
					 motion.curAnim.calibration.frequency + bobPhase)) +
					 motion.curAnim.joints[0].bobOffset;

			thrustOsc = motion.curAnim.joints[0].thrust *
						Math.sin(filter.degToRad(motion.cumulativeTime *
						motion.curAnim.calibration.frequency * 2 +
						motion.curAnim.joints[0].thrustPhase)) +
						motion.curAnim.joints[0].thrustOffset;

			swayOscLast = motion.curTransition.lastAnim.joints[0].sway *
						  Math.sin(filter.degToRad(motion.walkWheelPos +
						  motion.curTransition.lastAnim.joints[0].swayPhase)) +
						  motion.curTransition.lastAnim.joints[0].swayOffset;

			var bobPhaseLast = motion.curTransition.lastAnim.joints[0].bobPhase;
			if (motion.direction === motion.BACKWARDS) bobPhaseLast +=90;
			bobOscLast = motion.curTransition.lastAnim.joints[0].bob *
						 Math.sin(filter.degToRad(motion.walkWheelPos + bobPhaseLast));
			bobOscLast = filter.clipTrough(bobOscLast, motion.curTransition.lastAnim.joints[0].bob , 2);
			bobOscLast = hipsBobLPFilter.process(bobOscLast);
			bobOscLast += motion.curTransition.lastAnim.joints[0].bobOffset;

			thrustOscLast = motion.curTransition.lastAnim.joints[0].thrust *
							Math.sin(filter.degToRad(motion.walkWheelPos * 2 +
							motion.curTransition.lastAnim.joints[0].thrustPhase)) +
							motion.curTransition.lastAnim.joints[0].thrustOffset;

		} // end if walking at start of transition
		else {

			swayOsc = motion.curAnim.joints[0].sway *
					  Math.sin(filter.degToRad(cycle * adjFreq + motion.curAnim.joints[0].swayPhase)) +
					  motion.curAnim.joints[0].swayOffset;

			var bobPhase = motion.curAnim.joints[0].bobPhase;
			if (motion.direction === motion.BACKWARDS) bobPhase += 90;
			bobOsc = motion.curAnim.joints[0].bob *
					 Math.sin(filter.degToRad(cycle * adjFreq * 2 + bobPhase));
			if (state.currentState === state.WALKING ||
			   state.currentState === state.EDIT_WALK_STYLES ||
			   state.currentState === state.EDIT_WALK_TWEAKS ||
			   state.currentState === state.EDIT_WALK_JOINTS) {

					// apply clipping filter to flatten the curve's peaks (inputValue, peak, strength)
					bobOsc = filter.clipTrough(bobOsc, motion.curAnim.joints[0].bob , 2);
					bobOsc = hipsBobLPFilter.process(bobOsc);
			}
			bobOsc += motion.curAnim.joints[0].bobOffset;

			thrustOsc = motion.curAnim.joints[0].thrust *
						Math.sin(filter.degToRad(cycle * adjFreq * 2 +
						motion.curAnim.joints[0].thrustPhase)) +
						motion.curAnim.joints[0].thrustOffset;

			swayOscLast = motion.curTransition.lastAnim.joints[0].sway *
						  Math.sin(filter.degToRad(motion.cumulativeTime *
						  motion.curTransition.lastAnim.calibration.frequency +
						  motion.curTransition.lastAnim.joints[0].swayPhase)) +
						  motion.curTransition.lastAnim.joints[0].swayOffset;

			bobOscLast = motion.curTransition.lastAnim.joints[0].bob *
						 Math.sin(filter.degToRad(motion.cumulativeTime *
						 motion.curTransition.lastAnim.calibration.frequency * 2 +
						 motion.curTransition.lastAnim.joints[0].bobPhase)) +
						 motion.curTransition.lastAnim.joints[0].bobOffset;

			thrustOscLast = motion.curTransition.lastAnim.joints[0].thrust *
							Math.sin(filter.degToRad(motion.cumulativeTime *
							motion.curTransition.lastAnim.calibration.frequency * 2 +
							motion.curTransition.lastAnim.joints[0].thrustPhase)) +
							motion.curTransition.lastAnim.joints[0].thrustOffset;
		}

		swayOsc = (transProgress * swayOsc) + ((1 - transProgress) * swayOscLast);
		bobOsc = (transProgress * bobOsc) + ((1 - transProgress) * bobOscLast);
		thrustOsc = (transProgress * thrustOsc) + ((1 - transProgress) * thrustOscLast);

	}// if current transition active
	else {

		swayOsc = motion.curAnim.joints[0].sway *
				  Math.sin(filter.degToRad(cycle * adjFreq + motion.curAnim.joints[0].swayPhase)) +
				  motion.curAnim.joints[0].swayOffset;

		bobPhase = motion.curAnim.joints[0].bobPhase;
		if (motion.direction === motion.BACKWARDS) bobPhase += 90;
		bobOsc = motion.curAnim.joints[0].bob * Math.sin(filter.degToRad(cycle * adjFreq * 2 + bobPhase));
		if (state.currentState === state.WALKING ||
		   state.currentState === state.EDIT_WALK_STYLES ||
		   state.currentState === state.EDIT_WALK_TWEAKS ||
		   state.currentState === state.EDIT_WALK_JOINTS) {

				// apply clipping filter to flatten the curve's peaks (inputValue, peak, strength)
				bobOsc = filter.clipTrough(bobOsc, motion.curAnim.joints[0].bob , 2);
				bobOsc = hipsBobLPFilter.process(bobOsc);
		}
		bobOsc += motion.curAnim.joints[0].bobOffset;

		thrustOsc = motion.curAnim.joints[0].thrust *
					Math.sin(filter.degToRad(cycle * adjFreq * 2 +
					motion.curAnim.joints[0].thrustPhase)) +
					motion.curAnim.joints[0].thrustOffset;
	}

	// convert local hips translations to global and apply
	var aviOrientation = MyAvatar.orientation;
	var front = Quat.getFront(aviOrientation);
	var right = Quat.getRight(aviOrientation);
	var up = Quat.getUp(aviOrientation);
	var aviFront = Vec3.multiply(front, thrustOsc);
	var aviRight = Vec3.multiply(right, swayOsc);
	var aviUp = Vec3.multiply(up, bobOsc);
	var aviTranslationOffset = {x: 0, y: 0, z: 0};

	aviTranslationOffset = Vec3.sum(aviTranslationOffset, aviFront);
	aviTranslationOffset = Vec3.sum(aviTranslationOffset, aviRight);
	aviTranslationOffset = Vec3.sum(aviTranslationOffset, aviUp);

	MyAvatar.setSkeletonOffset({
		x: aviTranslationOffset.x,
		y: aviTranslationOffset.y,
		z: aviTranslationOffset.z
	});

	// hips rotation
	if (motion.curTransition !== nullTransition) {

		if (motion.curTransition.walkingAtStart) {

			pitchOsc = motion.curAnim.joints[0].pitch *
				Math.sin(filter.degToRad(motion.cumulativeTime * motion.curAnim.calibration.frequency * 2 +
				motion.curAnim.joints[0].pitchPhase)) + motion.curAnim.joints[0].pitchOffset;

			yawOsc = motion.curAnim.joints[0].yaw *
				Math.sin(filter.degToRad(motion.cumulativeTime * motion.curAnim.calibration.frequency +
				motion.curAnim.joints[0].yawPhase - reverseModifier)) + motion.curAnim.joints[0].yawOffset;

			rollOsc = motion.curAnim.joints[0].roll *
				Math.sin(filter.degToRad(motion.cumulativeTime * motion.curAnim.calibration.frequency +
				motion.curAnim.joints[0].rollPhase)) + motion.curAnim.joints[0].rollOffset;

			pitchOscLast = motion.curTransition.lastAnim.joints[0].pitch *
				Math.sin(filter.degToRad(motion.walkWheelPos * 2 +
				motion.curTransition.lastAnim.joints[0].pitchPhase)) +
				motion.curTransition.lastAnim.joints[0].pitchOffset;

			yawOscLast = motion.curTransition.lastAnim.joints[0].yaw *
				Math.sin(filter.degToRad(motion.walkWheelPos +
				motion.curTransition.lastAnim.joints[0].yawPhase));

			yawOscLast += motion.curTransition.lastAnim.joints[0].yaw *
				hipsYawShaper.shapeWave(filter.degToRad(motion.walkWheelPos +
				motion.curTransition.lastAnim.joints[0].yawPhase - reverseModifier)) +
				motion.curTransition.lastAnim.joints[0].yawOffset;

			rollOscLast = (motion.curTransition.lastAnim.joints[0].roll *
				Math.sin(filter.degToRad(motion.walkWheelPos +
				motion.curTransition.lastAnim.joints[0].rollPhase)) +
				motion.curTransition.lastAnim.joints[0].rollOffset);

		} else {

			pitchOsc = motion.curAnim.joints[0].pitch *
				Math.sin(filter.degToRad(cycle * adjFreq * 2 +
				motion.curAnim.joints[0].pitchPhase)) +
				motion.curAnim.joints[0].pitchOffset;

			yawOsc = motion.curAnim.joints[0].yaw *
				Math.sin(filter.degToRad(cycle * adjFreq +
				motion.curAnim.joints[0].yawPhase - reverseModifier)) +
				motion.curAnim.joints[0].yawOffset;

			rollOsc = motion.curAnim.joints[0].roll *
				Math.sin(filter.degToRad(cycle * adjFreq +
				motion.curAnim.joints[0].rollPhase)) +
				motion.curAnim.joints[0].rollOffset;

			pitchOscLast = motion.curTransition.lastAnim.joints[0].pitch *
				Math.sin(filter.degToRad(motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency * 2 +
				motion.curTransition.lastAnim.joints[0].pitchPhase)) +
				motion.curTransition.lastAnim.joints[0].pitchOffset;

			yawOscLast = motion.curTransition.lastAnim.joints[0].yaw *
				Math.sin(filter.degToRad(motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency +
				motion.curTransition.lastAnim.joints[0].yawPhase - reverseModifier)) +
				motion.curTransition.lastAnim.joints[0].yawOffset;

			rollOscLast = motion.curTransition.lastAnim.joints[0].roll *
				Math.sin(filter.degToRad(motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency +
				motion.curTransition.lastAnim.joints[0].rollPhase)) +
				motion.curTransition.lastAnim.joints[0].rollOffset;
		}

		pitchOsc = (transProgress * pitchOsc) + ((1 - transProgress) * pitchOscLast);
		yawOsc = (transProgress * yawOsc) + ((1 - transProgress) * yawOscLast);
		rollOsc = (transProgress * rollOsc) + ((1 - transProgress) * rollOscLast);

	} else {

		pitchOsc = motion.curAnim.joints[0].pitch *
			Math.sin(filter.degToRad((cycle * adjFreq * 2) +
			motion.curAnim.joints[0].pitchPhase)) +
			motion.curAnim.joints[0].pitchOffset;

		yawOsc = motion.curAnim.joints[0].yaw *
			Math.sin(filter.degToRad((cycle * adjFreq) +
			motion.curAnim.joints[0].yawPhase));

		yawOsc += motion.curAnim.joints[0].yaw *
			  hipsYawShaper.shapeWave(filter.degToRad(cycle * adjFreq) +
			  motion.curAnim.joints[0].yawPhase - reverseModifier)+
			  motion.curAnim.joints[0].yawOffset;

		rollOsc = (motion.curAnim.joints[0].roll *
				Math.sin(filter.degToRad((cycle * adjFreq) +
				motion.curAnim.joints[0].rollPhase)) +
				motion.curAnim.joints[0].rollOffset);
	}

	// apply hips rotation
	MyAvatar.setJointData("Hips", Quat.fromPitchYawRollDegrees(
									pitchOsc + (leanMod * getLeanPitch(speed)),
									yawOsc,
									rollOsc + getLeanRoll(deltaTime, speed)));

	// upper legs
	if (state.currentState !== state.SIDE_STEP &&
	   state.currentState !== state.EDIT_SIDESTEP_LEFT &&
	   state.currentState !== state.EDIT_SIDESTEP_RIGHT) {

		if (motion.curTransition !== nullTransition) {

			if (motion.curTransition.walkingAtStart) {

				pitchOscLeft = motion.curAnim.joints[1].pitch *
					Math.sin(filter.degToRad(motion.cumulativeTime *
					motion.curAnim.calibration.frequency +
					reverseModifier * motion.curAnim.joints[1].pitchPhase));

				pitchOscRight = motion.curAnim.joints[1].pitch *
					Math.sin(filter.degToRad(motion.cumulativeTime *
					motion.curAnim.calibration.frequency +
					reverseModifier * motion.curAnim.joints[1].pitchPhase));

				yawOsc = motion.curAnim.joints[1].yaw *
					Math.sin(filter.degToRad(motion.cumulativeTime *
					motion.curAnim.calibration.frequency +
					motion.curAnim.joints[1].yawPhase));

				rollOsc = motion.curAnim.joints[1].roll *
					Math.sin(filter.degToRad(motion.cumulativeTime *
					motion.curAnim.calibration.frequency +
					motion.curAnim.joints[1].rollPhase));

				pitchOffset = motion.curAnim.joints[1].pitchOffset;
				yawOffset = motion.curAnim.joints[1].yawOffset;
				rollOffset = motion.curAnim.joints[1].rollOffset;

				pitchOscLeftLast = motion.curTransition.lastAnim.joints[1].pitch *
					motion.curTransition.lastAnim.harmonics.leftUpperLeg.calculate(
					filter.degToRad(motion.walkWheelPos +
						motion.curTransition.lastAnim.joints[1].pitchPhase + 180 + reverseModifier));

				pitchOscRightLast = motion.curTransition.lastAnim.joints[1].pitch *
					motion.curTransition.lastAnim.harmonics.rightUpperLeg.calculate(
					filter.degToRad(motion.walkWheelPos +
						motion.curTransition.lastAnim.joints[1].pitchPhase + reverseModifier));

				yawOscLast = motion.curTransition.lastAnim.joints[1].yaw *
					Math.sin(filter.degToRad(motion.walkWheelPos +
					motion.curTransition.lastAnim.joints[1].yawPhase));

				rollOscLast = motion.curTransition.lastAnim.joints[1].roll *
					Math.sin(filter.degToRad(motion.walkWheelPos +
					motion.curTransition.lastAnim.joints[1].rollPhase));

				pitchOffsetLast = motion.curTransition.lastAnim.joints[1].pitchOffset;
				yawOffsetLast = motion.curTransition.lastAnim.joints[1].yawOffset;
				rollOffsetLast = motion.curTransition.lastAnim.joints[1].rollOffset;

			} else {

				if (state.currentState === state.WALKING ||
				   state.currentState === state.EDIT_WALK_STYLES ||
				   state.currentState === state.EDIT_WALK_TWEAKS ||
				   state.currentState === state.EDIT_WALK_JOINTS) {

					pitchOscLeft = motion.curAnim.joints[1].pitch *
								   motion.curAnim.harmonics.leftUpperLeg.calculate(filter.degToRad(cycle * adjFreq +
								   motion.curAnim.joints[1].pitchPhase + 180 + reverseModifier));
					pitchOscRight = motion.curAnim.joints[1].pitch *
									motion.curAnim.harmonics.rightUpperLeg.calculate(filter.degToRad(cycle * adjFreq +
									motion.curAnim.joints[1].pitchPhase + reverseModifier));
				} else {

					pitchOscLeft = motion.curAnim.joints[1].pitch * Math.sin(filter.degToRad(cycle * adjFreq
												+ motion.curAnim.joints[1].pitchPhase));
					pitchOscRight = motion.curAnim.joints[1].pitch * Math.sin(filter.degToRad(cycle * adjFreq
												+ motion.curAnim.joints[1].pitchPhase));
				}

				yawOsc = motion.curAnim.joints[1].yaw *
					Math.sin(filter.degToRad(cycle * adjFreq +
					motion.curAnim.joints[1].yawPhase));

				rollOsc = motion.curAnim.joints[1].roll *
					Math.sin(filter.degToRad(cycle * adjFreq +
					motion.curAnim.joints[1].rollPhase));

				pitchOffset = motion.curAnim.joints[1].pitchOffset;
				yawOffset = motion.curAnim.joints[1].yawOffset;
				rollOffset = motion.curAnim.joints[1].rollOffset;

				pitchOscLeftLast = motion.curTransition.lastAnim.joints[1].pitch *
					Math.sin(filter.degToRad(motion.cumulativeTime *
					motion.curTransition.lastAnim.calibration.frequency +
					reverseModifier * motion.curTransition.lastAnim.joints[1].pitchPhase));

				pitchOscRightLast = motion.curTransition.lastAnim.joints[1].pitch *
					Math.sin(filter.degToRad(motion.cumulativeTime *
					motion.curTransition.lastAnim.calibration.frequency +
					reverseModifier * motion.curTransition.lastAnim.joints[1].pitchPhase));

				yawOscLast = motion.curTransition.lastAnim.joints[1].yaw *
					Math.sin(filter.degToRad(motion.cumulativeTime *
					motion.curTransition.lastAnim.calibration.frequency +
					motion.curTransition.lastAnim.joints[1].yawPhase));

				rollOscLast = motion.curTransition.lastAnim.joints[1].roll *
					Math.sin(filter.degToRad(motion.cumulativeTime *
					motion.curTransition.lastAnim.calibration.frequency +
					motion.curTransition.lastAnim.joints[1].rollPhase));

				pitchOffsetLast = motion.curTransition.lastAnim.joints[1].pitchOffset;
				yawOffsetLast = motion.curTransition.lastAnim.joints[1].yawOffset;
				rollOffsetLast = motion.curTransition.lastAnim.joints[1].rollOffset;
			}
			pitchOscLeft = (transProgress * pitchOscLeft) + ((1 - transProgress) * pitchOscLeftLast);
			pitchOscRight = (transProgress * pitchOscRight) + ((1 - transProgress) * pitchOscRightLast);
			yawOsc = (transProgress * yawOsc) + ((1 - transProgress) * yawOscLast);
			rollOsc = (transProgress * rollOsc) + ((1 - transProgress) * rollOscLast);

			pitchOffset = (transProgress * pitchOffset) + ((1 - transProgress) * pitchOffsetLast);
			yawOffset = (transProgress * yawOffset) + ((1 - transProgress) * yawOffsetLast);
			rollOffset = (transProgress * rollOffset) + ((1 - transProgress) * rollOffsetLast);

		} else {

			if (state.currentState === state.WALKING ||
			   state.currentState === state.EDIT_WALK_STYLES ||
			   state.currentState === state.EDIT_WALK_TWEAKS ||
			   state.currentState === state.EDIT_WALK_JOINTS) {

				pitchOscLeft = motion.curAnim.joints[1].pitch *
							   motion.curAnim.harmonics.leftUpperLeg.calculate(filter.degToRad(cycle * adjFreq +
							   motion.curAnim.joints[1].pitchPhase + 180 + reverseModifier));
				pitchOscRight = motion.curAnim.joints[1].pitch *
								motion.curAnim.harmonics.rightUpperLeg.calculate(filter.degToRad(cycle * adjFreq +
								motion.curAnim.joints[1].pitchPhase + reverseModifier));
			} else {

				pitchOscLeft = motion.curAnim.joints[1].pitch * Math.sin(filter.degToRad(cycle * adjFreq
											+ motion.curAnim.joints[1].pitchPhase));
				pitchOscRight = motion.curAnim.joints[1].pitch * Math.sin(filter.degToRad(cycle * adjFreq
											+ motion.curAnim.joints[1].pitchPhase));
			}

			yawOsc = motion.curAnim.joints[1].yaw *
				Math.sin(filter.degToRad((cycle * adjFreq) +
					motion.curAnim.joints[1].yawPhase));

			rollOsc = motion.curAnim.joints[1].roll *
				Math.sin(filter.degToRad((cycle * adjFreq) +
					motion.curAnim.joints[1].rollPhase));

			pitchOffset = motion.curAnim.joints[1].pitchOffset;
			yawOffset = motion.curAnim.joints[1].yawOffset;
			rollOffset = motion.curAnim.joints[1].rollOffset;
		}

		// apply the upper leg rotations
		MyAvatar.setJointData("LeftUpLeg", Quat.fromPitchYawRollDegrees(
			pitchOscLeft + pitchOffset,
			yawOsc - yawOffset,
			-rollOsc + rollOffset));

		MyAvatar.setJointData("RightUpLeg", Quat.fromPitchYawRollDegrees(
			pitchOscRight + pitchOffset,
			yawOsc + yawOffset,
			-rollOsc - rollOffset));

		// lower leg
		if (motion.curTransition !== nullTransition) {

			if (motion.curTransition.walkingAtStart) {

				pitchOscLeft = motion.curAnim.joints[2].pitch *
					Math.sin(filter.degToRad(motion.cumulativeTime *
					motion.curAnim.calibration.frequency +
					motion.curAnim.joints[2].pitchPhase + 180));

				pitchOscRight = motion.curAnim.joints[2].pitch *
					Math.sin(filter.degToRad(motion.cumulativeTime *
					motion.curAnim.calibration.frequency +
					motion.curAnim.joints[2].pitchPhase));

				yawOsc = motion.curAnim.joints[2].yaw *
					Math.sin(filter.degToRad(motion.cumulativeTime *
					motion.curAnim.calibration.frequency +
					motion.curAnim.joints[2].yawPhase));

				rollOsc = motion.curAnim.joints[2].roll *
					Math.sin(filter.degToRad(motion.cumulativeTime *
					motion.curAnim.calibration.frequency +
					motion.curAnim.joints[2].rollPhase));

				pitchOffset = motion.curAnim.joints[2].pitchOffset;
				yawOffset = motion.curAnim.joints[2].yawOffset;
				rollOffset = motion.curAnim.joints[2].rollOffset;

				pitchOscLeftLast = motion.curTransition.lastAnim.joints[2].pitch *
					motion.curTransition.lastAnim.harmonics.leftLowerLeg.calculate(filter.degToRad(motion.walkWheelPos +
					motion.curTransition.lastAnim.joints[2].pitchPhase + 180));

				pitchOscRightLast = motion.curTransition.lastAnim.joints[2].pitch *
					motion.curTransition.lastAnim.harmonics.leftLowerLeg.calculate(filter.degToRad(motion.walkWheelPos +
					motion.curTransition.lastAnim.joints[2].pitchPhase));

				yawOscLast = motion.curTransition.lastAnim.joints[2].yaw *
					Math.sin(filter.degToRad(motion.walkWheelPos +
					motion.curTransition.lastAnim.joints[2].yawPhase));

				rollOscLast = motion.curTransition.lastAnim.joints[2].roll *
					Math.sin(filter.degToRad(motion.walkWheelPos +
					motion.curTransition.lastAnim.joints[2].rollPhase));

				pitchOffsetLast = motion.curTransition.lastAnim.joints[2].pitchOffset;
				yawOffsetLast = motion.curTransition.lastAnim.joints[2].yawOffset;
				rollOffsetLast = motion.curTransition.lastAnim.joints[2].rollOffset;

			} else {

				if (state.currentState === state.WALKING ||
				   state.currentState === state.EDIT_WALK_STYLES ||
				   state.currentState === state.EDIT_WALK_TWEAKS ||
				   state.currentState === state.EDIT_WALK_JOINTS) {

					pitchOscLeft = motion.curAnim.harmonics.leftLowerLeg.calculate(filter.degToRad(reverseSignModifier * cycle * adjFreq +
								   motion.curAnim.joints[2].pitchPhase + 180));
					pitchOscRight = motion.curAnim.harmonics.rightLowerLeg.calculate(filter.degToRad(reverseSignModifier * cycle * adjFreq +
									motion.curAnim.joints[2].pitchPhase));

				} else {

					pitchOscLeft = Math.sin(filter.degToRad((cycle * adjFreq) +
								   motion.curAnim.joints[2].pitchPhase + 180));

					pitchOscRight = Math.sin(filter.degToRad((cycle * adjFreq) +
									motion.curAnim.joints[2].pitchPhase));
				}
				pitchOscLeft *= motion.curAnim.joints[2].pitch;
				pitchOscRight *= motion.curAnim.joints[2].pitch;

				yawOsc = motion.curAnim.joints[2].yaw *
						 Math.sin(filter.degToRad(cycle * adjFreq +
						 motion.curAnim.joints[2].yawPhase));

				rollOsc = motion.curAnim.joints[2].roll *
						  Math.sin(filter.degToRad(cycle * adjFreq +
						  motion.curAnim.joints[2].rollPhase));

				pitchOffset = motion.curAnim.joints[2].pitchOffset;
				yawOffset = motion.curAnim.joints[2].yawOffset;
				rollOffset = motion.curAnim.joints[2].rollOffset;

				pitchOscLeftLast = motion.curTransition.lastAnim.joints[2].pitch *
					Math.sin(filter.degToRad(motion.cumulativeTime *
					motion.curTransition.lastAnim.calibration.frequency +
					motion.curTransition.lastAnim.joints[2].pitchPhase + 180));

				pitchOscRightLast = motion.curTransition.lastAnim.joints[2].pitch *
					Math.sin(filter.degToRad(motion.cumulativeTime *
					motion.curTransition.lastAnim.calibration.frequency +
					motion.curTransition.lastAnim.joints[2].pitchPhase));

				yawOscLast = motion.curTransition.lastAnim.joints[2].yaw *
					Math.sin(filter.degToRad(motion.cumulativeTime *
					motion.curTransition.lastAnim.calibration.frequency +
					motion.curTransition.lastAnim.joints[2].yawPhase));

				rollOscLast = motion.curTransition.lastAnim.joints[2].roll *
					Math.sin(filter.degToRad(motion.cumulativeTime *
					motion.curTransition.lastAnim.calibration.frequency +
					motion.curTransition.lastAnim.joints[2].rollPhase));

				pitchOffsetLast = motion.curTransition.lastAnim.joints[2].pitchOffset;
				yawOffsetLast = motion.curTransition.lastAnim.joints[2].yawOffset;
				rollOffsetLast = motion.curTransition.lastAnim.joints[2].rollOffset;
			}

			pitchOscLeft = (transProgress * pitchOscLeft) + ((1 - transProgress) * pitchOscLeftLast);
			pitchOscRight = (transProgress * pitchOscRight) + ((1 - transProgress) * pitchOscRightLast);
			yawOsc = (transProgress * yawOsc) + ((1 - transProgress) * yawOscLast);
			rollOsc = (transProgress * rollOsc) + ((1 - transProgress) * rollOscLast);

			pitchOffset = (transProgress * pitchOffset) + ((1 - transProgress) * pitchOffsetLast);
			yawOffset = (transProgress * yawOffset) + ((1 - transProgress) * yawOffsetLast);
			rollOffset = (transProgress * rollOffset) + ((1 - transProgress) * rollOffsetLast);

			rollOscLeft = rollOsc;
			rollOscRight = rollOsc;

		} else { // end if transitioning

			if (state.currentState === state.WALKING ||
			   state.currentState === state.EDIT_WALK_STYLES ||
			   state.currentState === state.EDIT_WALK_TWEAKS ||
			   state.currentState === state.EDIT_WALK_JOINTS) {

				pitchOscLeft = motion.curAnim.harmonics.leftLowerLeg.calculate(filter.degToRad(reverseSignModifier * cycle * adjFreq +
							   motion.curAnim.joints[2].pitchPhase + 180));
				pitchOscRight = motion.curAnim.harmonics.rightLowerLeg.calculate(filter.degToRad(reverseSignModifier * cycle * adjFreq +
								motion.curAnim.joints[2].pitchPhase));

			} else {

				pitchOscLeft = Math.sin(filter.degToRad((cycle * adjFreq) +
							   motion.curAnim.joints[2].pitchPhase + 180));

				pitchOscRight = Math.sin(filter.degToRad((cycle * adjFreq) +
								motion.curAnim.joints[2].pitchPhase));
			}

			pitchOscLeft *= motion.curAnim.joints[2].pitch;
			pitchOscRight *= motion.curAnim.joints[2].pitch;

			yawOsc = motion.curAnim.joints[2].yaw *
				Math.sin(filter.degToRad((cycle * adjFreq) +
					motion.curAnim.joints[2].yawPhase));

			rollOsc = Math.sin(filter.degToRad((cycle * adjFreq) +
						   motion.curAnim.joints[2].rollPhase));

			rollOsc = motion.curAnim.joints[2].roll;

			pitchOffset = motion.curAnim.joints[2].pitchOffset;
			yawOffset = motion.curAnim.joints[2].yawOffset;
			rollOffset = motion.curAnim.joints[2].rollOffset;
		}

		pitchOscLeft += pitchOffset;
		pitchOscRight += pitchOffset;

		// apply lower leg joint rotations
		MyAvatar.setJointData("LeftLeg", Quat.fromPitchYawRollDegrees(
			pitchOscLeft,
			yawOsc + yawOffset,
			rollOsc + rollOffset));
		MyAvatar.setJointData("RightLeg", Quat.fromPitchYawRollDegrees(
			pitchOscRight,
			yawOsc - yawOffset,
			rollOsc - rollOffset));

	} // end if !state.SIDE_STEP

	else if (state.currentState === state.SIDE_STEP ||
		state.currentState === state.EDIT_SIDESTEP_LEFT ||
		state.currentState === state.EDIT_SIDESTEP_RIGHT) {

		// sidestepping uses the sinewave generators slightly differently for the legs
		pitchOsc = motion.curAnim.joints[1].pitch *
			Math.sin(filter.degToRad((cycle * adjFreq) +
			motion.curAnim.joints[1].pitchPhase));

		yawOsc = motion.curAnim.joints[1].yaw *
			Math.sin(filter.degToRad((cycle * adjFreq) +
			motion.curAnim.joints[1].yawPhase));

		rollOsc = motion.curAnim.joints[1].roll *
			Math.sin(filter.degToRad((cycle * adjFreq) +
			motion.curAnim.joints[1].rollPhase));

		// apply upper leg rotations for sidestepping
		MyAvatar.setJointData("RightUpLeg", Quat.fromPitchYawRollDegrees(
			-pitchOsc + motion.curAnim.joints[1].pitchOffset,
			yawOsc + motion.curAnim.joints[1].yawOffset,
			rollOsc + motion.curAnim.joints[1].rollOffset));

		MyAvatar.setJointData("LeftUpLeg", Quat.fromPitchYawRollDegrees(
			pitchOsc + motion.curAnim.joints[1].pitchOffset,
			yawOsc - motion.curAnim.joints[1].yawOffset,
			-rollOsc - motion.curAnim.joints[1].rollOffset));

		// calculate lower leg joint rotations for sidestepping
		pitchOsc = motion.curAnim.joints[2].pitch *
			Math.sin(filter.degToRad((cycle * adjFreq) +
				motion.curAnim.joints[2].pitchPhase));

		yawOsc = motion.curAnim.joints[2].yaw *
			Math.sin(filter.degToRad((cycle * adjFreq) +
				motion.curAnim.joints[2].yawPhase));

		rollOsc = motion.curAnim.joints[2].roll *
			Math.sin(filter.degToRad((cycle * adjFreq) +
				motion.curAnim.joints[2].rollPhase));

		// apply lower leg joint rotations
		MyAvatar.setJointData("RightLeg", Quat.fromPitchYawRollDegrees(
			-pitchOsc + motion.curAnim.joints[2].pitchOffset,
			yawOsc - motion.curAnim.joints[2].yawOffset,
			rollOsc - motion.curAnim.joints[2].rollOffset));

		MyAvatar.setJointData("LeftLeg", Quat.fromPitchYawRollDegrees(
			pitchOsc + motion.curAnim.joints[2].pitchOffset,
			yawOsc + motion.curAnim.joints[2].yawOffset,
			rollOsc + motion.curAnim.joints[2].rollOffset));
	}

	// feet
	if (motion.curAnim === motion.selSideStepLeft ||
		motion.curAnim === motion.selSideStepRight ) {

		sideStepHandPitchSign = -1;
		sideStepFootPitchModifier = 0.5;
	}
	if (motion.curTransition !== nullTransition) {

		if (motion.curTransition.walkingAtStart) {

			pitchOscLeft = motion.curAnim.joints[3].pitch *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curAnim.calibration.frequency) +
				motion.curAnim.joints[3].pitchPhase) + 180);

			pitchOscRight = motion.curAnim.joints[3].pitch *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curAnim.calibration.frequency) +
				motion.curAnim.joints[3].pitchPhase));

			yawOsc = motion.curAnim.joints[3].yaw *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curAnim.calibration.frequency) +
				motion.curAnim.joints[3].yawPhase));

			rollOsc = motion.curAnim.joints[3].roll *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curAnim.calibration.frequency) +
				motion.curAnim.joints[3].rollPhase));

			pitchOffset = motion.curAnim.joints[3].pitchOffset;
			yawOffset = motion.curAnim.joints[3].yawOffset;
			rollOffset = motion.curAnim.joints[3].rollOffset;

			pitchOscLeftLast = motion.curTransition.lastAnim.joints[3].pitch *
				motion.curTransition.lastAnim.harmonics.leftFoot.calculate(filter.degToRad(reverseSignModifier * motion.walkWheelPos +
				motion.curTransition.lastAnim.joints[3].pitchPhase + reverseModifier));

			pitchOscRightLast = motion.curTransition.lastAnim.joints[3].pitch *
				motion.curTransition.lastAnim.harmonics.rightFoot.calculate(filter.degToRad(reverseSignModifier * motion.walkWheelPos +
				motion.curTransition.lastAnim.joints[3].pitchPhase + 180 + reverseModifier));

			yawOscLast = motion.curTransition.lastAnim.joints[3].yaw *
				Math.sin(filter.degToRad((motion.walkWheelPos) +
				motion.curTransition.lastAnim.joints[3].yawPhase));

			rollOscLast = motion.curTransition.lastAnim.joints[3].roll *
				Math.sin(filter.degToRad((motion.walkWheelPos) +
				motion.curTransition.lastAnim.joints[3].rollPhase));

			pitchOffsetLast = motion.curTransition.lastAnim.joints[3].pitchOffset;
			yawOffsetLast = motion.curTransition.lastAnim.joints[3].yawOffset;
			rollOffsetLast = motion.curTransition.lastAnim.joints[3].rollOffset;

		} else {

			if (state.currentState === state.WALKING ||
			   state.currentState === state.EDIT_WALK_STYLES ||
			   state.currentState === state.EDIT_WALK_TWEAKS ||
			   state.currentState === state.EDIT_WALK_JOINTS) {

				pitchOscLeft = motion.curAnim.harmonics.leftFoot.calculate(filter.degToRad(reverseSignModifier * cycle * adjFreq +
							   motion.curAnim.joints[3].pitchPhase + reverseModifier));

				pitchOscRight = motion.curAnim.harmonics.rightFoot.calculate(filter.degToRad(reverseSignModifier * cycle * adjFreq +
								motion.curAnim.joints[3].pitchPhase + 180 + reverseModifier));

			} else {

				pitchOscLeft = Math.sin(filter.degToRad(cycle * adjFreq * sideStepFootPitchModifier +
							   motion.curAnim.joints[3].pitchPhase));

				pitchOscRight = Math.sin(filter.degToRad(cycle * adjFreq * sideStepFootPitchModifier +
								motion.curAnim.joints[3].pitchPhase + 180));
			}

			yawOsc = motion.curAnim.joints[3].yaw *
				Math.sin(filter.degToRad((cycle * adjFreq) +
					motion.curAnim.joints[3].yawPhase));

			rollOsc = motion.curAnim.joints[3].roll *
				Math.sin(filter.degToRad((cycle * adjFreq) +
					motion.curAnim.joints[3].rollPhase));

			pitchOffset = motion.curAnim.joints[3].pitchOffset;
			yawOffset = motion.curAnim.joints[3].yawOffset;
			rollOffset = motion.curAnim.joints[3].rollOffset;

			pitchOscLeftLast = motion.curTransition.lastAnim.joints[3].pitch *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency) +
				motion.curTransition.lastAnim.joints[3].pitchPhase + 180));

			pitchOscRightLast = motion.curTransition.lastAnim.joints[3].pitch *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency) +
				motion.curTransition.lastAnim.joints[3].pitchPhase));

			yawOscLast = motion.curTransition.lastAnim.joints[3].yaw *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency) +
				motion.curTransition.lastAnim.joints[3].yawPhase));

			rollOscLast = motion.curTransition.lastAnim.joints[3].roll *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency) +
				motion.curTransition.lastAnim.joints[3].rollPhase));

			pitchOffsetLast = motion.curTransition.lastAnim.joints[3].pitchOffset;
			yawOffsetLast = motion.curTransition.lastAnim.joints[3].yawOffset;
			rollOffsetLast = motion.curTransition.lastAnim.joints[3].rollOffset;
		}

		pitchOscLeft = (transProgress * pitchOscLeft) + ((1 - transProgress) * pitchOscLeftLast);
		pitchOscRight = (transProgress * pitchOscRight) + ((1 - transProgress) * pitchOscRightLast);
		yawOsc = (transProgress * yawOsc) + ((1 - transProgress) * yawOscLast);
		rollOsc = (transProgress * rollOsc) + ((1 - transProgress) * rollOscLast);

		pitchOffset = (transProgress * pitchOffset) + ((1 - transProgress) * pitchOffsetLast);
		yawOffset = (transProgress * yawOffset) + ((1 - transProgress) * yawOffsetLast);
		rollOffset = (transProgress * rollOffset) + ((1 - transProgress) * rollOffsetLast);

	} else {

		if (state.currentState === state.WALKING ||
		   state.currentState === state.EDIT_WALK_STYLES ||
		   state.currentState === state.EDIT_WALK_TWEAKS ||
		   state.currentState === state.EDIT_WALK_JOINTS) {

			pitchOscLeft = motion.curAnim.harmonics.leftFoot.calculate(filter.degToRad(reverseSignModifier * cycle * adjFreq +
						   motion.curAnim.joints[3].pitchPhase + reverseModifier));

			pitchOscRight = motion.curAnim.harmonics.rightFoot.calculate(filter.degToRad(reverseSignModifier * cycle * adjFreq +
							motion.curAnim.joints[3].pitchPhase + 180 + reverseModifier));

		} else {

			pitchOscLeft = Math.sin(filter.degToRad(cycle * adjFreq * sideStepFootPitchModifier +
						   motion.curAnim.joints[3].pitchPhase));

			pitchOscRight = Math.sin(filter.degToRad(cycle * adjFreq * sideStepFootPitchModifier +
							motion.curAnim.joints[3].pitchPhase + 180));
		}

		yawOsc = Math.sin(filter.degToRad((cycle * adjFreq) +
						motion.curAnim.joints[3].yawPhase));

		pitchOscLeft *= motion.curAnim.joints[3].pitch;
		pitchOscRight *= motion.curAnim.joints[3].pitch;

		yawOsc *= motion.curAnim.joints[3].yaw;

		rollOsc = motion.curAnim.joints[3].roll *
			Math.sin(filter.degToRad((cycle * adjFreq) +
				motion.curAnim.joints[3].rollPhase));

		pitchOffset = motion.curAnim.joints[3].pitchOffset;
		yawOffset = motion.curAnim.joints[3].yawOffset;
		rollOffset = motion.curAnim.joints[3].rollOffset;
	}

	// apply foot rotations
	MyAvatar.setJointData("LeftFoot", Quat.fromPitchYawRollDegrees(
		pitchOscLeft + pitchOffset,
		yawOsc - yawOffset,
		rollOsc - rollOffset));

	MyAvatar.setJointData("RightFoot", Quat.fromPitchYawRollDegrees(
		pitchOscRight + pitchOffset,
		yawOsc + yawOffset,
		rollOsc + rollOffset));

	// play footfall sound yet? To determine this, we take the differential of the
	// foot's pitch curve to decide when the foot hits the ground.
	if (state.currentState === state.WALKING ||
		state.currentState === state.SIDE_STEP ||
		state.currentState === state.EDIT_WALK_STYLES ||
		state.currentState === state.EDIT_WALK_TWEAKS ||
		state.currentState === state.EDIT_WALK_JOINTS ||
		state.currentState === state.EDIT_SIDESTEP_LEFT ||
		state.currentState === state.EDIT_SIDESTEP_RIGHT) {

		// find dy/dx by determining the cosine wave for the foot's pitch function.
		var feetPitchDifferential = Math.cos(filter.degToRad((cycle * adjFreq) + motion.curAnim.joints[3].pitchPhase));
		var threshHold = 0.9; // sets the audio trigger point. with accuracy.
		if (feetPitchDifferential < -threshHold &&
			motion.nextStep === LEFT &&
			motion.direction !== UP &&
			motion.direction !== DOWN) {

			playFootstep(LEFT);
			motion.nextStep = RIGHT;
		} else if (feetPitchDifferential > threshHold &&
			motion.nextStep === RIGHT &&
			motion.direction !== UP &&
			motion.direction !== DOWN) {

			playFootstep(RIGHT);
			motion.nextStep = LEFT;
		}
	}

	// toes
	if (motion.curTransition !== nullTransition) {

		if (motion.curTransition.walkingAtStart) {

			pitchOsc = motion.curAnim.joints[4].pitch *
				Math.sin(filter.degToRad((2 * motion.cumulativeTime *
				motion.curAnim.calibration.frequency) +
				motion.curAnim.joints[4].pitchPhase));

			yawOsc = motion.curAnim.joints[4].yaw *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curAnim.calibration.frequency) +
				motion.curAnim.joints[4].yawPhase));

			rollOsc = motion.curAnim.joints[4].roll *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curAnim.calibration.frequency) +
				motion.curAnim.joints[4].rollPhase));

			pitchOffset = motion.curAnim.joints[4].pitchOffset;
			yawOffset = motion.curAnim.joints[4].yawOffset;
			rollOffset = motion.curAnim.joints[4].rollOffset;

			pitchOscLast = motion.curTransition.lastAnim.joints[4].pitch *
				Math.sin(filter.degToRad((2 * motion.walkWheelPos) +
					motion.curTransition.lastAnim.joints[4].pitchPhase));

			yawOscLast = motion.curTransition.lastAnim.joints[4].yaw *
				Math.sin(filter.degToRad((motion.walkWheelPos) +
					motion.curTransition.lastAnim.joints[4].yawPhase));

			rollOscLast = motion.curTransition.lastAnim.joints[4].roll *
				Math.sin(filter.degToRad((motion.walkWheelPos) +
					motion.curTransition.lastAnim.joints[4].rollPhase));

			pitchOffsetLast = motion.curTransition.lastAnim.joints[4].pitchOffset;
			yawOffsetLast = motion.curTransition.lastAnim.joints[4].yawOffset;
			rollOffsetLast = motion.curTransition.lastAnim.joints[4].rollOffset;

		} else {

			pitchOsc = motion.curAnim.joints[4].pitch *
				Math.sin(filter.degToRad((2 * cycle * adjFreq) +
					motion.curAnim.joints[4].pitchPhase));

			yawOsc = motion.curAnim.joints[4].yaw *
				Math.sin(filter.degToRad((cycle * adjFreq) +
					motion.curAnim.joints[4].yawPhase));

			rollOsc = motion.curAnim.joints[4].roll *
				Math.sin(filter.degToRad((cycle * adjFreq) +
					motion.curAnim.joints[4].rollPhase));

			pitchOffset = motion.curAnim.joints[4].pitchOffset;
			yawOffset = motion.curAnim.joints[4].yawOffset;
			rollOffset = motion.curAnim.joints[4].rollOffset;

			pitchOscLast = motion.curTransition.lastAnim.joints[4].pitch *
				Math.sin(filter.degToRad((2 * motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency) +
				motion.curTransition.lastAnim.joints[4].pitchPhase));

			yawOscLast = motion.curTransition.lastAnim.joints[4].yaw *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency) +
				motion.curTransition.lastAnim.joints[4].yawPhase));

			rollOscLast = motion.curTransition.lastAnim.joints[4].roll *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency) +
				motion.curTransition.lastAnim.joints[4].rollPhase));

			pitchOffsetLast = motion.curTransition.lastAnim.joints[4].pitchOffset;
			yawOffsetLast = motion.curTransition.lastAnim.joints[4].yawOffset;
			rollOffsetLast = motion.curTransition.lastAnim.joints[4].rollOffset;
		}

		pitchOsc = (transProgress * pitchOsc) + ((1 - transProgress) * pitchOscLast);
		yawOsc = (transProgress * yawOsc) + ((1 - transProgress) * yawOscLast);
		rollOsc = (transProgress * rollOsc) + ((1 - transProgress) * rollOscLast);

		pitchOffset = (transProgress * pitchOffset) + ((1 - transProgress) * pitchOffsetLast);
		yawOffset = (transProgress * yawOffset) + ((1 - transProgress) * yawOffsetLast);
		rollOffset = (transProgress * rollOffset) + ((1 - transProgress) * rollOffsetLast);

	} else {

		pitchOsc = motion.curAnim.joints[4].pitch *
			Math.sin(filter.degToRad((2 * cycle * adjFreq) +
				motion.curAnim.joints[4].pitchPhase));

		yawOsc = motion.curAnim.joints[4].yaw *
			Math.sin(filter.degToRad((cycle * adjFreq) +
				motion.curAnim.joints[4].yawPhase));

		rollOsc = motion.curAnim.joints[4].roll *
			Math.sin(filter.degToRad((cycle * adjFreq) +
				motion.curAnim.joints[4].rollPhase));

		pitchOffset = motion.curAnim.joints[4].pitchOffset;
		yawOffset = motion.curAnim.joints[4].yawOffset;
		rollOffset = motion.curAnim.joints[4].rollOffset;
	}

	// apply toe rotations
	MyAvatar.setJointData("RightToeBase", Quat.fromPitchYawRollDegrees(
		pitchOsc + pitchOffset,
		yawOsc + yawOffset,
		rollOsc + rollOffset));

	MyAvatar.setJointData("LeftToeBase", Quat.fromPitchYawRollDegrees(
		pitchOsc + pitchOffset,
		yawOsc - yawOffset,
		rollOsc - rollOffset));

	// spine
	if (motion.curTransition !== nullTransition) {

		if (motion.curTransition.walkingAtStart) {

			pitchOsc = motion.curAnim.joints[5].pitch *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curAnim.calibration.frequency * 2) +
				motion.curAnim.joints[5].pitchPhase)) +
				motion.curAnim.joints[5].pitchOffset;

			yawOsc = motion.curAnim.joints[5].yaw *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curAnim.calibration.frequency) +
				motion.curAnim.joints[5].yawPhase)) +
				motion.curAnim
				.joints[5].yawOffset;

			rollOsc = motion.curAnim.joints[5].roll *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curAnim.calibration.frequency) +
				motion.curAnim.joints[5].rollPhase)) +
				motion.curAnim.joints[5].rollOffset;

			pitchOscLast = motion.curTransition.lastAnim.joints[5].pitch *
				Math.sin(filter.degToRad((motion.walkWheelPos * 2) +
				motion.curTransition.lastAnim.joints[5].pitchPhase)) +
				motion.curTransition.lastAnim.joints[5].pitchOffset;

			yawOscLast = motion.curTransition.lastAnim.joints[5].yaw *
				Math.sin(filter.degToRad((motion.walkWheelPos) +
				motion.curTransition.lastAnim.joints[5].yawPhase)) +
				motion.curTransition.lastAnim.joints[5].yawOffset;

			rollOscLast = motion.curTransition.lastAnim.joints[5].roll *
				Math.sin(filter.degToRad((motion.walkWheelPos) +
				motion.curTransition.lastAnim.joints[5].rollPhase)) +
				motion.curTransition.lastAnim.joints[5].rollOffset;
		} else {

			pitchOsc = motion.curAnim.joints[5].pitch *
				Math.sin(filter.degToRad((cycle * adjFreq * 2) +
				motion.curAnim.joints[5].pitchPhase)) +
				motion.curAnim.joints[5].pitchOffset;

			yawOsc = motion.curAnim.joints[5].yaw *
				Math.sin(filter.degToRad((cycle * adjFreq) +
				motion.curAnim.joints[5].yawPhase)) +
				motion.curAnim.joints[5].yawOffset;

			rollOsc = motion.curAnim.joints[5].roll *
				Math.sin(filter.degToRad((cycle * adjFreq) +
				motion.curAnim.joints[5].rollPhase)) +
				motion.curAnim.joints[5].rollOffset;

			pitchOscLast = motion.curTransition.lastAnim.joints[5].pitch *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency * 2) +
				motion.curTransition.lastAnim.joints[5].pitchPhase)) +
				motion.curTransition.lastAnim.joints[5].pitchOffset;

			yawOscLast = motion.curTransition.lastAnim.joints[5].yaw *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency) +
				motion.curTransition.lastAnim.joints[5].yawPhase)) +
				motion.curTransition.lastAnim.joints[5].yawOffset;

			rollOscLast = motion.curTransition.lastAnim.joints[5].roll *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency) +
				motion.curTransition.lastAnim.joints[5].rollPhase)) +
				motion.curTransition.lastAnim.joints[5].rollOffset;
		}

		pitchOsc = (transProgress * pitchOsc) + ((1 - transProgress) * pitchOscLast);
		yawOsc = (transProgress * yawOsc) + ((1 - transProgress) * yawOscLast);
		rollOsc = (transProgress * rollOsc) + ((1 - transProgress) * rollOscLast);

	} else {

		pitchOsc = motion.curAnim.joints[5].pitch *
			Math.sin(filter.degToRad((cycle * adjFreq * 2) +
			motion.curAnim.joints[5].pitchPhase)) +
			motion.curAnim.joints[5].pitchOffset;

		yawOsc = motion.curAnim.joints[5].yaw *
			Math.sin(filter.degToRad((cycle * adjFreq) +
			motion.curAnim.joints[5].yawPhase)) +
			motion.curAnim.joints[5].yawOffset;

		rollOsc = motion.curAnim.joints[5].roll *
			Math.sin(filter.degToRad((cycle * adjFreq) +
			motion.curAnim.joints[5].rollPhase)) +
			motion.curAnim.joints[5].rollOffset;
	}

	// apply spine joint rotations
	MyAvatar.setJointData("Spine", Quat.fromPitchYawRollDegrees(pitchOsc, yawOsc, rollOsc));

	// spine 1
	if (motion.curTransition !== nullTransition) {

		if (motion.curTransition.walkingAtStart) {

			pitchOsc = motion.curAnim.joints[6].pitch *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curAnim.calibration.frequency * 2) +
				motion.curAnim.joints[6].pitchPhase)) +
				motion.curAnim.joints[6].pitchOffset;

			yawOsc = motion.curAnim.joints[6].yaw *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curAnim.calibration.frequency) +
				motion.curAnim.joints[6].yawPhase)) +
				motion.curAnim.joints[6].yawOffset;

			rollOsc = motion.curAnim.joints[6].roll *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curAnim.calibration.frequency) +
				motion.curAnim.joints[6].rollPhase)) +
				motion.curAnim.joints[6].rollOffset;

			pitchOscLast = motion.curTransition.lastAnim.joints[6].pitch *
				Math.sin(filter.degToRad((motion.walkWheelPos * 2) +
				motion.curTransition.lastAnim.joints[6].pitchPhase)) +
				motion.curTransition.lastAnim.joints[6].pitchOffset;

			yawOscLast = motion.curTransition.lastAnim.joints[6].yaw *
				Math.sin(filter.degToRad((motion.walkWheelPos) +
				motion.curTransition.lastAnim.joints[6].yawPhase)) +
				motion.curTransition.lastAnim.joints[6].yawOffset;

			rollOscLast = motion.curTransition.lastAnim.joints[6].roll *
				Math.sin(filter.degToRad((motion.walkWheelPos) +
				motion.curTransition.lastAnim.joints[6].rollPhase)) +
				motion.curTransition.lastAnim.joints[6].rollOffset;

		} else {

			pitchOsc = motion.curAnim.joints[6].pitch *
				Math.sin(filter.degToRad((cycle * adjFreq * 2) +
				motion.curAnim.joints[6].pitchPhase)) +
				motion.curAnim.joints[6].pitchOffset;

			yawOsc = motion.curAnim.joints[6].yaw *
				Math.sin(filter.degToRad((cycle * adjFreq) +
				motion.curAnim.joints[6].yawPhase)) +
				motion.curAnim.joints[6].yawOffset;

			rollOsc = motion.curAnim.joints[6].roll *
				Math.sin(filter.degToRad((cycle * adjFreq) +
				motion.curAnim.joints[6].rollPhase)) +
				motion.curAnim.joints[6].rollOffset;

			pitchOscLast = motion.curTransition.lastAnim.joints[6].pitch *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency * 2) +
				motion.curTransition.lastAnim.joints[6].pitchPhase)) +
				motion.curTransition.lastAnim.joints[6].pitchOffset;

			yawOscLast = motion.curTransition.lastAnim.joints[6].yaw *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency) +
				motion.curTransition.lastAnim.joints[6].yawPhase)) +
				motion.curTransition.lastAnim.joints[6].yawOffset;

			rollOscLast = motion.curTransition.lastAnim.joints[6].roll *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency) +
				motion.curTransition.lastAnim.joints[6].rollPhase)) +
				motion.curTransition.lastAnim.joints[6].rollOffset;
		}

		pitchOsc = (transProgress * pitchOsc) + ((1 - transProgress) * pitchOscLast);
		yawOsc = (transProgress * yawOsc) + ((1 - transProgress) * yawOscLast);
		rollOsc = (transProgress * rollOsc) + ((1 - transProgress) * rollOscLast);

	} else {

		pitchOsc = motion.curAnim.joints[6].pitch *
			Math.sin(filter.degToRad((cycle * adjFreq * 2) +
			motion.curAnim.joints[6].pitchPhase)) +
			motion.curAnim.joints[6].pitchOffset;

		yawOsc = motion.curAnim.joints[6].yaw *
			Math.sin(filter.degToRad((cycle * adjFreq) +
			motion.curAnim.joints[6].yawPhase)) +
			motion.curAnim.joints[6].yawOffset;

		rollOsc = motion.curAnim.joints[6].roll *
			Math.sin(filter.degToRad((cycle * adjFreq) +
			motion.curAnim.joints[6].rollPhase)) +
			motion.curAnim.joints[6].rollOffset;
	}

	// apply spine1 joint rotations
	MyAvatar.setJointData("Spine1", Quat.fromPitchYawRollDegrees(pitchOsc, yawOsc, rollOsc));

	// spine 2
	if (motion.curTransition !== nullTransition) {

		if (motion.curTransition.walkingAtStart) {

			pitchOsc = motion.curAnim.joints[7].pitch *
					   Math.sin(filter.degToRad(motion.cumulativeTime *
					   motion.curAnim.calibration.frequency * 2 +
					   motion.curAnim.joints[7].pitchPhase)) +
					   motion.curAnim.joints[7].pitchOffset;

			yawOsc = motion.curAnim.joints[7].yaw *
					 Math.sin(filter.degToRad(motion.cumulativeTime *
					 motion.curAnim.calibration.frequency +
					 motion.curAnim.joints[7].yawPhase)) +
					 motion.curAnim.joints[7].yawOffset;

			rollOsc = motion.curAnim.joints[7].roll *
					  Math.sin(filter.degToRad(motion.cumulativeTime *
					  motion.curAnim.calibration.frequency +
					  motion.curAnim.joints[7].rollPhase)) +
					  motion.curAnim.joints[7].rollOffset;

			pitchOscLast = motion.curTransition.lastAnim.joints[7].pitch *
						   Math.sin(filter.degToRad(motion.walkWheelPos * 2 +
						   motion.curTransition.lastAnim.joints[7].pitchPhase)) +
						   motion.curTransition.lastAnim.joints[7].pitchOffset;

			yawOscLast = motion.curTransition.lastAnim.joints[7].yaw *
						 Math.sin(filter.degToRad(motion.walkWheelPos +
						 motion.curTransition.lastAnim.joints[7].yawPhase)) +
						 motion.curTransition.lastAnim.joints[7].yawOffset;

			rollOscLast = motion.curTransition.lastAnim.joints[7].roll *
						  Math.sin(filter.degToRad(motion.walkWheelPos +
						  motion.curTransition.lastAnim.joints[7].rollPhase)) +
						  motion.curTransition.lastAnim.joints[7].rollOffset;
		} else {

			pitchOsc = motion.curAnim.joints[7].pitch *
					   Math.sin(filter.degToRad(cycle * adjFreq * 2 +
					   motion.curAnim.joints[7].pitchPhase)) +
					   motion.curAnim.joints[7].pitchOffset;

			yawOsc = motion.curAnim.joints[7].yaw *
					 Math.sin(filter.degToRad(cycle * adjFreq +
					 motion.curAnim.joints[7].yawPhase)) +
					 motion.curAnim.joints[7].yawOffset;

			rollOsc = motion.curAnim.joints[7].roll *
					  Math.sin(filter.degToRad(cycle * adjFreq +
					  motion.curAnim.joints[7].rollPhase)) +
					  motion.curAnim.joints[7].rollOffset;

			pitchOscLast = motion.curTransition.lastAnim.joints[7].pitch *
						   Math.sin(filter.degToRad(motion.cumulativeTime * 2 *
						   motion.curTransition.lastAnim.calibration.frequency +
						   motion.curTransition.lastAnim.joints[7].pitchPhase)) +
						   motion.curTransition.lastAnim.joints[7].pitchOffset;

			yawOscLast = motion.curTransition.lastAnim.joints[7].yaw *
						 Math.sin(filter.degToRad(motion.cumulativeTime *
						 motion.curTransition.lastAnim.calibration.frequency +
						 motion.curTransition.lastAnim.joints[7].yawPhase)) +
						 motion.curTransition.lastAnim.joints[7].yawOffset;

			rollOscLast = motion.curTransition.lastAnim.joints[7].roll *
						  Math.sin(filter.degToRad(motion.cumulativeTime *
						  motion.curTransition.lastAnim.calibration.frequency +
						  motion.curTransition.lastAnim.joints[7].rollPhase)) +
						  motion.curTransition.lastAnim.joints[7].rollOffset;
		}

		pitchOsc = (transProgress * pitchOsc) + ((1 - transProgress) * pitchOscLast);
		yawOsc = (transProgress * yawOsc) + ((1 - transProgress) * yawOscLast);
		rollOsc = (transProgress * rollOsc) + ((1 - transProgress) * rollOscLast);

	} else {

		pitchOsc = motion.curAnim.joints[7].pitch *
				   Math.sin(filter.degToRad(cycle * adjFreq * 2 +
				   motion.curAnim.joints[7].pitchPhase)) +
				   motion.curAnim.joints[7].pitchOffset;

		yawOsc = motion.curAnim.joints[7].yaw *
				 Math.sin(filter.degToRad(cycle * adjFreq +
				 motion.curAnim.joints[7].yawPhase)) +
				 motion.curAnim.joints[7].yawOffset;

		rollOsc = motion.curAnim.joints[7].roll *
				  Math.sin(filter.degToRad(cycle * adjFreq +
				  motion.curAnim.joints[7].rollPhase)) +
				  motion.curAnim.joints[7].rollOffset;
	}

	// apply spine2 joint rotations
	MyAvatar.setJointData("Spine2", Quat.fromPitchYawRollDegrees(pitchOsc, yawOsc, rollOsc));

	if (!motion.armsFree) {

		// shoulders
		if (motion.curTransition !== nullTransition) {

			if (motion.curTransition.walkingAtStart) {

				pitchOsc = motion.curAnim.joints[8].pitch *
						   Math.sin(filter.degToRad(motion.cumulativeTime *
						   motion.curAnim.calibration.frequency +
						   motion.curAnim.joints[8].pitchPhase)) +
						   motion.curAnim.joints[8].pitchOffset;

				yawOsc = motion.curAnim.joints[8].yaw *
						 Math.sin(filter.degToRad(motion.cumulativeTime *
						 motion.curAnim.calibration.frequency +
						 motion.curAnim.joints[8].yawPhase));

				rollOsc = motion.curAnim.joints[8].roll *
						  Math.sin(filter.degToRad(motion.cumulativeTime *
						  motion.curAnim.calibration.frequency +
						  motion.curAnim.joints[8].rollPhase)) +
						  motion.curAnim.joints[8].rollOffset;

				yawOffset = motion.curAnim.joints[8].yawOffset;

				pitchOscLast = motion.curTransition.lastAnim.joints[8].pitch *
							   Math.sin(filter.degToRad(motion.walkWheelPos +
							   motion.curTransition.lastAnim.joints[8].pitchPhase)) +
							   motion.curTransition.lastAnim.joints[8].pitchOffset;

				yawOscLast = motion.curTransition.lastAnim.joints[8].yaw *
							 Math.sin(filter.degToRad(motion.walkWheelPos +
							 motion.curTransition.lastAnim.joints[8].yawPhase))

				rollOscLast = motion.curTransition.lastAnim.joints[8].roll *
							  Math.sin(filter.degToRad(motion.walkWheelPos +
							  motion.curTransition.lastAnim.joints[8].rollPhase)) +
							  motion.curTransition.lastAnim.joints[8].rollOffset;

				yawOffsetLast = motion.curTransition.lastAnim.joints[8].yawOffset;

			} else {

				pitchOsc = motion.curAnim.joints[8].pitch *
						   Math.sin(filter.degToRad((cycle * adjFreq) +
						   motion.curAnim.joints[8].pitchPhase)) +
						   motion.curAnim.joints[8].pitchOffset;

				yawOsc = motion.curAnim.joints[8].yaw *
						 Math.sin(filter.degToRad((cycle * adjFreq) +
						 motion.curAnim.joints[8].yawPhase));

				rollOsc = motion.curAnim.joints[8].roll *
						  Math.sin(filter.degToRad((cycle * adjFreq) +
						  motion.curAnim.joints[8].rollPhase)) +
						  motion.curAnim.joints[8].rollOffset;

				yawOffset = motion.curAnim.joints[8].yawOffset;

				pitchOscLast = motion.curTransition.lastAnim.joints[8].pitch *
							   Math.sin(filter.degToRad(motion.cumulativeTime *
							   motion.curTransition.lastAnim.calibration.frequency +
							   motion.curTransition.lastAnim.joints[8].pitchPhase)) +
							   motion.curTransition.lastAnim.joints[8].pitchOffset;

				yawOscLast = motion.curTransition.lastAnim.joints[8].yaw *
							 Math.sin(filter.degToRad(motion.cumulativeTime *
							 motion.curTransition.lastAnim.calibration.frequency +
							 motion.curTransition.lastAnim.joints[8].yawPhase))

				rollOscLast = motion.curTransition.lastAnim.joints[8].roll *
							  Math.sin(filter.degToRad(motion.cumulativeTime *
							  motion.curTransition.lastAnim.calibration.frequency +
							  motion.curTransition.lastAnim.joints[8].rollPhase)) +
							  motion.curTransition.lastAnim.joints[8].rollOffset;

				yawOffsetLast = motion.curTransition.lastAnim.joints[8].yawOffset;
			}

			pitchOsc = (transProgress * pitchOsc) + ((1 - transProgress) * pitchOscLast);
			yawOsc = (transProgress * yawOsc) + ((1 - transProgress) * yawOscLast);
			rollOsc = (transProgress * rollOsc) + ((1 - transProgress) * rollOscLast);

			yawOffset = (transProgress * yawOffset) + ((1 - transProgress) * yawOffsetLast);

		} else {

			pitchOsc = motion.curAnim.joints[8].pitch *
					   Math.sin(filter.degToRad((cycle * adjFreq) +
					   motion.curAnim.joints[8].pitchPhase)) +
					   motion.curAnim.joints[8].pitchOffset;

			yawOsc = motion.curAnim.joints[8].yaw *
					 Math.sin(filter.degToRad((cycle * adjFreq) +
					 motion.curAnim.joints[8].yawPhase));

			rollOsc = motion.curAnim.joints[8].roll *
					  Math.sin(filter.degToRad((cycle * adjFreq) +
					  motion.curAnim.joints[8].rollPhase)) +
					  motion.curAnim.joints[8].rollOffset;

			yawOffset = motion.curAnim.joints[8].yawOffset;
		}

		MyAvatar.setJointData("RightShoulder", Quat.fromPitchYawRollDegrees(pitchOsc, yawOsc + yawOffset, rollOsc));
		MyAvatar.setJointData("LeftShoulder", Quat.fromPitchYawRollDegrees(pitchOsc, yawOsc - yawOffset, -rollOsc));

		// upper arms
		if (motion.curTransition !== nullTransition) {

			if (motion.curTransition.walkingAtStart) {

				pitchOsc = motion.curAnim.joints[9].pitch *
						   Math.sin(filter.degToRad((motion.cumulativeTime *
						   motion.curAnim.calibration.frequency) +
						   motion.curAnim.joints[9].pitchPhase));

				yawOsc = motion.curAnim.joints[9].yaw *
						 Math.sin(filter.degToRad((motion.cumulativeTime *
						 motion.curAnim.calibration.frequency) +
						 motion.curAnim.joints[9].yawPhase));

				rollOsc = motion.curAnim.joints[9].roll *
						  Math.sin(filter.degToRad((motion.cumulativeTime *
						  motion.curAnim.calibration.frequency * 2) +
						  motion.curAnim.joints[9].rollPhase)) +
						  motion.curAnim.joints[9].rollOffset;

				pitchOffset = motion.curAnim.joints[9].pitchOffset;
				yawOffset = motion.curAnim.joints[9].yawOffset;

				pitchOscLast = motion.curTransition.lastAnim.joints[9].pitch *
							   Math.sin(filter.degToRad(motion.walkWheelPos +
							   motion.curTransition.lastAnim.joints[9].pitchPhase));

				yawOscLast = motion.curTransition.lastAnim.joints[9].yaw *
							 Math.sin(filter.degToRad(motion.walkWheelPos +
							 motion.curTransition.lastAnim.joints[9].yawPhase));

				rollOscLast = motion.curTransition.lastAnim.joints[9].roll *
							  Math.sin(filter.degToRad(motion.walkWheelPos +
							  motion.curTransition.lastAnim.joints[9].rollPhase)) +
							  motion.curTransition.lastAnim.joints[9].rollOffset;

				pitchOffsetLast = motion.curTransition.lastAnim.joints[9].pitchOffset;
				yawOffsetLast = motion.curTransition.lastAnim.joints[9].yawOffset;

			} else {

				pitchOsc = motion.curAnim.joints[9].pitch *
						   Math.sin(filter.degToRad((cycle * adjFreq) +
						   motion.curAnim.joints[9].pitchPhase));

				yawOsc = motion.curAnim.joints[9].yaw *
						 Math.sin(filter.degToRad((cycle * adjFreq) +
						 motion.curAnim.joints[9].yawPhase));

				rollOsc = motion.curAnim.joints[9].roll *
						  Math.sin(filter.degToRad((cycle * adjFreq * 2) +
						  motion.curAnim.joints[9].rollPhase)) +
						  motion.curAnim.joints[9].rollOffset;

				pitchOffset = motion.curAnim.joints[9].pitchOffset;
				yawOffset = motion.curAnim.joints[9].yawOffset;

				pitchOscLast = motion.curTransition.lastAnim.joints[9].pitch *
							   Math.sin(filter.degToRad(motion.cumulativeTime *
							   motion.curTransition.lastAnim.calibration.frequency +
							   motion.curTransition.lastAnim.joints[9].pitchPhase))

				yawOscLast = motion.curTransition.lastAnim.joints[9].yaw *
							 Math.sin(filter.degToRad(motion.cumulativeTime *
							 motion.curTransition.lastAnim.calibration.frequency +
							 motion.curTransition.lastAnim.joints[9].yawPhase))

				rollOscLast = motion.curTransition.lastAnim.joints[9].roll *
							  Math.sin(filter.degToRad(motion.cumulativeTime *
							  motion.curTransition.lastAnim.calibration.frequency +
							  motion.curTransition.lastAnim.joints[9].rollPhase)) +
							  motion.curTransition.lastAnim.joints[9].rollOffset;

				pitchOffsetLast = motion.curTransition.lastAnim.joints[9].pitchOffset;
				yawOffsetLast = motion.curTransition.lastAnim.joints[9].yawOffset;
			}

			pitchOsc = (transProgress * pitchOsc) + ((1 - transProgress) * pitchOscLast);
			yawOsc = (transProgress * yawOsc) + ((1 - transProgress) * yawOscLast);
			rollOsc = (transProgress * rollOsc) + ((1 - transProgress) * rollOscLast);

			pitchOffset = (transProgress * pitchOffset) + ((1 - transProgress) * pitchOffsetLast);
			yawOffset = (transProgress * yawOffset) + ((1 - transProgress) * yawOffsetLast);

		} else {

			pitchOsc = motion.curAnim.joints[9].pitch *
					   Math.sin(filter.degToRad((cycle * adjFreq) +
					   motion.curAnim.joints[9].pitchPhase));

			yawOsc = motion.curAnim.joints[9].yaw *
					 Math.sin(filter.degToRad((cycle * adjFreq) +
					 motion.curAnim.joints[9].yawPhase));

			rollOsc = motion.curAnim.joints[9].roll *
					  Math.sin(filter.degToRad((cycle * adjFreq * 2) +
					  motion.curAnim.joints[9].rollPhase)) +
					  motion.curAnim.joints[9].rollOffset;

			pitchOffset = motion.curAnim.joints[9].pitchOffset;
			yawOffset = motion.curAnim.joints[9].yawOffset;

		}

		MyAvatar.setJointData("RightArm", Quat.fromPitchYawRollDegrees(
											(-1 * flyingModifier) * pitchOsc + pitchOffset,
											yawOsc - yawOffset,
											rollOsc));

		MyAvatar.setJointData("LeftArm", Quat.fromPitchYawRollDegrees(
											pitchOsc + pitchOffset,
											yawOsc + yawOffset,
											-rollOsc));

		// forearms
		if (motion.curTransition !== nullTransition) {

			if (motion.curTransition.walkingAtStart) {

				pitchOsc = motion.curAnim.joints[10].pitch *
						   Math.sin(filter.degToRad((motion.cumulativeTime *
						   motion.curAnim.calibration.frequency) +
						   motion.curAnim.joints[10].pitchPhase)) +
						   motion.curAnim.joints[10].pitchOffset;

				yawOsc = motion.curAnim.joints[10].yaw *
						 Math.sin(filter.degToRad((motion.cumulativeTime *
						 motion.curAnim.calibration.frequency) +
						 motion.curAnim.joints[10].yawPhase));

				rollOsc = motion.curAnim.joints[10].roll *
						  Math.sin(filter.degToRad((motion.cumulativeTime *
						  motion.curAnim.calibration.frequency) +
						  motion.curAnim.joints[10].rollPhase));

				yawOffset = motion.curAnim.joints[10].yawOffset;
				rollOffset = motion.curAnim.joints[10].rollOffset;

				pitchOscLast = motion.curTransition.lastAnim.joints[10].pitch *
							   Math.sin(filter.degToRad((motion.walkWheelPos) +
							   motion.curTransition.lastAnim.joints[10].pitchPhase)) +
							   motion.curTransition.lastAnim.joints[10].pitchOffset;

				yawOscLast = motion.curTransition.lastAnim.joints[10].yaw *
							 Math.sin(filter.degToRad((motion.walkWheelPos) +
							 motion.curTransition.lastAnim.joints[10].yawPhase));

				rollOscLast = motion.curTransition.lastAnim.joints[10].roll *
							  Math.sin(filter.degToRad((motion.walkWheelPos) +
							  motion.curTransition.lastAnim.joints[10].rollPhase));

				yawOffsetLast = motion.curTransition.lastAnim.joints[10].yawOffset;
				rollOffsetLast = motion.curTransition.lastAnim.joints[10].rollOffset;

			} else {

				pitchOsc = motion.curAnim.joints[10].pitch *
						   Math.sin(filter.degToRad((cycle * adjFreq) +
						   motion.curAnim.joints[10].pitchPhase)) +
						   motion.curAnim.joints[10].pitchOffset;

				yawOsc = motion.curAnim.joints[10].yaw *
						 Math.sin(filter.degToRad((cycle * adjFreq) +
						 motion.curAnim.joints[10].yawPhase));

				rollOsc = motion.curAnim.joints[10].roll *
						  Math.sin(filter.degToRad((cycle * adjFreq) +
						  motion.curAnim.joints[10].rollPhase));

				yawOffset = motion.curAnim.joints[10].yawOffset;
				rollOffset = motion.curAnim.joints[10].rollOffset;

				pitchOscLast = motion.curTransition.lastAnim.joints[10].pitch *
							   Math.sin(filter.degToRad((motion.cumulativeTime *
							   motion.curTransition.lastAnim.calibration.frequency) +
							   motion.curTransition.lastAnim.joints[10].pitchPhase)) +
							   motion.curTransition.lastAnim.joints[10].pitchOffset;

				yawOscLast = motion.curTransition.lastAnim.joints[10].yaw *
							 Math.sin(filter.degToRad((motion.cumulativeTime *
							 motion.curTransition.lastAnim.calibration.frequency) +
							 motion.curTransition.lastAnim.joints[10].yawPhase));

				rollOscLast = motion.curTransition.lastAnim.joints[10].roll *
							  Math.sin(filter.degToRad((motion.cumulativeTime *
							  motion.curTransition.lastAnim.calibration.frequency) +
							  motion.curTransition.lastAnim.joints[10].rollPhase));

				yawOffsetLast = motion.curTransition.lastAnim.joints[10].yawOffset;
				rollOffsetLast = motion.curTransition.lastAnim.joints[10].rollOffset;
			}

			// blend the animations
			pitchOsc = (transProgress * pitchOsc) + ((1 - transProgress) * pitchOscLast);
			yawOsc = -(transProgress * yawOsc) + ((1 - transProgress) * yawOscLast);
			rollOsc = (transProgress * rollOsc) + ((1 - transProgress) * rollOscLast);

			yawOffset = (transProgress * yawOffset) + ((1 - transProgress) * yawOffsetLast);
			rollOffset = (transProgress * rollOffset) + ((1 - transProgress) * rollOffsetLast);

		} else {

			pitchOsc = motion.curAnim.joints[10].pitch *
					  Math.sin(filter.degToRad((cycle * adjFreq) +
					  motion.curAnim.joints[10].pitchPhase)) +
					  motion.curAnim.joints[10].pitchOffset;

			yawOsc = motion.curAnim.joints[10].yaw *
					 Math.sin(filter.degToRad((cycle * adjFreq) +
					 motion.curAnim.joints[10].yawPhase));

			rollOsc = motion.curAnim.joints[10].roll *
					  Math.sin(filter.degToRad((cycle * adjFreq) +
					  motion.curAnim.joints[10].rollPhase));

			yawOffset = motion.curAnim.joints[10].yawOffset;
			rollOffset = motion.curAnim.joints[10].rollOffset;
		}

		// apply forearms rotations
		MyAvatar.setJointData("RightForeArm",
			Quat.fromPitchYawRollDegrees(pitchOsc, yawOsc + yawOffset, rollOsc + rollOffset));
		MyAvatar.setJointData("LeftForeArm",
			Quat.fromPitchYawRollDegrees(pitchOsc, yawOsc - yawOffset, rollOsc - rollOffset));

		// hands
		if (motion.curTransition !== nullTransition) {

			if (motion.curTransition.walkingAtStart) {

				pitchOsc = motion.curAnim.joints[11].pitch *
						   Math.sin(filter.degToRad((motion.cumulativeTime *
						   motion.curAnim.calibration.frequency) +
						   motion.curAnim.joints[11].pitchPhase)) +
						   motion.curAnim.joints[11].pitchOffset;

				yawOsc = motion.curAnim.joints[11].yaw *
						 Math.sin(filter.degToRad((motion.cumulativeTime *
						 motion.curAnim.calibration.frequency) +
						 motion.curAnim.joints[11].yawPhase)) +
						 motion.curAnim.joints[11].yawOffset;

				rollOsc = motion.curAnim.joints[11].roll *
						  Math.sin(filter.degToRad((motion.cumulativeTime *
						  motion.curAnim.calibration.frequency) +
						  motion.curAnim.joints[11].rollPhase));

				pitchOscLast = motion.curTransition.lastAnim.joints[11].pitch *
							   Math.sin(filter.degToRad(motion.walkWheelPos +
							   motion.curTransition.lastAnim.joints[11].pitchPhase)) +
							   motion.curTransition.lastAnim.joints[11].pitchOffset;

				yawOscLast = motion.curTransition.lastAnim.joints[11].yaw *
							 Math.sin(filter.degToRad(motion.walkWheelPos +
							 motion.curTransition.lastAnim.joints[11].yawPhase)) +
							 motion.curTransition.lastAnim.joints[11].yawOffset;

				rollOscLast = motion.curTransition.lastAnim.joints[11].roll *
							  Math.sin(filter.degToRad(motion.walkWheelPos +
							  motion.curTransition.lastAnim.joints[11].rollPhase))

				rollOffset = motion.curAnim.joints[11].rollOffset;
				rollOffsetLast = motion.curTransition.lastAnim.joints[11].rollOffset;

			} else {

				pitchOsc = motion.curAnim.joints[11].pitch *
						   Math.sin(filter.degToRad((cycle * adjFreq) +
						   motion.curAnim.joints[11].pitchPhase)) +
						   motion.curAnim.joints[11].pitchOffset;

				yawOsc = motion.curAnim.joints[11].yaw *
						 Math.sin(filter.degToRad((cycle * adjFreq) +
						 motion.curAnim.joints[11].yawPhase)) +
						 motion.curAnim.joints[11].yawOffset;

				rollOsc = motion.curAnim.joints[11].roll *
						  Math.sin(filter.degToRad((cycle * adjFreq) +
						  motion.curAnim.joints[11].rollPhase));

				rollOffset = motion.curAnim.joints[11].rollOffset;

				pitchOscLast = motion.curTransition.lastAnim.joints[11].pitch *
							   Math.sin(filter.degToRad(motion.cumulativeTime *
							   motion.curTransition.lastAnim.calibration.frequency +
							   motion.curTransition.lastAnim.joints[11].pitchPhase)) +
							   motion.curTransition.lastAnim.joints[11].pitchOffset;

				yawOscLast = motion.curTransition.lastAnim.joints[11].yaw *
							 Math.sin(filter.degToRad(motion.cumulativeTime *
							 motion.curTransition.lastAnim.calibration.frequency +
							 motion.curTransition.lastAnim.joints[11].yawPhase)) +
							 motion.curTransition.lastAnim.joints[11].yawOffset;

				rollOscLast = motion.curTransition.lastAnim.joints[11].roll *
							  Math.sin(filter.degToRad(motion.cumulativeTime *
							  motion.curTransition.lastAnim.calibration.frequency +
							  motion.curTransition.lastAnim.joints[11].rollPhase))

				rollOffset = motion.curAnim.joints[11].rollOffset;
				rollOffsetLast = motion.curTransition.lastAnim.joints[11].rollOffset;
			}

			pitchOsc = (transProgress * pitchOsc) + ((1 - transProgress) * pitchOscLast);
			yawOsc = (transProgress * yawOsc) + ((1 - transProgress) * yawOscLast);
			rollOsc = (transProgress * rollOsc) + ((1 - transProgress) * rollOscLast);

			rollOffset = (transProgress * rollOffset) + ((1 - transProgress) * rollOffsetLast);

		} else {

			pitchOsc = motion.curAnim.joints[11].pitch *
					   Math.sin(filter.degToRad((cycle * adjFreq) +
					   motion.curAnim.joints[11].pitchPhase)) +
					   motion.curAnim.joints[11].pitchOffset;

			yawOsc = motion.curAnim.joints[11].yaw *
					 Math.sin(filter.degToRad((cycle * adjFreq) +
					 motion.curAnim.joints[11].yawPhase)) +
					 motion.curAnim.joints[11].yawOffset;

			rollOsc = motion.curAnim.joints[11].roll *
					  Math.sin(filter.degToRad((cycle * adjFreq) +
					  motion.curAnim.joints[11].rollPhase));

			rollOffset = motion.curAnim.joints[11].rollOffset;
		}

		// set the hand rotations
		MyAvatar.setJointData("RightHand",
			Quat.fromPitchYawRollDegrees(sideStepHandPitchSign * pitchOsc, yawOsc, rollOsc + rollOffset));
		MyAvatar.setJointData("LeftHand",
			Quat.fromPitchYawRollDegrees(pitchOsc, -yawOsc, rollOsc - rollOffset));

	} // end if (!motion.armsFree)

	// head and neck
	if (motion.curTransition !== nullTransition) {

		if (motion.curTransition.walkingAtStart) {

			pitchOsc = 0.5 * motion.curAnim.joints[12].pitch *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curAnim.calibration.frequency * 2) +
				motion.curAnim.joints[12].pitchPhase)) +
				motion.curAnim.joints[12].pitchOffset;

			yawOsc = 0.5 * motion.curAnim.joints[12].yaw *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curAnim.calibration.frequency) +
				motion.curAnim.joints[12].yawPhase)) +
				motion.curAnim.joints[12].yawOffset;

			rollOsc = 0.5 * motion.curAnim.joints[12].roll *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curAnim.calibration.frequency) +
				motion.curAnim.joints[12].rollPhase)) +
				motion.curAnim.joints[12].rollOffset;

			pitchOscLast = 0.5 * motion.curTransition.lastAnim.joints[12].pitch *
				Math.sin(filter.degToRad((motion.walkWheelPos * 2) +
				motion.curTransition.lastAnim.joints[12].pitchPhase)) +
				motion.curTransition.lastAnim.joints[12].pitchOffset;

			yawOscLast = 0.5 * motion.curTransition.lastAnim.joints[12].yaw *
				Math.sin(filter.degToRad((motion.walkWheelPos) +
				motion.curTransition.lastAnim.joints[12].yawPhase)) +
				motion.curTransition.lastAnim.joints[12].yawOffset;

			rollOscLast = 0.5 * motion.curTransition.lastAnim.joints[12].roll *
				Math.sin(filter.degToRad((motion.walkWheelPos) +
				motion.curTransition.lastAnim.joints[12].rollPhase)) +
				motion.curTransition.lastAnim.joints[12].rollOffset;

		} else {

			pitchOsc = 0.5 * motion.curAnim.joints[12].pitch *
				Math.sin(filter.degToRad((cycle * adjFreq * 2) +
				motion.curAnim.joints[12].pitchPhase)) +
				motion.curAnim.joints[12].pitchOffset;

			yawOsc = 0.5 * motion.curAnim.joints[12].yaw *
				Math.sin(filter.degToRad((cycle * adjFreq) +
				motion.curAnim.joints[12].yawPhase)) +
				motion.curAnim.joints[12].yawOffset;

			rollOsc = 0.5 * motion.curAnim.joints[12].roll *
				Math.sin(filter.degToRad((cycle * adjFreq) +
				motion.curAnim.joints[12].rollPhase)) +
				motion.curAnim.joints[12].rollOffset;

			pitchOscLast = 0.5 * motion.curTransition.lastAnim.joints[12].pitch *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency * 2) +
				motion.curTransition.lastAnim.joints[12].pitchPhase)) +
				motion.curTransition.lastAnim.joints[12].pitchOffset;

			yawOscLast = 0.5 * motion.curTransition.lastAnim.joints[12].yaw *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency) +
				motion.curTransition.lastAnim.joints[12].yawPhase)) +
				motion.curTransition.lastAnim.joints[12].yawOffset;

			rollOscLast = 0.5 * motion.curTransition.lastAnim.joints[12].roll *
				Math.sin(filter.degToRad((motion.cumulativeTime *
				motion.curTransition.lastAnim.calibration.frequency) +
				motion.curTransition.lastAnim.joints[12].rollPhase)) +
				motion.curTransition.lastAnim.joints[12].rollOffset;
		}

		pitchOsc = (transProgress * pitchOsc) + ((1 - transProgress) * pitchOscLast);
		yawOsc = (transProgress * yawOsc) + ((1 - transProgress) * yawOscLast);
		rollOsc = (transProgress * rollOsc) + ((1 - transProgress) * rollOscLast);

	} else {

		pitchOsc = 0.5 * motion.curAnim.joints[12].pitch *
			Math.sin(filter.degToRad((cycle * adjFreq * 2) +
			motion.curAnim.joints[12].pitchPhase)) +
			motion.curAnim.joints[12].pitchOffset;

		yawOsc = 0.5 * motion.curAnim.joints[12].yaw *
			Math.sin(filter.degToRad((cycle * adjFreq) +
			motion.curAnim.joints[12].yawPhase)) +
			motion.curAnim.joints[12].yawOffset;

		rollOsc = 0.5 * motion.curAnim.joints[12].roll *
			Math.sin(filter.degToRad((cycle * adjFreq) +
			motion.curAnim.joints[12].rollPhase)) +
			motion.curAnim.joints[12].rollOffset;
	}

	MyAvatar.setJointData("Head", Quat.fromPitchYawRollDegrees(pitchOsc, yawOsc, rollOsc));
	MyAvatar.setJointData("Neck", Quat.fromPitchYawRollDegrees(pitchOsc, yawOsc, rollOsc));
}

// Begin by setting an internal state
state.setInternalState(state.STANDING);