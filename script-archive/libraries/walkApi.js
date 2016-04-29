//
//  walkApi.js
//  version 1.3
//
//  Created by David Wooldridge, June 2015
//  Copyright © 2014 - 2015 High Fidelity, Inc.
//
//  Exposes API for use by walk.js version 1.2+.
//
//  Editing tools for animation data files available here: https://github.com/DaveDubUK/walkTools
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// included here to ensure walkApi.js can be used as an API, separate from walk.js
Script.include("walkConstants.js");

Avatar = function() {
    // if Hydras are connected, the only way to enable use is to never set any arm joint rotation
    this.hydraCheck = function () {
        return Controller.Hardware.Hydra !== undefined;
    }
    // settings
    this.headFree = true;
    this.armsFree = this.hydraCheck(); // automatically sets true to enable Hydra support - temporary fix
    this.makesFootStepSounds = false;
    this.blenderPreRotations = false; // temporary fix
    this.animationSet = undefined; // currently just one animation set
    this.setAnimationSet = function(animationSet) {
        this.animationSet = animationSet;
        switch (animationSet) {
            case 'standardMale':
                this.selectedIdle = walkAssets.getAnimationDataFile("MaleIdle");
                this.selectedWalk = walkAssets.getAnimationDataFile("MaleWalk");
                this.selectedWalkBackwards = walkAssets.getAnimationDataFile("MaleWalkBackwards");
                this.selectedSideStepLeft = walkAssets.getAnimationDataFile("MaleSideStepLeft");
                this.selectedSideStepRight = walkAssets.getAnimationDataFile("MaleSideStepRight");
                this.selectedWalkBlend = walkAssets.getAnimationDataFile("WalkBlend");
                this.selectedHover = walkAssets.getAnimationDataFile("MaleHover");
                this.selectedFly = walkAssets.getAnimationDataFile("MaleFly");
                this.selectedFlyBackwards = walkAssets.getAnimationDataFile("MaleFlyBackwards");
                this.selectedFlyDown = walkAssets.getAnimationDataFile("MaleFlyDown");
                this.selectedFlyUp = walkAssets.getAnimationDataFile("MaleFlyUp");
                this.selectedFlyBlend = walkAssets.getAnimationDataFile("FlyBlend");
                this.currentAnimation = this.selectedIdle;
                return;
        }
    }
    this.setAnimationSet('standardMale');

    // calibration
    this.calibration = {
        hipsToFeet: 1,
        strideLength: this.selectedWalk.calibration.strideLength
    }
    this.distanceFromSurface = 0;
    this.calibrate = function() {
        // Triple check: measurements are taken three times to ensure accuracy - the first result is often too large
        const MAX_ATTEMPTS = 3;
        var attempts = MAX_ATTEMPTS;
        var extraAttempts = 0;
        do {
            for (joint in walkAssets.animationReference.joints) {
                var IKChain = walkAssets.animationReference.joints[joint].IKChain;

                // only need to zero right leg IK chain and hips
                if (IKChain === "RightLeg" || joint === "Hips" ) {
                    MyAvatar.setJointRotation(joint, Quat.fromPitchYawRollDegrees(0, 0, 0));
                }
            }
            this.calibration.hipsToFeet = MyAvatar.getJointPosition("Hips").y - MyAvatar.getJointPosition("RightToeBase").y;

            // maybe measuring before Blender pre-rotations have been applied?
            if (this.calibration.hipsToFeet < 0 && this.blenderPreRotations) {
                this.calibration.hipsToFeet *= -1;
            }

            if (this.calibration.hipsToFeet === 0 && extraAttempts < 100) {
                attempts++;
                extraAttempts++;// Interface can sometimes report zero for hips to feet. if so, we try again.
            }
        } while (attempts-- > 1)

        // just in case
        if (this.calibration.hipsToFeet <= 0 || isNaN(this.calibration.hipsToFeet)) {
            this.calibration.hipsToFeet = 1;
            print('walk.js error: Unable to get a non-zero measurement for the avatar hips to feet measure. Hips to feet set to default value ('+
                  this.calibration.hipsToFeet.toFixed(3)+'m). This will cause some foot sliding. If your avatar has only just appeared, it is recommended that you re-load the walk script.');
        } else {
            print('walk.js info: Hips to feet calibrated to '+this.calibration.hipsToFeet.toFixed(3)+'m');
        }
    }

    // pose the fingers
    this.poseFingers = function() {
        for (knuckle in walkAssets.animationReference.leftHand) {
            if (walkAssets.animationReference.leftHand[knuckle].IKChain === "LeftHandThumb") {
                MyAvatar.setJointRotation(knuckle, Quat.fromPitchYawRollDegrees(0, 0, -4));
            } else {
                MyAvatar.setJointRotation(knuckle, Quat.fromPitchYawRollDegrees(16, 0, 5));
            }
        }
        for (knuckle in walkAssets.animationReference.rightHand) {
            if (walkAssets.animationReference.rightHand[knuckle].IKChain === "RightHandThumb") {
                MyAvatar.setJointRotation(knuckle, Quat.fromPitchYawRollDegrees(0, 0, 4));
            } else {
                MyAvatar.setJointRotation(knuckle, Quat.fromPitchYawRollDegrees(16, 0, -5));
            }
        }
    };
    this.calibrate();
    this.poseFingers();

    // footsteps
    this.nextStep = RIGHT; // the first step is right, because the waveforms say so
    this.leftAudioInjector = null;
    this.rightAudioInjector = null;
    this.makeFootStepSound = function() {
        // correlate footstep volume with avatar speed. place the audio source at the feet, not the hips
        const SPEED_THRESHOLD = 0.4;
        const VOLUME_ATTENUATION = 0.8;
        const MIN_VOLUME = 0.5;
        var volume = Vec3.length(motion.velocity) > SPEED_THRESHOLD ?
                     VOLUME_ATTENUATION * Vec3.length(motion.velocity) / MAX_WALK_SPEED : MIN_VOLUME;
        volume = volume > 1 ? 1 : volume; // occurs when landing at speed - can walk faster than max walking speed
        var options = {
            position: Vec3.sum(MyAvatar.position, {x:0, y: -this.calibration.hipsToFeet, z:0}),
            volume: volume
        };
        if (this.nextStep === RIGHT) {
            if (this.rightAudioInjector === null) {
                this.rightAudioInjector = Audio.playSound(walkAssets.footsteps[0], options);
            } else {
                this.rightAudioInjector.setOptions(options);
                this.rightAudioInjector.restart();
            }
            this.nextStep = LEFT;
        } else if (this.nextStep === LEFT) {
            if (this.leftAudioInjector === null) {
                this.leftAudioInjector = Audio.playSound(walkAssets.footsteps[1], options);
            } else {
                this.leftAudioInjector.setOptions(options);
                this.leftAudioInjector.restart();
            }
            this.nextStep = RIGHT;
        }
    }
};

// constructor for the Motion object
Motion = function() {
    this.isLive = true;
    // locomotion status
    this.state = STATIC;
    this.nextState = STATIC;
    this.isMoving = false;
    this.isWalkingSpeed = false;
    this.isFlyingSpeed = false;
    this.isAccelerating = false;
    this.isDecelerating = false;
    this.isDeceleratingFast = false;
    this.isComingToHalt = false;
    this.directedAcceleration = 0;

    // used to make sure at least one step has been taken when transitioning from a walk cycle
    this.elapsedFTDegrees = 0;

    // the current transition (any layered transitions are nested within this transition)
    this.currentTransition = null;

    // orientation, locomotion and timing
    this.velocity = {x:0, y:0, z:0};
    this.acceleration = {x:0, y:0, z:0};
    this.yaw = Quat.safeEulerAngles(MyAvatar.orientation).y;
    this.yawDelta = 0;
    this.yawDeltaAcceleration = 0;
    this.direction = FORWARDS;
    this.deltaTime = 0;

    // historical orientation, locomotion and timing
    this.lastDirection = FORWARDS;
    this.lastVelocity = {x:0, y:0, z:0};
    this.lastYaw = Quat.safeEulerAngles(MyAvatar.orientation).y;
    this.lastYawDelta = 0;
    this.lastYawDeltaAcceleration = 0;

    // Quat.safeEulerAngles(MyAvatar.orientation).y tends to repeat values between frames, so values are filtered
    var YAW_SMOOTHING = 22;
    this.yawFilter = filter.createAveragingFilter(YAW_SMOOTHING);
    this.deltaTimeFilter = filter.createAveragingFilter(YAW_SMOOTHING);
    this.yawDeltaAccelerationFilter = filter.createAveragingFilter(YAW_SMOOTHING);

    // assess locomotion state
    this.assess = function(deltaTime) {
        // calculate avatar frame speed, velocity and acceleration
        this.deltaTime = deltaTime;
        this.velocity = Vec3.multiplyQbyV(Quat.inverse(MyAvatar.orientation), MyAvatar.getVelocity());
        var lateralVelocity = Math.sqrt(Math.pow(this.velocity.x, 2) + Math.pow(this.velocity.z, 2));

        // MyAvatar.getAcceleration() currently not working. bug report submitted: https://worklist.net/20527
        var acceleration = {x:0, y:0, z:0};
        this.acceleration.x = (this.velocity.x - this.lastVelocity.x) / deltaTime;
        this.acceleration.y = (this.velocity.y - this.lastVelocity.y) / deltaTime;
        this.acceleration.z = (this.velocity.z - this.lastVelocity.z) / deltaTime;

        // MyAvatar.getAngularVelocity and MyAvatar.getAngularAcceleration currently not working. bug report submitted
        this.yaw = Quat.safeEulerAngles(MyAvatar.orientation).y;
        if (this.lastYaw < 0 && this.yaw > 0 || this.lastYaw > 0 && this.yaw < 0) {
            this.lastYaw *= -1;
        }
        var timeDelta = this.deltaTimeFilter.process(deltaTime);
        this.yawDelta = filter.degToRad(this.yawFilter.process(this.lastYaw - this.yaw)) / timeDelta;
        this.yawDeltaAcceleration = this.yawDeltaAccelerationFilter.process(this.lastYawDelta - this.yawDelta) / timeDelta;

        // how far above the surface is the avatar? (for testing / validation purposes)
        var pickRay = {origin: MyAvatar.position, direction: {x:0, y:-1, z:0}};
        var distanceFromSurface = Entities.findRayIntersectionBlocking(pickRay).distance;
        avatar.distanceFromSurface = distanceFromSurface - avatar.calibration.hipsToFeet;

        // determine principle direction of locomotion
        var FWD_BACK_BIAS = 100; // helps prevent false sidestep condition detection when banking hard
        if (Math.abs(this.velocity.x) > Math.abs(this.velocity.y) &&
            Math.abs(this.velocity.x) > FWD_BACK_BIAS * Math.abs(this.velocity.z)) {
            if (this.velocity.x < 0) {
                this.directedAcceleration = -this.acceleration.x;
                this.direction = LEFT;
            } else if (this.velocity.x > 0){
                this.directedAcceleration = this.acceleration.x;
                this.direction = RIGHT;
            }
        } else if (Math.abs(this.velocity.y) > Math.abs(this.velocity.x) &&
                   Math.abs(this.velocity.y) > Math.abs(this.velocity.z)) {
            if (this.velocity.y > 0) {
                this.directedAcceleration = this.acceleration.y;
                this.direction = UP;
            } else if (this.velocity.y < 0) {
                this.directedAcceleration = -this.acceleration.y;
                this.direction = DOWN;
            }
        } else if (FWD_BACK_BIAS * Math.abs(this.velocity.z) > Math.abs(this.velocity.x) &&
                   Math.abs(this.velocity.z) > Math.abs(this.velocity.y)) {
            if (this.velocity.z < 0) {
                this.direction = FORWARDS;
                this.directedAcceleration = -this.acceleration.z;
            } else if (this.velocity.z > 0) {
                this.directedAcceleration = this.acceleration.z;
                this.direction = BACKWARDS;
            }
        } else {
            this.direction = NONE;
            this.directedAcceleration = 0;
        }

        // set speed flags
        if (Vec3.length(this.velocity) < MOVE_THRESHOLD) {
            this.isMoving = false;
            this.isWalkingSpeed = false;
            this.isFlyingSpeed = false;
            this.isComingToHalt = false;
        } else if (Vec3.length(this.velocity) < MAX_WALK_SPEED) {
            this.isMoving = true;
            this.isWalkingSpeed = true;
            this.isFlyingSpeed = false;
        } else {
            this.isMoving = true;
            this.isWalkingSpeed = false;
            this.isFlyingSpeed = true;
        }

        // set acceleration flags
        if (this.directedAcceleration > ACCELERATION_THRESHOLD) {
            this.isAccelerating = true;
            this.isDecelerating = false;
            this.isDeceleratingFast = false;
            this.isComingToHalt = false;
        } else if (this.directedAcceleration < DECELERATION_THRESHOLD) {
            this.isAccelerating = false;
            this.isDecelerating = true;
            this.isDeceleratingFast = (this.directedAcceleration < FAST_DECELERATION_THRESHOLD);
        } else {
            this.isAccelerating = false;
            this.isDecelerating = false;
            this.isDeceleratingFast = false;
        }

        // use the gathered information to build up some spatial awareness
        var isOnSurface = (avatar.distanceFromSurface < ON_SURFACE_THRESHOLD);
        var isUnderGravity = (avatar.distanceFromSurface < GRAVITY_THRESHOLD);
        var isTakingOff = (isUnderGravity && this.velocity.y > OVERCOME_GRAVITY_SPEED);
        var isComingInToLand = (isUnderGravity && this.velocity.y < -OVERCOME_GRAVITY_SPEED);
        var aboutToLand = isComingInToLand && avatar.distanceFromSurface < LANDING_THRESHOLD;
        var surfaceMotion = isOnSurface && this.isMoving;
        var acceleratingAndAirborne = this.isAccelerating && !isOnSurface;
        var goingTooFastToWalk = !this.isDecelerating && this.isFlyingSpeed;
        var movingDirectlyUpOrDown = (this.direction === UP || this.direction === DOWN)
        var maybeBouncing = Math.abs(this.acceleration.y > BOUNCE_ACCELERATION_THRESHOLD) ? true : false;

        // we now have enough information to set the appropriate locomotion mode
        switch (this.state) {
            case STATIC:
                var staticToAirMotion = this.isMoving && (acceleratingAndAirborne || goingTooFastToWalk ||
                                                           (movingDirectlyUpOrDown && !isOnSurface));
                var staticToSurfaceMotion = surfaceMotion && !motion.isComingToHalt && !movingDirectlyUpOrDown &&
                                            !this.isDecelerating && lateralVelocity > MOVE_THRESHOLD;

                if (staticToAirMotion) {
                    this.nextState = AIR_MOTION;
                } else if (staticToSurfaceMotion) {
                    this.nextState = SURFACE_MOTION;
                } else {
                    this.nextState = STATIC;
                }
                break;

            case SURFACE_MOTION:
                var surfaceMotionToStatic = !this.isMoving ||
                                            (this.isDecelerating && motion.lastDirection !== DOWN && surfaceMotion &&
                                            !maybeBouncing && Vec3.length(this.velocity) < MAX_WALK_SPEED);
                var surfaceMotionToAirMotion = (acceleratingAndAirborne || goingTooFastToWalk || movingDirectlyUpOrDown) &&
                                               (!surfaceMotion && isTakingOff) ||
                                               (!surfaceMotion && this.isMoving && !isComingInToLand);

                if (surfaceMotionToStatic) {
                    // working on the assumption that stopping is now inevitable
                    if (!motion.isComingToHalt && isOnSurface) {
                        motion.isComingToHalt = true;
                    }
                    this.nextState = STATIC;
                } else if (surfaceMotionToAirMotion) {
                    this.nextState = AIR_MOTION;
                } else {
                    this.nextState = SURFACE_MOTION;
                }
                break;

            case AIR_MOTION:
                var airMotionToSurfaceMotion = (surfaceMotion || aboutToLand) && !movingDirectlyUpOrDown;
                var airMotionToStatic = !this.isMoving && this.direction === this.lastDirection;

                if (airMotionToSurfaceMotion){
                    this.nextState = SURFACE_MOTION;
                } else if (airMotionToStatic) {
                    this.nextState = STATIC;
                } else {
                    this.nextState = AIR_MOTION;
                }
                break;
        }
    }

    // frequency time wheel (foot / ground speed matching)
    const DEFAULT_HIPS_TO_FEET = 1;
    this.frequencyTimeWheelPos = 0;
    this.frequencyTimeWheelRadius = DEFAULT_HIPS_TO_FEET / 2;
    this.recentFrequencyTimeIncrements = [];
    const FT_WHEEL_HISTORY_LENGTH = 8;
    for (var i = 0; i < FT_WHEEL_HISTORY_LENGTH; i++) {
        this.recentFrequencyTimeIncrements.push(0);
    }
    this.averageFrequencyTimeIncrement = 0;

    this.advanceFrequencyTimeWheel = function(angle){
        this.elapsedFTDegrees += angle;
        // keep a running average of increments for use in transitions (used during transitioning)
        this.recentFrequencyTimeIncrements.push(angle);
        this.recentFrequencyTimeIncrements.shift();
        for (increment in this.recentFrequencyTimeIncrements) {
            this.averageFrequencyTimeIncrement += this.recentFrequencyTimeIncrements[increment];
        }
        this.averageFrequencyTimeIncrement /= this.recentFrequencyTimeIncrements.length;
        this.frequencyTimeWheelPos += angle;
        const FULL_CIRCLE = 360;
        if (this.frequencyTimeWheelPos >= FULL_CIRCLE) {
            this.frequencyTimeWheelPos = this.frequencyTimeWheelPos % FULL_CIRCLE;
        }
    }

    this.saveHistory = function() {
        this.lastDirection = this.direction;
        this.lastVelocity = this.velocity;
        this.lastYaw = this.yaw;
        this.lastYawDelta = this.yawDelta;
        this.lastYawDeltaAcceleration = this.yawDeltaAcceleration;
    }
};  // end Motion constructor

// animation manipulation object
animationOperations = (function() {

    return {

        // helper function for renderMotion(). calculate joint translations based on animation file settings and frequency * time
        calculateTranslations: function(animation, ft, direction) {
            var jointName = "Hips";
            var joint = animation.joints[jointName];
            var jointTranslations = {x:0, y:0, z:0};

            // gather modifiers and multipliers
            modifiers = new FrequencyMultipliers(joint, direction);

            // calculate translations. Use synthesis filters where specified by the animation data file.

            // sway (oscillation on the x-axis)
            if (animation.filters.hasOwnProperty(jointName) && 'swayFilter' in animation.filters[jointName]) {
                jointTranslations.x = joint.sway * animation.filters[jointName].swayFilter.calculate
                    (filter.degToRad(modifiers.swayFrequencyMultiplier * ft + joint.swayPhase)) + joint.swayOffset;
            } else {
                jointTranslations.x = joint.sway * Math.sin
                    (filter.degToRad(modifiers.swayFrequencyMultiplier * ft + joint.swayPhase)) + joint.swayOffset;
            }
            // bob (oscillation on the y-axis)
            if (animation.filters.hasOwnProperty(jointName) && 'bobFilter' in animation.filters[jointName]) {
                jointTranslations.y = joint.bob * animation.filters[jointName].bobFilter.calculate
                    (filter.degToRad(modifiers.bobFrequencyMultiplier * ft + joint.bobPhase)) + joint.bobOffset;
            } else {
                jointTranslations.y = joint.bob * Math.sin
                    (filter.degToRad(modifiers.bobFrequencyMultiplier * ft + joint.bobPhase)) + joint.bobOffset;

                if (animation.filters.hasOwnProperty(jointName) && 'bobLPFilter' in animation.filters[jointName]) {
                    jointTranslations.y = filter.clipTrough(jointTranslations.y, joint, 2);
                    jointTranslations.y = animation.filters[jointName].bobLPFilter.process(jointTranslations.y);
                }
            }
            // thrust (oscillation on the z-axis)
            if (animation.filters.hasOwnProperty(jointName) && 'thrustFilter' in animation.filters[jointName]) {
                jointTranslations.z = joint.thrust * animation.filters[jointName].thrustFilter.calculate
                    (filter.degToRad(modifiers.thrustFrequencyMultiplier * ft + joint.thrustPhase)) + joint.thrustOffset;
            } else {
                jointTranslations.z = joint.thrust * Math.sin
                    (filter.degToRad(modifiers.thrustFrequencyMultiplier * ft + joint.thrustPhase)) + joint.thrustOffset;
            }
            return jointTranslations;
        },

        // helper function for renderMotion(). calculate joint rotations based on animation file settings and frequency * time
        calculateRotations: function(jointName, animation, ft, direction) {
            var joint = animation.joints[jointName];
            var jointRotations = {x:0, y:0, z:0};

            if (avatar.blenderPreRotations) {
                jointRotations = Vec3.sum(jointRotations, walkAssets.blenderPreRotations.joints[jointName]);
            }

            // gather frequency multipliers for this joint
            modifiers = new FrequencyMultipliers(joint, direction);

            // calculate rotations. Use synthesis filters where specified by the animation data file.

            // calculate pitch
            if (animation.filters.hasOwnProperty(jointName) &&
               'pitchFilter' in animation.filters[jointName]) {
                jointRotations.x += joint.pitch * animation.filters[jointName].pitchFilter.calculate
                    (filter.degToRad(ft * modifiers.pitchFrequencyMultiplier + joint.pitchPhase)) + joint.pitchOffset;
            } else {
                jointRotations.x += joint.pitch * Math.sin
                    (filter.degToRad(ft * modifiers.pitchFrequencyMultiplier + joint.pitchPhase)) + joint.pitchOffset;
            }
            // calculate yaw
            if (animation.filters.hasOwnProperty(jointName) &&
               'yawFilter' in animation.filters[jointName]) {
                jointRotations.y += joint.yaw * animation.filters[jointName].yawFilter.calculate
                    (filter.degToRad(ft * modifiers.yawFrequencyMultiplier + joint.yawPhase)) + joint.yawOffset;
            } else {
                jointRotations.y += joint.yaw * Math.sin
                    (filter.degToRad(ft * modifiers.yawFrequencyMultiplier + joint.yawPhase)) + joint.yawOffset;
            }
            // calculate roll
            if (animation.filters.hasOwnProperty(jointName) &&
               'rollFilter' in animation.filters[jointName]) {
                jointRotations.z += joint.roll * animation.filters[jointName].rollFilter.calculate
                    (filter.degToRad(ft * modifiers.rollFrequencyMultiplier + joint.rollPhase)) + joint.rollOffset;
            } else {
                jointRotations.z += joint.roll * Math.sin
                    (filter.degToRad(ft * modifiers.rollFrequencyMultiplier + joint.rollPhase)) + joint.rollOffset;
            }
            return jointRotations;
        },

        zeroAnimation: function(animation) {
            for (i in animation.joints) {
                for (j in animation.joints[i]) {
                    animation.joints[i][j] = 0;
                }
            }
        },

        blendAnimation: function(sourceAnimation, targetAnimation, percent) {
            for (i in targetAnimation.joints) {
                targetAnimation.joints[i].pitch += percent * sourceAnimation.joints[i].pitch;
                targetAnimation.joints[i].yaw += percent * sourceAnimation.joints[i].yaw;
                targetAnimation.joints[i].roll += percent * sourceAnimation.joints[i].roll;
                targetAnimation.joints[i].pitchPhase += percent * sourceAnimation.joints[i].pitchPhase;
                targetAnimation.joints[i].yawPhase += percent * sourceAnimation.joints[i].yawPhase;
                targetAnimation.joints[i].rollPhase += percent * sourceAnimation.joints[i].rollPhase;
                targetAnimation.joints[i].pitchOffset += percent * sourceAnimation.joints[i].pitchOffset;
                targetAnimation.joints[i].yawOffset += percent * sourceAnimation.joints[i].yawOffset;
                targetAnimation.joints[i].rollOffset += percent * sourceAnimation.joints[i].rollOffset;
                if (i === "Hips") {
                    // Hips only
                    targetAnimation.joints[i].thrust += percent * sourceAnimation.joints[i].thrust;
                    targetAnimation.joints[i].sway += percent * sourceAnimation.joints[i].sway;
                    targetAnimation.joints[i].bob += percent * sourceAnimation.joints[i].bob;
                    targetAnimation.joints[i].thrustPhase += percent * sourceAnimation.joints[i].thrustPhase;
                    targetAnimation.joints[i].swayPhase += percent * sourceAnimation.joints[i].swayPhase;
                    targetAnimation.joints[i].bobPhase += percent * sourceAnimation.joints[i].bobPhase;
                    targetAnimation.joints[i].thrustOffset += percent * sourceAnimation.joints[i].thrustOffset;
                    targetAnimation.joints[i].swayOffset += percent * sourceAnimation.joints[i].swayOffset;
                    targetAnimation.joints[i].bobOffset += percent * sourceAnimation.joints[i].bobOffset;
                }
            }
        },

        deepCopy: function(sourceAnimation, targetAnimation) {
            // calibration
            targetAnimation.calibration = JSON.parse(JSON.stringify(sourceAnimation.calibration));

            // harmonics
            targetAnimation.harmonics = {};
            if (sourceAnimation.harmonics) {
                targetAnimation.harmonics = JSON.parse(JSON.stringify(sourceAnimation.harmonics));
            }

            // filters
            targetAnimation.filters = {};
            for (i in sourceAnimation.filters) {
                // are any filters specified for this joint?
                if (sourceAnimation.filters[i]) {
                    targetAnimation.filters[i] = sourceAnimation.filters[i];
                    // wave shapers
                    if (sourceAnimation.filters[i].pitchFilter) {
                        targetAnimation.filters[i].pitchFilter = sourceAnimation.filters[i].pitchFilter;
                    }
                    if (sourceAnimation.filters[i].yawFilter) {
                        targetAnimation.filters[i].yawFilter = sourceAnimation.filters[i].yawFilter;
                    }
                    if (sourceAnimation.filters[i].rollFilter) {
                        targetAnimation.filters[i].rollFilter = sourceAnimation.filters[i].rollFilter;
                    }
                    // LP filters
                    if (sourceAnimation.filters[i].swayLPFilter) {
                        targetAnimation.filters[i].swayLPFilter = sourceAnimation.filters[i].swayLPFilter;
                    }
                    if (sourceAnimation.filters[i].bobLPFilter) {
                        targetAnimation.filters[i].bobLPFilter = sourceAnimation.filters[i].bobLPFilter;
                    }
                    if (sourceAnimation.filters[i].thrustLPFilter) {
                        targetAnimation.filters[i].thrustLPFilter = sourceAnimation.filters[i].thrustLPFilter;
                    }
                }
            }
            // joints
            targetAnimation.joints = JSON.parse(JSON.stringify(sourceAnimation.joints));
        }
    }

})(); // end animation object literal

// ReachPose datafile wrapper object
ReachPose = function(reachPoseName) {
    this.name = reachPoseName;
    this.reachPoseParameters = walkAssets.getReachPoseParameters(reachPoseName);
    this.reachPoseDataFile = walkAssets.getReachPoseDataFile(reachPoseName);
    this.progress = 0;
    this.smoothingFilter = filter.createAveragingFilter(this.reachPoseParameters.smoothing);
    this.currentStrength = function() {
        // apply optionally smoothed (D)ASDR envelope to reach pose's strength / influence whilst active
        var segmentProgress = undefined; // progress through chosen segment
        var segmentTimeDelta = undefined; // total change in time over chosen segment
        var segmentStrengthDelta = undefined; // total change in strength over chosen segment
        var lastStrength = undefined; // the last value the previous segment held
        var currentStrength = undefined; // return value

        // select parameters based on segment (a segment being one of (D),A,S,D or R)
        if (this.progress >= this.reachPoseParameters.sustain.timing) {
            // release segment
            segmentProgress = this.progress - this.reachPoseParameters.sustain.timing;
            segmentTimeDelta = this.reachPoseParameters.release.timing - this.reachPoseParameters.sustain.timing;
            segmentStrengthDelta = this.reachPoseParameters.release.strength - this.reachPoseParameters.sustain.strength;
            lastStrength = this.reachPoseParameters.sustain.strength;
        } else if (this.progress >= this.reachPoseParameters.decay.timing) {
            // sustain phase
            segmentProgress = this.progress - this.reachPoseParameters.decay.timing;
            segmentTimeDelta = this.reachPoseParameters.sustain.timing - this.reachPoseParameters.decay.timing;
            segmentStrengthDelta = this.reachPoseParameters.sustain.strength - this.reachPoseParameters.decay.strength;
            lastStrength = this.reachPoseParameters.decay.strength;
        } else if (this.progress >= this.reachPoseParameters.attack.timing) {
            // decay phase
            segmentProgress = this.progress - this.reachPoseParameters.attack.timing;
            segmentTimeDelta = this.reachPoseParameters.decay.timing - this.reachPoseParameters.attack.timing;
            segmentStrengthDelta = this.reachPoseParameters.decay.strength - this.reachPoseParameters.attack.strength;
            lastStrength = this.reachPoseParameters.attack.strength;
        } else if (this.progress >= this.reachPoseParameters.delay.timing) {
            // attack phase
            segmentProgress = this.progress - this.reachPoseParameters.delay.timing;
            segmentTimeDelta = this.reachPoseParameters.attack.timing - this.reachPoseParameters.delay.timing;
            segmentStrengthDelta = this.reachPoseParameters.attack.strength - this.reachPoseParameters.delay.strength;
            lastStrength = 0; //this.delay.strength;
        } else {
            // delay phase
            segmentProgress = this.progress;
            segmentTimeDelta = this.reachPoseParameters.delay.timing;
            segmentStrengthDelta = this.reachPoseParameters.delay.strength;
            lastStrength = 0;
        }
        currentStrength = segmentTimeDelta > 0 ? lastStrength + segmentStrengthDelta * segmentProgress / segmentTimeDelta
                                               : lastStrength;
        // smooth off the response curve
        currentStrength = this.smoothingFilter.process(currentStrength);
        return currentStrength;
    }
};

// constructor with default parameters
TransitionParameters = function() {
    this.duration = 0.5;
    this.easingLower = {x:0.25, y:0.75};
    this.easingUpper = {x:0.75, y:0.25};
    this.reachPoses = [];
}

const QUARTER_CYCLE = 90;
const HALF_CYCLE = 180;
const THREE_QUARTER_CYCLE = 270;
const FULL_CYCLE = 360;

// constructor for animation Transition
Transition = function(nextAnimation, lastAnimation, lastTransition, playTransitionReachPoses) {

    if (playTransitionReachPoses === undefined) {
        playTransitionReachPoses = true;
    }

    // record the current state of animation
    this.nextAnimation = nextAnimation;
    this.lastAnimation = lastAnimation;
    this.lastTransition = lastTransition;

    // collect information about the currently playing animation
    this.direction = motion.direction;
    this.lastDirection = motion.lastDirection;
    this.lastFrequencyTimeWheelPos = motion.frequencyTimeWheelPos;
    this.lastFrequencyTimeIncrement = motion.averageFrequencyTimeIncrement;
    this.lastFrequencyTimeWheelRadius = motion.frequencyTimeWheelRadius;
    this.degreesToTurn = 0; // total degrees to turn the ft wheel before the avatar stops (walk only)
    this.degreesRemaining = 0; // remaining degrees to turn the ft wheel before the avatar stops (walk only)
    this.lastElapsedFTDegrees = motion.elapsedFTDegrees; // degrees elapsed since last transition start
    motion.elapsedFTDegrees = 0; // reset ready for the next transition
    motion.frequencyTimeWheelPos = 0; // start the next animation's frequency time wheel from zero

    // set parameters for the transition
    this.parameters = new TransitionParameters();
    this.liveReachPoses = [];
    if (walkAssets && lastAnimation && nextAnimation) {
        // overwrite this.parameters with any transition parameters specified for this particular transition
        walkAssets.getTransitionParameters(lastAnimation, nextAnimation, this.parameters);
        // fire up any reach poses for this transition
        if (playTransitionReachPoses) {
            for (poseName in this.parameters.reachPoses) {
                this.liveReachPoses.push(new ReachPose(this.parameters.reachPoses[poseName]));
            }
        }
    }
    this.startTime = new Date().getTime(); // Starting timestamp (seconds)
    this.progress = 0; // how far are we through the transition?
    this.filteredProgress = 0;

    // coming to a halt whilst walking? if so, will need a clean stopping point defined
    if (motion.isComingToHalt) {

        const FULL_CYCLE_THRESHOLD = 320;
        const HALF_CYCLE_THRESHOLD = 140;
        const CYCLE_COMMIT_THRESHOLD = 5;

        // how many degrees do we need to turn the walk wheel to finish walking with both feet on the ground?
        if (this.lastElapsedFTDegrees < CYCLE_COMMIT_THRESHOLD) {
            // just stop the walk cycle right here and blend to idle
            this.degreesToTurn = 0;
        } else if (this.lastElapsedFTDegrees < HALF_CYCLE) {
            // we have not taken a complete step yet, so we advance to the second stop angle
            this.degreesToTurn = HALF_CYCLE  - this.lastFrequencyTimeWheelPos;
        } else if (this.lastFrequencyTimeWheelPos > 0 && this.lastFrequencyTimeWheelPos <= HALF_CYCLE_THRESHOLD) {
            // complete the step and stop at 180
            this.degreesToTurn = HALF_CYCLE - this.lastFrequencyTimeWheelPos;
        } else if (this.lastFrequencyTimeWheelPos > HALF_CYCLE_THRESHOLD && this.lastFrequencyTimeWheelPos <= HALF_CYCLE) {
            // complete the step and next then stop at 0
            this.degreesToTurn = HALF_CYCLE - this.lastFrequencyTimeWheelPos + HALF_CYCLE;
        } else if (this.lastFrequencyTimeWheelPos > HALF_CYCLE && this.lastFrequencyTimeWheelPos <= FULL_CYCLE_THRESHOLD) {
            // complete the step and stop at 0
            this.degreesToTurn = FULL_CYCLE - this.lastFrequencyTimeWheelPos;
        } else {
            // complete the step and the next then stop at 180
            this.degreesToTurn = FULL_CYCLE - this.lastFrequencyTimeWheelPos + HALF_CYCLE;
        }

        // transition length in this case should be directly proportional to the remaining degrees to turn
        var MIN_FT_INCREMENT = 5.0; // degrees per frame
        var MIN_TRANSITION_DURATION = 0.4;
        const TWO_THIRDS = 0.6667;
        this.lastFrequencyTimeIncrement *= TWO_THIRDS; // help ease the transition
        var lastFrequencyTimeIncrement = this.lastFrequencyTimeIncrement > MIN_FT_INCREMENT ?
                                         this.lastFrequencyTimeIncrement : MIN_FT_INCREMENT;
        var timeToFinish = Math.max(motion.deltaTime * this.degreesToTurn / lastFrequencyTimeIncrement,
                                    MIN_TRANSITION_DURATION);
        this.parameters.duration = timeToFinish;
        this.degreesRemaining = this.degreesToTurn;
    }

    // deal with transition recursion (overlapping transitions)
    this.recursionDepth = 0;
    this.incrementRecursion = function() {
        this.recursionDepth += 1;

        // cancel any continued motion
        this.degreesToTurn = 0;

        // limit the number of layered / nested transitions
        if (this.lastTransition !== nullTransition) {
            this.lastTransition.incrementRecursion();
            if (this.lastTransition.recursionDepth > MAX_TRANSITION_RECURSION) {
                this.lastTransition = nullTransition;
            }
        }
    };
    if (this.lastTransition !== nullTransition) {
        this.lastTransition.incrementRecursion();
    }

    // end of transition initialisation. begin Transition public methods

    // keep up the pace for the frequency time wheel for the last animation
    this.advancePreviousFrequencyTimeWheel = function(deltaTime) {
        var wheelAdvance = undefined;

        if (this.lastAnimation === avatar.selectedWalkBlend &&
            this.nextAnimation === avatar.selectedIdle) {
            if (this.degreesRemaining <= 0) {
                // stop continued motion
                wheelAdvance = 0;
                if (motion.isComingToHalt) {
                    if (this.lastFrequencyTimeWheelPos < QUARTER_CYCLE) {
                        this.lastFrequencyTimeWheelPos = 0;
                    } else {
                        this.lastFrequencyTimeWheelPos = HALF_CYCLE;
                    }
                }
            } else {
                wheelAdvance = this.lastFrequencyTimeIncrement;
                var distanceToTravel = avatar.calibration.strideLength * wheelAdvance / HALF_CYCLE;
                if (this.degreesRemaining <= 0) {
                    distanceToTravel = 0;
                    this.degreesRemaining = 0;
                } else {
                    this.degreesRemaining -= wheelAdvance;
                }
            }
        } else {
            wheelAdvance = this.lastFrequencyTimeIncrement;
        }

        // advance the ft wheel
        this.lastFrequencyTimeWheelPos += wheelAdvance;
        if (this.lastFrequencyTimeWheelPos >= FULL_CYCLE) {
            this.lastFrequencyTimeWheelPos = this.lastFrequencyTimeWheelPos % FULL_CYCLE;
        }

        // advance ft wheel for the nested (previous) Transition
        if (this.lastTransition !== nullTransition) {
            this.lastTransition.advancePreviousFrequencyTimeWheel(deltaTime);
        }
        // update the lastElapsedFTDegrees for short stepping
        this.lastElapsedFTDegrees += wheelAdvance;
        this.degreesTurned += wheelAdvance;
    };

    this.updateProgress = function() {
        const MILLISECONDS_CONVERT = 1000;
        const ACCURACY_INCREASER = 1000;
        var elapasedTime = (new Date().getTime() - this.startTime) / MILLISECONDS_CONVERT;
        this.progress = elapasedTime / this.parameters.duration;
        this.progress = Math.round(this.progress * ACCURACY_INCREASER) / ACCURACY_INCREASER;

        // updated nested transition/s
        if (this.lastTransition !== nullTransition) {
            if (this.lastTransition.updateProgress() === TRANSITION_COMPLETE) {
                // the previous transition is now complete
                this.lastTransition = nullTransition;
            }
        }

        // update any reachPoses
        for (pose in this.liveReachPoses) {
            // use independent timing for reachPoses
            this.liveReachPoses[pose].progress += (motion.deltaTime / this.liveReachPoses[pose].reachPoseParameters.duration);
            if (this.liveReachPoses[pose].progress >= 1) {
                // time to kill off this reach pose
                this.liveReachPoses.splice(pose, 1);
            }
        }

        // update transition progress
        this.filteredProgress = filter.bezier(this.progress, this.parameters.easingLower, this.parameters.easingUpper);
        return this.progress >= 1 ? TRANSITION_COMPLETE : false;
    };

    this.blendTranslations = function(frequencyTimeWheelPos, direction) {
        var lastTranslations = {x:0, y:0, z:0};
        var nextTranslations = animationOperations.calculateTranslations(this.nextAnimation,
                                                                         frequencyTimeWheelPos,
                                                                         direction);
        // are we blending with a previous, still live transition?
        if (this.lastTransition !== nullTransition) {
            lastTranslations = this.lastTransition.blendTranslations(this.lastFrequencyTimeWheelPos,
                                                                     this.lastDirection);
        } else {
            lastTranslations = animationOperations.calculateTranslations(this.lastAnimation,
                                                     this.lastFrequencyTimeWheelPos,
                                                     this.lastDirection);
        }

        // blend last / next translations
        nextTranslations = Vec3.multiply(this.filteredProgress, nextTranslations);
        lastTranslations = Vec3.multiply((1 - this.filteredProgress), lastTranslations);
        nextTranslations = Vec3.sum(nextTranslations, lastTranslations);

        if (this.liveReachPoses.length > 0) {
            for (pose in this.liveReachPoses) {
                var reachPoseStrength = this.liveReachPoses[pose].currentStrength();
                var poseTranslations = animationOperations.calculateTranslations(
                                                             this.liveReachPoses[pose].reachPoseDataFile,
                                                             frequencyTimeWheelPos,
                                                             direction);

                // can't use Vec3 operations here, as if x,y or z is zero, the reachPose should have no influence at all
                if (Math.abs(poseTranslations.x) > 0) {
                    nextTranslations.x = reachPoseStrength * poseTranslations.x + (1 - reachPoseStrength) * nextTranslations.x;
                }
                if (Math.abs(poseTranslations.y) > 0) {
                    nextTranslations.y = reachPoseStrength * poseTranslations.y + (1 - reachPoseStrength) * nextTranslations.y;
                }
                if (Math.abs(poseTranslations.z) > 0) {
                    nextTranslations.z = reachPoseStrength * poseTranslations.z + (1 - reachPoseStrength) * nextTranslations.z;
                }
            }
        }
        return nextTranslations;
    };

    this.blendRotations = function(jointName, frequencyTimeWheelPos, direction) {
        var lastRotations = {x:0, y:0, z:0};
        var nextRotations = animationOperations.calculateRotations(jointName,
                                               this.nextAnimation,
                                               frequencyTimeWheelPos,
                                               direction);

        // are we blending with a previous, still live transition?
        if (this.lastTransition !== nullTransition) {
            lastRotations = this.lastTransition.blendRotations(jointName,
                                                               this.lastFrequencyTimeWheelPos,
                                                               this.lastDirection);
        } else {
            lastRotations = animationOperations.calculateRotations(jointName,
                                               this.lastAnimation,
                                               this.lastFrequencyTimeWheelPos,
                                               this.lastDirection);
        }
        // blend last / next translations
        nextRotations = Vec3.multiply(this.filteredProgress, nextRotations);
        lastRotations = Vec3.multiply((1 - this.filteredProgress), lastRotations);
        nextRotations = Vec3.sum(nextRotations, lastRotations);

        // are there reachPoses defined for this transition?
        if (this.liveReachPoses.length > 0) {
            for (pose in this.liveReachPoses) {
                var reachPoseStrength = this.liveReachPoses[pose].currentStrength();
                var poseRotations = animationOperations.calculateRotations(jointName,
                                                       this.liveReachPoses[pose].reachPoseDataFile,
                                                       frequencyTimeWheelPos,
                                                       direction);

                // don't use Vec3 operations here, as if x,y or z is zero, the reach pose should have no influence at all
                if (Math.abs(poseRotations.x) > 0) {
                    nextRotations.x = reachPoseStrength * poseRotations.x + (1 - reachPoseStrength) * nextRotations.x;
                }
                if (Math.abs(poseRotations.y) > 0) {
                    nextRotations.y = reachPoseStrength * poseRotations.y + (1 - reachPoseStrength) * nextRotations.y;
                }
                if (Math.abs(poseRotations.z) > 0) {
                    nextRotations.z = reachPoseStrength * poseRotations.z + (1 - reachPoseStrength) * nextRotations.z;
                }
            }
        }
        return nextRotations;
    };
}; // end Transition constructor

// individual joint modifiers
FrequencyMultipliers = function(joint, direction) {
    // gather multipliers
    this.pitchFrequencyMultiplier = 1;
    this.yawFrequencyMultiplier = 1;
    this.rollFrequencyMultiplier = 1;
    this.swayFrequencyMultiplier = 1;
    this.bobFrequencyMultiplier = 1;
    this.thrustFrequencyMultiplier = 1;

    if (joint) {
        if (joint.pitchFrequencyMultiplier) {
            this.pitchFrequencyMultiplier = joint.pitchFrequencyMultiplier;
        }
        if (joint.yawFrequencyMultiplier) {
            this.yawFrequencyMultiplier = joint.yawFrequencyMultiplier;
        }
        if (joint.rollFrequencyMultiplier) {
            this.rollFrequencyMultiplier = joint.rollFrequencyMultiplier;
        }
        if (joint.swayFrequencyMultiplier) {
            this.swayFrequencyMultiplier = joint.swayFrequencyMultiplier;
        }
        if (joint.bobFrequencyMultiplier) {
            this.bobFrequencyMultiplier = joint.bobFrequencyMultiplier;
        }
        if (joint.thrustFrequencyMultiplier) {
            this.thrustFrequencyMultiplier = joint.thrustFrequencyMultiplier;
        }
    }
};
