//
//  walk.js
//  version 1.25
//
//  Created by David Wooldridge, June 2015
//  Copyright © 2014 - 2015 High Fidelity, Inc.
//
//  Animates an avatar using procedural animation techniques.
//
//  Editing tools for animation data files available here: https://github.com/DaveDubUK/walkTools
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// animations, reach poses, reach pose parameters, transitions, transition parameters, sounds, image/s and reference files
const HIFI_PUBLIC_BUCKET = "https://hifi-public.s3.amazonaws.com/";
var pathToAssets = HIFI_PUBLIC_BUCKET + "procedural-animator/assets/";

Script.include([
    "./libraries/walkConstants.js",
    "./libraries/walkFilters.js",
    "./libraries/walkApi.js",
    pathToAssets + "walkAssets.js"
]);

// construct Avatar, Motion and (null) Transition
var avatar = new Avatar();
var motion = new Motion();
var nullTransition = new Transition();
motion.currentTransition = nullTransition;

// create settings (gets initial values from avatar)
Script.include("./libraries/walkSettings.js");

// Main loop
Script.update.connect(function(deltaTime) {

    if (motion.isLive) {

        // assess current locomotion state
        motion.assess(deltaTime);

        // decide which animation should be playing
        selectAnimation();

        // advance the animation cycle/s by the correct amount/s
        advanceAnimations();

        // update the progress of any live transitions
        updateTransitions();

        // apply translation and rotations
        renderMotion();

        // save this frame's parameters
        motion.saveHistory();
    }
});

// helper function for selectAnimation()
function setTransition(nextAnimation, playTransitionReachPoses) {
    var lastTransition = motion.currentTransition;
    var lastAnimation = avatar.currentAnimation;

    // if already transitioning from a blended walk need to maintain the previous walk's direction
    if (lastAnimation.lastDirection) {
        switch(lastAnimation.lastDirection) {

            case FORWARDS:
                lastAnimation = avatar.selectedWalk;
                break;

            case BACKWARDS:
                lastAnimation = avatar.selectedWalkBackwards;
                break;

            case LEFT:
                lastAnimation = avatar.selectedSideStepLeft;
                break;

            case RIGHT:
                lastAnimation = avatar.selectedSideStepRight;
                break;
        }
    }

    motion.currentTransition = new Transition(nextAnimation, lastAnimation, lastTransition, playTransitionReachPoses);
    avatar.currentAnimation = nextAnimation;

    // reset default first footstep
    if (nextAnimation === avatar.selectedWalkBlend && lastTransition === nullTransition) {
        avatar.nextStep = RIGHT;
    }
}

// fly animation blending: smoothing / damping filters
const FLY_BLEND_DAMPING = 50;
var flyUpFilter = filter.createAveragingFilter(FLY_BLEND_DAMPING);
var flyDownFilter = filter.createAveragingFilter(FLY_BLEND_DAMPING);
var flyForwardFilter = filter.createAveragingFilter(FLY_BLEND_DAMPING);
var flyBackwardFilter = filter.createAveragingFilter(FLY_BLEND_DAMPING);

// select / blend the appropriate animation for the current state of motion
function selectAnimation() {
    var playTransitionReachPoses = true;

    // select appropriate animation. create transitions where appropriate
    switch (motion.nextState) {
        case STATIC: {
            if (avatar.distanceFromSurface < ON_SURFACE_THRESHOLD &&
                    avatar.currentAnimation !== avatar.selectedIdle) {
                setTransition(avatar.selectedIdle, playTransitionReachPoses);
            } else if (!(avatar.distanceFromSurface < ON_SURFACE_THRESHOLD) &&
                         avatar.currentAnimation !== avatar.selectedHover) {
                setTransition(avatar.selectedHover, playTransitionReachPoses);
            }
            motion.state = STATIC;
            avatar.selectedWalkBlend.lastDirection = NONE;
            break;
        }

        case SURFACE_MOTION: {
            // walk transition reach poses are currently only specified for starting to walk forwards
            playTransitionReachPoses = (motion.direction === FORWARDS);
            var isAlreadyWalking = (avatar.currentAnimation === avatar.selectedWalkBlend);

            switch (motion.direction) {
                case FORWARDS:
                    if (avatar.selectedWalkBlend.lastDirection !== FORWARDS) {
                        animationOperations.deepCopy(avatar.selectedWalk, avatar.selectedWalkBlend);
                        avatar.calibration.strideLength = avatar.selectedWalk.calibration.strideLength;
                    }
                    avatar.selectedWalkBlend.lastDirection = FORWARDS;
                    break;

                case BACKWARDS:
                    if (avatar.selectedWalkBlend.lastDirection !== BACKWARDS) {
                        animationOperations.deepCopy(avatar.selectedWalkBackwards, avatar.selectedWalkBlend);
                        avatar.calibration.strideLength = avatar.selectedWalkBackwards.calibration.strideLength;
                    }
                    avatar.selectedWalkBlend.lastDirection = BACKWARDS;
                    break;

                case LEFT:
                    animationOperations.deepCopy(avatar.selectedSideStepLeft, avatar.selectedWalkBlend);
                    avatar.selectedWalkBlend.lastDirection = LEFT;
                    avatar.calibration.strideLength = avatar.selectedSideStepLeft.calibration.strideLength;
                    break

                case RIGHT:
                    animationOperations.deepCopy(avatar.selectedSideStepRight, avatar.selectedWalkBlend);
                    avatar.selectedWalkBlend.lastDirection = RIGHT;
                    avatar.calibration.strideLength = avatar.selectedSideStepRight.calibration.strideLength;
                    break;

                default:
                    // condition occurs when the avi goes through the floor due to collision hull errors
                    animationOperations.deepCopy(avatar.selectedWalk, avatar.selectedWalkBlend);
                    avatar.selectedWalkBlend.lastDirection = FORWARDS;
                    avatar.calibration.strideLength = avatar.selectedWalk.calibration.strideLength;
                    break;
            }

            if (!isAlreadyWalking && !motion.isComingToHalt) {
                setTransition(avatar.selectedWalkBlend, playTransitionReachPoses);
            }
            motion.state = SURFACE_MOTION;
            break;
        }

        case AIR_MOTION: {
            // blend the up, down, forward and backward flying animations relative to motion speed and direction
            animationOperations.zeroAnimation(avatar.selectedFlyBlend);

            // calculate influences based on velocity and direction
            var velocityMagnitude = Vec3.length(motion.velocity);
            var verticalProportion = motion.velocity.y / velocityMagnitude;
            var thrustProportion = motion.velocity.z / velocityMagnitude / 2;

            // directional components
            var upComponent = motion.velocity.y > 0 ? verticalProportion : 0;
            var downComponent = motion.velocity.y < 0 ? -verticalProportion : 0;
            var forwardComponent = motion.velocity.z < 0 ? -thrustProportion : 0;
            var backwardComponent = motion.velocity.z > 0 ? thrustProportion : 0;

            // smooth / damp directional components to add visual 'weight'
            upComponent = flyUpFilter.process(upComponent);
            downComponent = flyDownFilter.process(downComponent);
            forwardComponent = flyForwardFilter.process(forwardComponent);
            backwardComponent = flyBackwardFilter.process(backwardComponent);

            // normalise directional components
            var normaliser = upComponent + downComponent + forwardComponent + backwardComponent;
            upComponent = upComponent / normaliser;
            downComponent = downComponent / normaliser;
            forwardComponent = forwardComponent / normaliser;
            backwardComponent = backwardComponent / normaliser;

            // blend animations proportionally
            if (upComponent > 0) {
                animationOperations.blendAnimation(avatar.selectedFlyUp,
                                         avatar.selectedFlyBlend,
                                         upComponent);
            }
            if (downComponent > 0) {
                animationOperations.blendAnimation(avatar.selectedFlyDown,
                                     avatar.selectedFlyBlend,
                                     downComponent);
            }
            if (forwardComponent > 0) {
                animationOperations.blendAnimation(avatar.selectedFly,
                                     avatar.selectedFlyBlend,
                                     Math.abs(forwardComponent));
            }
            if (backwardComponent > 0) {
                animationOperations.blendAnimation(avatar.selectedFlyBackwards,
                                     avatar.selectedFlyBlend,
                                     Math.abs(backwardComponent));
            }

            if (avatar.currentAnimation !== avatar.selectedFlyBlend) {
                setTransition(avatar.selectedFlyBlend, playTransitionReachPoses);
            }
            motion.state = AIR_MOTION;
            avatar.selectedWalkBlend.lastDirection = NONE;
            break;
        }
    } // end switch next state of motion
}

// determine the length of stride. advance the frequency time wheels. advance frequency time wheels for any live transitions
function advanceAnimations() {
    var wheelAdvance = 0;

    // turn the frequency time wheel
    if (avatar.currentAnimation === avatar.selectedWalkBlend) {
        // Using technique described here: http://www.gdcvault.com/play/1020583/Animation-Bootcamp-An-Indie-Approach
        // wrap the stride length around a 'surveyor's wheel' twice and calculate the angular speed at the given (linear) speed
        // omega = v / r , where r = circumference / 2 PI and circumference = 2 * stride length
        var speed = Vec3.length(motion.velocity);
        motion.frequencyTimeWheelRadius = avatar.calibration.strideLength / Math.PI;
        var ftWheelAngularVelocity = speed / motion.frequencyTimeWheelRadius;
        // calculate the degrees turned (at this angular speed) since last frame
        wheelAdvance = filter.radToDeg(motion.deltaTime * ftWheelAngularVelocity);
    } else {
        // turn the frequency time wheel by the amount specified for this animation
        wheelAdvance = filter.radToDeg(avatar.currentAnimation.calibration.frequency * motion.deltaTime);
    }

    if (motion.currentTransition !== nullTransition) {
        // the last animation is still playing so we turn it's frequency time wheel to maintain the animation
        if (motion.currentTransition.lastAnimation === motion.selectedWalkBlend) {
            // if at a stop angle (i.e. feet now under the avi) hold the wheel position for remainder of transition
            var tolerance = motion.currentTransition.lastFrequencyTimeIncrement + 0.1;
            if ((motion.currentTransition.lastFrequencyTimeWheelPos >
                (motion.currentTransition.stopAngle - tolerance)) &&
                (motion.currentTransition.lastFrequencyTimeWheelPos <
                (motion.currentTransition.stopAngle + tolerance))) {
                motion.currentTransition.lastFrequencyTimeIncrement = 0;
            }
        }
        motion.currentTransition.advancePreviousFrequencyTimeWheel(motion.deltaTime);
    }

    // avoid unnaturally fast walking when landing at speed - simulates skimming / skidding
    if (Math.abs(wheelAdvance) > MAX_FT_WHEEL_INCREMENT) {
        wheelAdvance = 0;
    }

    // advance the walk wheel the appropriate amount
    motion.advanceFrequencyTimeWheel(wheelAdvance);

    // walking? then see if it's a good time to measure the stride length (needs to be at least 97% of max walking speed)
    const ALMOST_ONE = 0.97;
    if (avatar.currentAnimation === avatar.selectedWalkBlend &&
       (Vec3.length(motion.velocity) / MAX_WALK_SPEED > ALMOST_ONE)) {

        var strideMaxAt = avatar.currentAnimation.calibration.strideMaxAt;
        const TOLERANCE = 1.0;

        if (motion.frequencyTimeWheelPos < (strideMaxAt + TOLERANCE) &&
            motion.frequencyTimeWheelPos > (strideMaxAt - TOLERANCE) &&
            motion.currentTransition === nullTransition) {
            // measure and save stride length
            var footRPos = MyAvatar.getJointPosition("RightFoot");
            var footLPos = MyAvatar.getJointPosition("LeftFoot");
            avatar.calibration.strideLength = Vec3.distance(footRPos, footLPos);
            avatar.currentAnimation.calibration.strideLength = avatar.calibration.strideLength;
        } else {
            // use the previously saved value for stride length
            avatar.calibration.strideLength = avatar.currentAnimation.calibration.strideLength;
        }
    } // end get walk stride length
}

// initialise a new transition. update progress of a live transition
function updateTransitions() {

    if (motion.currentTransition !== nullTransition) {
        // is this a new transition?
        if (motion.currentTransition.progress === 0) {
            // do we have overlapping transitions?
            if (motion.currentTransition.lastTransition !== nullTransition) {
                // is the last animation for the nested transition the same as the new animation?
                if (motion.currentTransition.lastTransition.lastAnimation === avatar.currentAnimation) {
                    // then sync the nested transition's frequency time wheel for a smooth animation blend
                    motion.frequencyTimeWheelPos = motion.currentTransition.lastTransition.lastFrequencyTimeWheelPos;
                }
            }
        }
        if (motion.currentTransition.updateProgress() === TRANSITION_COMPLETE) {
            motion.currentTransition = nullTransition;
        }
    }
}

// helper function for renderMotion(). calculate the amount to lean forwards (or backwards) based on the avi's velocity
var leanPitchSmoothingFilter = filter.createButterworthFilter();
function getLeanPitch() {
    var leanProgress = 0;

    if (motion.direction === DOWN ||
        motion.direction === FORWARDS ||
        motion.direction === BACKWARDS) {
        leanProgress = -motion.velocity.z / TOP_SPEED;
    }
    // use filters to shape the walking acceleration response
    leanProgress = leanPitchSmoothingFilter.process(leanProgress);
    return PITCH_MAX * leanProgress;
}

// helper function for renderMotion(). calculate the angle at which to bank into corners whilst turning
var leanRollSmoothingFilter = filter.createButterworthFilter();
function getLeanRoll() {
    var leanRollProgress = 0;
    var linearContribution = 0;
    const LOG_SCALER = 8;

    if (Vec3.length(motion.velocity) > 0) {
        linearContribution = (Math.log(Vec3.length(motion.velocity) / TOP_SPEED) + LOG_SCALER) / LOG_SCALER;
    }
    var angularContribution = Math.abs(motion.yawDelta) / DELTA_YAW_MAX;
    leanRollProgress = linearContribution;
    leanRollProgress *= angularContribution;
    // shape the response curve
    leanRollProgress = filter.bezier(leanRollProgress, {x: 1, y: 0}, {x: 1, y: 0});
    // which way to lean?
    var turnSign = (motion.yawDelta >= 0) ? 1 : -1;

    if (motion.direction === BACKWARDS ||
        motion.direction === LEFT) {
        turnSign *= -1;
    }
    // filter progress
    leanRollProgress = leanRollSmoothingFilter.process(turnSign * leanRollProgress);
    return ROLL_MAX * leanRollProgress;
}

// animate the avatar using sine waves, geometric waveforms and harmonic generators
function renderMotion() {
    // leaning in response to speed and acceleration
    var leanPitch = motion.state === STATIC ? 0 : getLeanPitch();
    var leanRoll = motion.state === STATIC ? 0 : getLeanRoll();
    var lastDirection = motion.lastDirection;
    // hips translations from currently playing animations
    var hipsTranslations = {x:0, y:0, z:0};

    if (motion.currentTransition !== nullTransition) {
        // maintain previous direction when transitioning from a walk
        if (motion.currentTransition.lastAnimation === avatar.selectedWalkBlend) {
            motion.lastDirection = motion.currentTransition.lastDirection;
        }
        hipsTranslations = motion.currentTransition.blendTranslations(motion.frequencyTimeWheelPos,
                                                                      motion.lastDirection);
    } else {
        hipsTranslations = animationOperations.calculateTranslations(avatar.currentAnimation,
                                                                     motion.frequencyTimeWheelPos,
                                                                     motion.direction);
    }
    // factor any leaning into the hips offset
    hipsTranslations.z += avatar.calibration.hipsToFeet * Math.sin(filter.degToRad(leanPitch));
    hipsTranslations.x += avatar.calibration.hipsToFeet * Math.sin(filter.degToRad(leanRoll));

    // ensure skeleton offsets are within the 1m limit
    hipsTranslations.x = hipsTranslations.x > 1 ? 1 : hipsTranslations.x;
    hipsTranslations.x = hipsTranslations.x < -1 ? -1 : hipsTranslations.x;
    hipsTranslations.y = hipsTranslations.y > 1 ? 1 : hipsTranslations.y;
    hipsTranslations.y = hipsTranslations.y < -1 ? -1 : hipsTranslations.y;
    hipsTranslations.z = hipsTranslations.z > 1 ? 1 : hipsTranslations.z;
    hipsTranslations.z = hipsTranslations.z < -1 ? -1 : hipsTranslations.z;
    // apply translations
    MyAvatar.setSkeletonOffset(hipsTranslations);

    // play footfall sound?
    var producingFootstepSounds = (avatar.currentAnimation === avatar.selectedWalkBlend) && avatar.makesFootStepSounds;

    if (motion.currentTransition !== nullTransition && avatar.makesFootStepSounds) {
        if (motion.currentTransition.nextAnimation === avatar.selectedWalkBlend ||
            motion.currentTransition.lastAnimation === avatar.selectedWalkBlend) {
                producingFootstepSounds = true;
        }
    }
    if (producingFootstepSounds) {
        const QUARTER_CYCLE = 90;
        const THREE_QUARTER_CYCLE = 270;
        var ftWheelPosition = motion.frequencyTimeWheelPos;

        if (motion.currentTransition !== nullTransition &&
            motion.currentTransition.lastAnimation === avatar.selectedWalkBlend) {
            ftWheelPosition = motion.currentTransition.lastFrequencyTimeWheelPos;
        }
        if (avatar.nextStep === LEFT && ftWheelPosition > THREE_QUARTER_CYCLE) {
            avatar.makeFootStepSound();
        } else if (avatar.nextStep === RIGHT && (ftWheelPosition < THREE_QUARTER_CYCLE && ftWheelPosition > QUARTER_CYCLE)) {
            avatar.makeFootStepSound();
        }
    }

    // apply joint rotations
    for (jointName in avatar.currentAnimation.joints) {
        var joint = walkAssets.animationReference.joints[jointName];
        var jointRotations = undefined;

        // ignore arms / head rotations if options are selected in the settings
        if (avatar.armsFree && (joint.IKChain === "LeftArm" || joint.IKChain === "RightArm")) {
            continue;
        }
        if (avatar.headFree && joint.IKChain === "Head") {
            continue;
        }

        // if there's a live transition, blend the rotations with the last animation's rotations
        if (motion.currentTransition !== nullTransition) {
            jointRotations = motion.currentTransition.blendRotations(jointName,
                                                                     motion.frequencyTimeWheelPos,
                                                                     motion.lastDirection);
        } else {
            jointRotations = animationOperations.calculateRotations(jointName,
                                                avatar.currentAnimation,
                                                motion.frequencyTimeWheelPos,
                                                motion.direction);
        }

        // apply angular velocity and speed induced leaning
        if (jointName === "Hips") {
            jointRotations.x += leanPitch;
            jointRotations.z += leanRoll;
        }

        // apply rotations
        MyAvatar.setJointRotation(jointName, Quat.fromVec3Degrees(jointRotations));
    }
}
