//
//  walk.js
//
//  version 1.12
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
var PITCH = 64;
var YAW = 128;
var ROLL = 256;
var SAWTOOTH = 1;
var TRIANGLE = 2;
var SQUARE = 4;
var VERY_LONG_TIME = 1000000.0;
var VERY_SHORT_TIME = 0.000001;

// ovelay images location
var pathToAssets = 'http://s3.amazonaws.com/hifi-public/procedural-animator/';

// load the UI
Script.include("./libraries/walkInterface.js");

// load filters (Bezier, Butterworth, add harmonics, averaging)
Script.include("./libraries/walkFilters.js");

// load objects, constructors and assets (state, Motion, Transition, walkAssets)
Script.include("./libraries/walkApi.js");

// initialise the motion properties and history object
var motion = new Motion();

// initialise Transitions
var nullTransition = new Transition();
motion.currentTransition = nullTransition;

// initialise the UI
walkInterface.initialise(state, motion, walkAssets);

// Begin by setting the STANDING internal state
state.setInternalState(state.STANDING);

// smoothing filters
var leanPitchFilter = filter.createButterworthFilter1();
var leanPitchSmoothingFilter = filter.createAveragingFilter(10);
var leanRollSmoothingFilter = filter.createAveragingFilter(10);
var flyUpFilter = filter.createAveragingFilter(30);
var flyFilter = filter.createAveragingFilter(30);

// Main loop
Script.update.connect(function(deltaTime) {

    if (state.powerOn) {

        // calculate avi linear speed
        var speed = Vec3.length(MyAvatar.getVelocity());

        // decide which animation should be playing
        selectAnimation(deltaTime, speed);

        // turn the frequency time wheels
        turnFrequencyTimeWheels(deltaTime, speed);

        // calculate (or fetch pre-calculated) stride length for this avi
        setStrideLength(speed);

        // update the progress of any live transition
        updateTransition();

        // apply translation and rotations
        animateAvatar(deltaTime, speed);
    }
});

// helper function for selectAnimation()
function determineLocomotionMode(speed) {

    var locomotionMode = undefined;

    if (speed < 0.1 && motion.currentAnimation !== motion.selectedFlyBlend) {

        locomotionMode = state.STANDING;

    } else if (speed === 0 && motion.currentAnimation === motion.selectedFlyBlend) {

        locomotionMode = state.STANDING;

    } else if (speed < TAKE_FLIGHT_SPEED) {

        locomotionMode = state.WALKING;

    } else if (speed >= TAKE_FLIGHT_SPEED) {

        locomotionMode = state.FLYING;
    }

    // maybe travelling at walking speed, but sideways?
    if (locomotionMode === state.WALKING &&
       (motion.direction === LEFT ||
        motion.direction === RIGHT)) {

            locomotionMode = state.SIDE_STEP;
    }

    // maybe completing a final step during a walking to standing transition?
    if (locomotionMode === state.WALKING &&
        motion.currentTransition.percentToMove > 0) {

            locomotionMode = state.STANDING;
    }

    // maybe starting to fly up or down?
    if (locomotionMode === state.WALKING &&
        motion.direction === UP || motion.direction === DOWN) {

            locomotionMode = state.FLYING;
    }

    // are we on a voxel surface?
    if(spatialInformation.distanceToVoxels() > 0.2 && speed > 0.1) {

        locomotionMode = state.FLYING;
    }

    return locomotionMode;
}

// helper function for selectAnimation()
function determineDirection(localVelocity) {

    if (Math.abs(localVelocity.x) > Math.abs(localVelocity.y) &&
        Math.abs(localVelocity.x) > Math.abs(localVelocity.z)) {

        if (localVelocity.x < 0) {
            return LEFT;
        } else {
            return RIGHT;
        }

    } else if (Math.abs(localVelocity.y) > Math.abs(localVelocity.x) &&
               Math.abs(localVelocity.y) > Math.abs(localVelocity.z)) {

        if (localVelocity.y > 0) {
            return UP;
        } else {
            return DOWN;
        }

    } else if (Math.abs(localVelocity.z) > Math.abs(localVelocity.x) &&
               Math.abs(localVelocity.z) > Math.abs(localVelocity.y)) {

        if (localVelocity.z < 0) {
            return FORWARDS;
        } else {
            return BACKWARDS;
        }
    }
}

// advance the animation's frequency time wheels. advance frequency time wheels for any live transitions also
function turnFrequencyTimeWheels(deltaTime, speed) {

    var wheelAdvance = 0;

    // turn the frequency time wheel
    if (motion.currentAnimation === motion.selectedWalk ||
        motion.currentAnimation === motion.selectedSideStepLeft ||
        motion.currentAnimation === motion.selectedSideStepRight) {

        // Using technique described here: http://www.gdcvault.com/play/1020583/Animation-Bootcamp-An-Indie-Approach
        // wrap the stride length around a 'surveyor's wheel' twice and calculate the angular speed at the given (linear) speed
        // omega = v / r , where r = circumference / 2 PI , where circumference = 2 * stride length
        motion.frequencyTimeWheelRadius = motion.strideLength / Math.PI;
        var angularVelocity = speed / motion.frequencyTimeWheelRadius;

        // calculate the degrees turned (at this angular speed) since last frame
        wheelAdvance = filter.radToDeg(deltaTime * angularVelocity);

    } else {

        // turn the frequency time wheel by the amount specified for this animation
        wheelAdvance = filter.radToDeg(motion.currentAnimation.calibration.frequency * deltaTime);
    }

    if(motion.currentTransition !== nullTransition) {

        // the last animation is still playing so we turn it's frequency time wheel to maintain the animation
        if (motion.currentTransition.lastAnimation === motion.selectedWalk) {

            // if at a stop angle (i.e. feet now under the avi) hold the wheel position for remainder of transition
            var tolerance = motion.currentTransition.lastFrequencyTimeIncrement + 0.1;
            if ((motion.currentTransition.lastFrequencyTimeWheelPos >
                (motion.currentTransition.stopAngle - tolerance)) &&
                (motion.currentTransition.lastFrequencyTimeWheelPos <
                (motion.currentTransition.stopAngle + tolerance))) {

                motion.currentTransition.lastFrequencyTimeIncrement = 0;
            }
        }
        motion.currentTransition.advancePreviousFrequencyTimeWheel();
    }

    // advance the walk wheel the appropriate amount
    motion.advanceFrequencyTimeWheel(wheelAdvance);
}

// helper function for selectAnimation()
function setTransition(nextAnimation) {

    var lastTransition = motion.currentTransition;
    motion.currentTransition = new Transition(nextAnimation,
                                              motion.currentAnimation,
                                              motion.currentTransition);

    if(motion.currentTransition.recursionDepth > 5) {

        delete motion.currentTransition;
        motion.currentTransition = lastTransition;
    }
}

// select which animation should be played based on speed, mode of locomotion and direction of travel
function selectAnimation(deltaTime, speed) {

    var localVelocity = {x: 0, y: 0, z: 0};
    if (speed > 0) {
        localVelocity = Vec3.multiplyQbyV(Quat.inverse(MyAvatar.orientation), MyAvatar.getVelocity());
    }

    // determine principle direction of movement
    motion.direction = determineDirection(localVelocity);

    // determine mode of locomotion
    var locomotionMode = determineLocomotionMode(speed);

    // select appropriate animation. If changing animation, initiate a new transition
    // note: The transitions are work in progress: https://worklist.net/20186
    switch (locomotionMode) {

        case state.STANDING: {

            var onVoxelSurface = spatialInformation.distanceToVoxels() < 0.3 ? true : false;

            if (state.currentState !== state.STANDING) {

                if (onVoxelSurface) {

                    setTransition(motion.selectedStand);

                } else {

                    setTransition(motion.selectedHovering);
                }
                state.setInternalState(state.STANDING);
            }
            if (onVoxelSurface) {

                motion.currentAnimation = motion.selectedStand;

            } else {

                motion.currentAnimation = motion.selectedHovering;
            }
            break;
        }

        case state.WALKING: {

            if (state.currentState !== state.WALKING) {

                setTransition(motion.selectedWalk);
                // set the walk wheel to it's starting point for this animation
                if (motion.direction === BACKWARDS) {

                    motion.frequencyTimeWheelPos = motion.selectedWalk.calibration.startAngleBackwards;

                } else {

                    motion.frequencyTimeWheelPos = motion.selectedWalk.calibration.startAngleForwards;
                }
                state.setInternalState(state.WALKING);
            }
            motion.currentAnimation = motion.selectedWalk;
            break;
        }

        case state.SIDE_STEP: {

            var selSideStep = undefined;

            if (state.currentState !== state.SIDE_STEP) {

                if (motion.direction === LEFT) {
                    // set the walk wheel to it's starting point for this animation
                    motion.frequencyTimeWheelPos = motion.selectedSideStepLeft.calibration.startAngle;
                    selSideStep = motion.selectedSideStepLeft;

                } else {
                    // set the walk wheel to it's starting point for this animation
                    motion.frequencyTimeWheelPos = motion.selectedSideStepRight.calibration.startAngle;
                    selSideStep = motion.selectedSideStepRight;
                }
                setTransition(selSideStep);
                state.setInternalState(state.SIDE_STEP);

            } else if (motion.direction === LEFT) {

                if (motion.lastDirection !== LEFT) {
                    // set the walk wheel to it's starting point for this animation
                    motion.frequencyTimeWheelPos = motion.selectedSideStepLeft.calibration.startAngle;
                    setTransition(motion.selectedSideStepLeft);
                }
                selSideStep = motion.selectedSideStepLeft;

            } else {

                if (motion.lastDirection !== RIGHT) {
                    // set the walk wheel to it's starting point for this animation
                    motion.frequencyTimeWheelPos = motion.selectedSideStepRight.calibration.startAngle;
                    setTransition(motion.selectedSideStepRight);
                }
                selSideStep = motion.selectedSideStepRight;
            }
            motion.currentAnimation = selSideStep;
            break;
        }

        case state.FLYING: {

            // blend the up, down and forward flying animations relative to motion direction
            animation.zeroAnimation(motion.selectedFlyBlend);

            var upVector = 0;
            var forwardVector = 0;

            if (speed > 0) {

                // calculate directionals
                upVector = localVelocity.y / speed;
                forwardVector = -localVelocity.z / speed;

                // add damping
                upVector = flyUpFilter.process(upVector);
                forwardVector = flyFilter.process(forwardVector);

                // normalise
                var denominator = Math.abs(upVector) + Math.abs(forwardVector);
                upVector = upVector / denominator;
                forwardVector = forwardVector / denominator;
            }

            if (upVector > 0) {

                animation.blendAnimation(motion.selectedFlyUp,
                                         motion.selectedFlyBlend,
                                         upVector);

            } else if (upVector < 0) {

                animation.blendAnimation(motion.selectedFlyDown,
                                         motion.selectedFlyBlend,
                                         -upVector);
            } else {

                forwardVector = 1;
            }
            animation.blendAnimation(motion.selectedFly,
                                     motion.selectedFlyBlend,
                                     Math.abs(forwardVector));

            if (state.currentState !== state.FLYING) {

                state.setInternalState(state.FLYING);
                setTransition(motion.selectedFlyBlend);
            }
            motion.currentAnimation = motion.selectedFlyBlend;
            break;
        }

    } // end switch(locomotionMode)

    // record frame details for future reference
    motion.lastDirection = motion.direction;
    motion.lastSpeed = localVelocity;
    motion.lastDistanceToVoxels = motion.calibration.distanceToVoxels;

    return;
}

// if the timing's right, recalculate the stride length. if not, fetch the previously calculated value
function setStrideLength(speed) {

    // if not at full speed the calculation could be wrong, as we may still be transitioning
    var atMaxSpeed = speed / MAX_WALK_SPEED < 0.97 ? false : true;

    // walking? then get the stride length
    if (motion.currentAnimation === motion.selectedWalk) {

        var strideMaxAt = motion.currentAnimation.calibration.forwardStrideMaxAt;
        if (motion.direction === BACKWARDS) {
            strideMaxAt = motion.currentAnimation.calibration.backwardsStrideMaxAt;
        }

        var tolerance = 1.0;
        if (motion.frequencyTimeWheelPos < (strideMaxAt + tolerance) &&
            motion.frequencyTimeWheelPos > (strideMaxAt - tolerance) &&
            atMaxSpeed && motion.currentTransition === nullTransition) {

            // measure and save stride length
            var footRPos = MyAvatar.getJointPosition("RightFoot");
            var footLPos = MyAvatar.getJointPosition("LeftFoot");
            motion.strideLength = Vec3.distance(footRPos, footLPos);

            if (motion.direction === FORWARDS) {
                motion.currentAnimation.calibration.strideLengthForwards = motion.strideLength;
            } else if (motion.direction === BACKWARDS) {
                motion.currentAnimation.calibration.strideLengthBackwards = motion.strideLength;
            }

        } else {

            // use the previously saved value for stride length
            if (motion.direction === FORWARDS) {
                motion.strideLength = motion.currentAnimation.calibration.strideLengthForwards;
            } else if (motion.direction === BACKWARDS) {
                motion.strideLength = motion.currentAnimation.calibration.strideLengthBackwards;
            }
        }
    } // end get walk stride length

    // sidestepping? get the stride length
    if (motion.currentAnimation === motion.selectedSideStepLeft ||
        motion.currentAnimation === motion.selectedSideStepRight) {

        // if the timing's right, take a snapshot of the stride max and recalibrate the stride length
        var tolerance = 1.0;
        if (motion.direction === LEFT) {

            if (motion.frequencyTimeWheelPos < motion.currentAnimation.calibration.strideMaxAt + tolerance &&
                motion.frequencyTimeWheelPos > motion.currentAnimation.calibration.strideMaxAt - tolerance &&
                atMaxSpeed && motion.currentTransition === nullTransition) {

                var footRPos = MyAvatar.getJointPosition("RightFoot");
                var footLPos = MyAvatar.getJointPosition("LeftFoot");
                motion.strideLength = Vec3.distance(footRPos, footLPos);
                motion.currentAnimation.calibration.strideLength = motion.strideLength;

            } else {

                motion.strideLength = motion.selectedSideStepLeft.calibration.strideLength;
            }

        } else if (motion.direction === RIGHT) {

            if (motion.frequencyTimeWheelPos < motion.currentAnimation.calibration.strideMaxAt + tolerance &&
                motion.frequencyTimeWheelPos > motion.currentAnimation.calibration.strideMaxAt - tolerance &&
                atMaxSpeed && motion.currentTransition === nullTransition) {

                var footRPos = MyAvatar.getJointPosition("RightFoot");
                var footLPos = MyAvatar.getJointPosition("LeftFoot");
                motion.strideLength = Vec3.distance(footRPos, footLPos);
                motion.currentAnimation.calibration.strideLength = motion.strideLength;

            } else {

                motion.strideLength = motion.selectedSideStepRight.calibration.strideLength;
            }
        }
    } // end get sidestep stride length
}

// initialise a new transition. update progress of a live transition
function updateTransition() {

    if (motion.currentTransition !== nullTransition) {

        // new transition?
        if (motion.currentTransition.progress === 0) {

            // overlapping transitions?
            if (motion.currentTransition.lastTransition !== nullTransition) {

                // is the last animation for the nested transition the same as the new animation?
                if (motion.currentTransition.lastTransition.lastAnimation === motion.currentAnimation) {

                    // sync the nested transitions's frequency time wheel for a smooth animation blend
                    motion.frequencyTimeWheelPos = motion.currentTransition.lastTransition.lastFrequencyTimeWheelPos;
                }
            }

            if (motion.currentTransition.lastAnimation === motion.selectedWalk) {

                // decide at which angle we should stop the frequency time wheel
                var stopAngle = motion.selectedWalk.calibration.stopAngleForwards;
                var percentToMove = 0;
                var lastFrequencyTimeWheelPos = motion.currentTransition.lastFrequencyTimeWheelPos;
                var lastElapsedFTDegrees = motion.currentTransition.lastElapsedFTDegrees;

                // set the stop angle depending on which quadrant of the walk cycle we are currently in
                // and decide whether we need to take an extra step to complete the walk cycle or not
                // - currently work in progress
                if(lastFrequencyTimeWheelPos <= stopAngle && lastElapsedFTDegrees < 180) {

                    // we have not taken a complete step yet, so we do need to do so before stopping
                    percentToMove = 100;
                    stopAngle += 180;

                } else if(lastFrequencyTimeWheelPos > stopAngle && lastFrequencyTimeWheelPos <= stopAngle + 90) {

                    // take an extra step to complete the walk cycle and stop at the second stop angle
                    percentToMove = 100;
                    stopAngle += 180;

                } else if(lastFrequencyTimeWheelPos > stopAngle + 90 && lastFrequencyTimeWheelPos <= stopAngle + 180) {

                    // stop on the other foot at the second stop angle for this walk cycle
                    percentToMove = 0;
                    if (motion.currentTransition.lastDirection === BACKWARDS) {

                        percentToMove = 100;

                    } else {

                        stopAngle += 180;
                    }

                } else if(lastFrequencyTimeWheelPos > stopAngle + 180 && lastFrequencyTimeWheelPos <= stopAngle + 270) {

                    // take an extra step to complete the walk cycle and stop at the first stop angle
                    percentToMove = 100;
                }

                // set it all in motion
                motion.currentTransition.stopAngle = stopAngle;
                motion.currentTransition.percentToMove = percentToMove;
            }

        } // end if new transition

        // update the Transition progress
        if (motion.currentTransition.updateProgress() >= 1) {

            // it's time to kill off this transition
            delete motion.currentTransition;
            motion.currentTransition = nullTransition;
        }
    }
}

// helper function for animateAvatar(). calculate the amount to lean forwards (or backwards) based on the avi's acceleration
function getLeanPitch(speed) {

    var leanProgress = undefined;

    if (motion.direction === LEFT ||
        motion.direction === RIGHT ||
        motion.direction === UP) {

        leanProgress = 0;

    } else {

        // boost lean for flying (leaning whilst walking is work in progress)
        var signedSpeed = -Vec3.multiplyQbyV(Quat.inverse(MyAvatar.orientation), MyAvatar.getVelocity()).z;
        var reverseMod = 1;
        var BOOST = 6;

        if (signedSpeed < 0) {

            reverseMod = -1.9;
            if (motion.direction === DOWN) {

                reverseMod *= -1;
            }
        }
        leanProgress = reverseMod * BOOST * speed / TOP_SPEED;
    }

    // use filters to shape the walking acceleration response
    leanProgress = leanPitchFilter.process(leanProgress);
    leanProgress = leanPitchSmoothingFilter.process(leanProgress);
    return motion.calibration.pitchMax * leanProgress;
}

// helper function for animateAvatar(). calculate the angle at which to bank into corners whilst turning
function getLeanRoll(deltaTime, speed) {

    var leanRollProgress = 0;
    var angularVelocity = filter.radToDeg(MyAvatar.getAngularVelocity().y);

    leanRollProgress = speed / TOP_SPEED;
    leanRollProgress *= Math.abs(angularVelocity) / motion.calibration.angularVelocityMax;

    // shape the response curve
    leanRollProgress = filter.bezier((1 - leanRollProgress),
        {x: 0, y: 0}, {x: 0, y: 1.3}, {x: 0.25, y: 0.5}, {x: 1, y: 1}).y;

    // which way to lean?
    var turnSign = -1;
    if (angularVelocity < 0.001) {

        turnSign = 1;
    }
    if (motion.direction === BACKWARDS ||
        motion.direction === LEFT) {

        turnSign *= -1;
    }

    // add damping with averaging filter
    leanRollProgress = leanRollSmoothingFilter.process(turnSign * leanRollProgress);
    return motion.calibration.rollMax * leanRollProgress;
}

// check for existence of object property (e.g. animation waveform synthesis filters)
function isDefined(value) {

    try {
        if (typeof value != 'undefined') return true;
    } catch (e) {
        return false;
    }
}

// helper function for animateAvatar(). calculate joint translations based on animation file settings and frequency * time
function calculateTranslations(animation, ft, direction) {

    var jointName = "Hips";
    var joint = animation.joints[jointName];
    var jointTranslations = {x:0, y:0, z:0};

    // gather modifiers and multipliers
    modifiers = new JointModifiers(joint, direction);

    // calculate translations. Use synthesis filters where specified by the animation file.

    // sway (oscillation on the x-axis)
    if(animation.filters.hasOwnProperty(jointName) &&
       'swayFilter' in animation.filters[jointName]) {

        jointTranslations.x = joint.sway *
            animation.filters[jointName].swayFilter.
            calculate(filter.degToRad(ft + joint.swayPhase)) +
            joint.swayOffset;

    } else {

        jointTranslations.x = joint.sway *
            Math.sin(filter.degToRad(ft + joint.swayPhase)) +
            joint.swayOffset;
    }

    // bob (oscillation on the y-axis)
    if(animation.filters.hasOwnProperty(jointName) &&
       'bobFilter' in animation.filters[jointName]) {

        jointTranslations.y = joint.bob *
            animation.filters[jointName].bobFilter.calculate
            (filter.degToRad(modifiers.bobFrequencyMultiplier * ft +
            joint.bobPhase + modifiers.bobReverseModifier)) +
            joint.bobOffset;

    } else {

        jointTranslations.y = joint.bob *
            Math.sin(filter.degToRad(modifiers.bobFrequencyMultiplier * ft +
            joint.bobPhase + modifiers.bobReverseModifier)) +
            joint.bobOffset;

        // check for specified low pass filter for this joint (currently Hips bob only)
        if(animation.filters.hasOwnProperty(jointName) &&
               'bobLPFilter' in animation.filters[jointName]) {

            jointTranslations.y = filter.clipTrough(jointTranslations.y, joint, 2);
            jointTranslations.y = animation.filters[jointName].bobLPFilter.process(jointTranslations.y);
        }
    }

    // thrust (oscillation on the z-axis)
    if(animation.filters.hasOwnProperty(jointName) &&
       'thrustFilter' in animation.filters[jointName]) {

        jointTranslations.z = joint.thrust *
            animation.filters[jointName].thrustFilter.
            calculate(filter.degToRad(modifiers.thrustFrequencyMultiplier * ft +
            joint.thrustPhase)) +
            joint.thrustOffset;

    } else {

        jointTranslations.z = joint.thrust *
            Math.sin(filter.degToRad(modifiers.thrustFrequencyMultiplier * ft +
            joint.thrustPhase)) +
            joint.thrustOffset;
    }

    return jointTranslations;
}

// helper function for animateAvatar(). calculate joint rotations based on animation file settings and frequency * time
function calculateRotations(jointName, animation, ft, direction) {

    var joint = animation.joints[jointName];
    var jointRotations = {x:0, y:0, z:0};

    // gather modifiers and multipliers for this joint
    modifiers = new JointModifiers(joint, direction);

    // calculate rotations. Use synthesis filters where specified by the animation file.

    // calculate pitch
    if(animation.filters.hasOwnProperty(jointName) &&
       'pitchFilter' in animation.filters[jointName]) {

        jointRotations.x = modifiers.pitchReverseInvert * (modifiers.pitchSign * joint.pitch *
            animation.filters[jointName].pitchFilter.calculate
            (filter.degToRad(ft * modifiers.pitchFrequencyMultiplier +
            joint.pitchPhase + modifiers.pitchPhaseModifier + modifiers.pitchReverseModifier)) +
            modifiers.pitchOffsetSign * joint.pitchOffset);

    } else {

        jointRotations.x = modifiers.pitchReverseInvert * (modifiers.pitchSign * joint.pitch *
            Math.sin(filter.degToRad(ft * modifiers.pitchFrequencyMultiplier +
            joint.pitchPhase + modifiers.pitchPhaseModifier + modifiers.pitchReverseModifier)) +
            modifiers.pitchOffsetSign * joint.pitchOffset);
    }

    // calculate yaw
    if(animation.filters.hasOwnProperty(jointName) &&
       'yawFilter' in animation.filters[jointName]) {

        jointRotations.y = modifiers.yawSign * joint.yaw *
            animation.filters[jointName].yawFilter.calculate
            (filter.degToRad(ft + joint.yawPhase + modifiers.yawReverseModifier)) +
            modifiers.yawOffsetSign * joint.yawOffset;

    } else {

        jointRotations.y = modifiers.yawSign * joint.yaw *
            Math.sin(filter.degToRad(ft + joint.yawPhase + modifiers.yawReverseModifier)) +
            modifiers.yawOffsetSign * joint.yawOffset;
    }

    // calculate roll
    if(animation.filters.hasOwnProperty(jointName) &&
       'rollFilter' in animation.filters[jointName]) {

        jointRotations.z = modifiers.rollSign * joint.roll *
            animation.filters[jointName].rollFilter.calculate
            (filter.degToRad(ft + joint.rollPhase + modifiers.rollReverseModifier)) +
            modifiers.rollOffsetSign * joint.rollOffset;

    } else {

        jointRotations.z = modifiers.rollSign * joint.roll *
            Math.sin(filter.degToRad(ft + joint.rollPhase + modifiers.rollReverseModifier)) +
            modifiers.rollOffsetSign * joint.rollOffset;
    }
    return jointRotations;
}

// animate the avatar using sine waves, geometric waveforms and harmonic generators
function animateAvatar(deltaTime, speed) {

    // adjust leaning direction for flying
    var flyingModifier = 1;
    if (state.currentState.FLYING) {

        flyingModifier = -1;
    }

    // leaning in response to speed and acceleration (affects Hips pitch and z-axis acceleration driven offset)
    var leanPitch = getLeanPitch(speed);

    // hips translations
    var hipsTranslations = undefined;
    if (motion.currentTransition !== nullTransition) {

        hipsTranslations = motion.currentTransition.blendTranslations(motion.frequencyTimeWheelPos, motion.direction);

    } else {

        hipsTranslations = calculateTranslations(motion.currentAnimation,
                                                 motion.frequencyTimeWheelPos,
                                                 motion.direction);
    }

    // factor in any acceleration driven leaning
    hipsTranslations.z += motion.calibration.hipsToFeet * Math.sin(filter.degToRad(leanPitch));

    // apply translations
    MyAvatar.setSkeletonOffset(hipsTranslations);

    // joint rotations
    for (jointName in walkAssets.animationReference.joints) {

        // ignore arms rotations if 'arms free' is selected for Leap / Hydra use
        if((walkAssets.animationReference.joints[jointName].IKChain === "LeftArm" ||
            walkAssets.animationReference.joints[jointName].IKChain === "RightArm") &&
            motion.armsFree) {

                continue;
        }

        if (isDefined(motion.currentAnimation.joints[jointName])) {

            var jointRotations = undefined;

            // if there's a live transition, blend the rotations with the last animation's rotations for this joint
            if (motion.currentTransition !== nullTransition) {

                jointRotations = motion.currentTransition.blendRotations(jointName,
                                                                         motion.frequencyTimeWheelPos,
                                                                         motion.direction);
            } else {

                jointRotations = calculateRotations(jointName,
                                                    motion.currentAnimation,
                                                    motion.frequencyTimeWheelPos,
                                                    motion.direction);
            }

            // apply lean
            if (jointName === "Hips") {

                jointRotations.x += leanPitch;
                jointRotations.z += getLeanRoll(deltaTime, speed);
            }

            // apply rotation
            MyAvatar.setJointData(jointName, Quat.fromVec3Degrees(jointRotations));
        }
    }
}