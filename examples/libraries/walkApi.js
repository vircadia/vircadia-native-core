//
//  walkObjects.js
//
//  version 1.003
//
//  Created by David Wooldridge, Autumn 2014
//
//  Motion, state and Transition objects for use by the walk.js script v1.12
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// constructor for the Motion object
Motion = function() {

    // selected animations
    this.selectedWalk = undefined;
    this.selectedStand = undefined;
    this.selectedFly = undefined;
    this.selectedFlyUp = undefined;
    this.selectedFlyDown = undefined;
    this.selectedFlyBlend = undefined;
    this.selectedHovering = undefined;
    this.selectedSideStepLeft = undefined;
    this.selectedSideStepRight = undefined;

    // if Hydras are connected, the only way to enable use is by never setting any rotations on the arm joints
    this.hydraCheck = function() {

        // function courtesy of Thijs Wenker (frisbee.js)
        var numberOfButtons = Controller.getNumberOfButtons();
        var numberOfTriggers = Controller.getNumberOfTriggers();
        var numberOfSpatialControls = Controller.getNumberOfSpatialControls();
        var controllersPerTrigger = numberOfSpatialControls / numberOfTriggers;
        hydrasConnected = (numberOfButtons == 12 && numberOfTriggers == 2 && controllersPerTrigger == 2);
        return hydrasConnected;
    }

    // settings
    this.armsFree = this.hydraCheck(); // automatically sets true for Hydra support - temporary fix
    this.makesFootStepSounds = true;
    this.avatarGender = undefined;
    this.setGender = function(gender) {

        this.avatarGender = gender;

        switch(this.avatarGender) {

            case MALE:

                this.selectedWalk = walkAssets.maleStandardWalk;
                this.selectedStand = walkAssets.maleStandOne;
                this.selectedFlyUp = walkAssets.maleFlyingUp;
                this.selectedFly = walkAssets.maleFlying;
                this.selectedFlyDown = walkAssets.maleFlyingDown;
                this.selectedFlyBlend = walkAssets.maleFlyingBlend;
                this.selectedHovering = walkAssets.maleHovering;
                this.selectedSideStepLeft = walkAssets.maleSideStepLeft;
                this.selectedSideStepRight = walkAssets.maleSideStepRight;
                this.currentAnimation = this.selectedStand;
                return;

            case FEMALE:

                this.selectedWalk = walkAssets.femaleStandardWalk;
                this.selectedStand = walkAssets.femaleStandOne;
                this.selectedFlyUp = walkAssets.femaleFlyingUp;
                this.selectedFly = walkAssets.femaleFlying;
                this.selectedFlyDown = walkAssets.femaleFlyingDown;
                this.selectedFlyBlend = walkAssets.femaleFlyingBlend;
                this.selectedHovering = walkAssets.femaleHovering;
                this.selectedSideStepLeft = walkAssets.femaleSideStepLeft;
                this.selectedSideStepRight = walkAssets.femaleSideStepRight;
                this.currentAnimation = this.selectedStand;
                return;
        }
    }
    this.setGender(MALE);

    // calibration
    this.calibration = {

        pitchMax: 8,
        maxWalkAcceleration: 5,
        maxWalkDeceleration: 25,
        rollMax: 80,
        angularVelocityMax: 70,
        hipsToFeet: MyAvatar.getJointPosition("Hips").y - MyAvatar.getJointPosition("RightFoot").y,
    }

    // used to make sure at least one step has been taken when transitioning from a walk cycle
    this.elapsedFTDegrees = 0;

    // the current animation and transition
    this.currentAnimation = this.selectedStand;
    this.currentTransition = null;

    // zero out avi's joints and pose the fingers
    this.avatarJointNames = MyAvatar.getJointNames();
    this.poseFingers = function() {

        for (var i = 0; i < this.avatarJointNames.length; i++) {

            if (i > 17 || i < 34) {
                // left hand fingers
                MyAvatar.setJointData(this.avatarJointNames[i], Quat.fromPitchYawRollDegrees(16, 0, 0));
            } else if (i > 33 || i < 38) {
                // left hand thumb
                MyAvatar.setJointData(this.avatarJointNames[i], Quat.fromPitchYawRollDegrees(4, 0, 0));
            } else if (i > 41 || i < 58) {
                // right hand fingers
                MyAvatar.setJointData(this.avatarJointNames[i], Quat.fromPitchYawRollDegrees(16, 0, 0));
            } else if (i > 57 || i < 62) {
                // right hand thumb
                MyAvatar.setJointData(this.avatarJointNames[i], Quat.fromPitchYawRollDegrees(4, 0, 0));
            }
        }
    };
    if (!this.armsFree) {
        this.poseFingers();
    }

    // frequency time wheel (foot / ground speed matching)
    this.direction = FORWARDS;
    this.strideLength = this.selectedWalk.calibration.strideLengthForwards;
    this.frequencyTimeWheelPos = 0;
    this.frequencyTimeWheelRadius = 0.5;
    this.recentFrequencyTimeIncrements = [];
    for(var i = 0; i < 8; i++) {

        this.recentFrequencyTimeIncrements.push(0);
    }
    this.averageFrequencyTimeIncrement = 0;

    this.advanceFrequencyTimeWheel = function(angle){

        this.elapsedFTDegrees += angle;

        this.recentFrequencyTimeIncrements.push(angle);
        this.recentFrequencyTimeIncrements.shift();
        for(increment in this.recentFrequencyTimeIncrements) {
            this.averageFrequencyTimeIncrement += this.recentFrequencyTimeIncrements[increment];
        }
        this.averageFrequencyTimeIncrement /= this.recentFrequencyTimeIncrements.length;

        this.frequencyTimeWheelPos += angle;
        if (this.frequencyTimeWheelPos >= 360) {
            this.frequencyTimeWheelPos = this.frequencyTimeWheelPos % 360;
        }
    }

    // currently disabled due to breaking changes in js audio native objects
    this.playFootStepSound = function(side) {

        //var options = new AudioInjectionOptions();
        //options.position = Camera.getPosition();
        //options.volume = 0.3;
        //var soundNumber = 2; // 0 to 2
        //if (side === RIGHT && motion.makesFootStepSounds) {
        //    Audio.playS ound(walkAssets.footsteps[soundNumber + 1], options);
        //} else if (side === LEFT && motion.makesFootStepSounds) {
        //    Audio.playSound(walkAssets.footsteps[soundNumber], options);
        //}
    }

    // history
    this.lastDirection = 0;
    this.lastSpeed = 0;
    this.transitionCount = 0;
    this.lastDistanceToVoxels = 0;

};  // end Motion constructor


spatialInformation = (function() {

    return {

        distanceToVoxels: function() {

            // use the blocking version of findRayIntersection to avoid errors
            var pickRay = {origin: MyAvatar.position, direction: {x:0, y:-1, z:0}};
            return Voxels.findRayIntersectionBlocking(pickRay).distance - motion.calibration.hipsToFeet;;
        }
    }

})(); // end spatialInformation object literal


// animation file manipulation
animation = (function() {

    return {

        zeroAnimation: function(animation) {

            for (i in animation.joints) {

                animation.joints[i].pitch = 0;
                animation.joints[i].yaw = 0;
                animation.joints[i].roll = 0;
                animation.joints[i].pitchPhase = 0;
                animation.joints[i].yawPhase = 0;
                animation.joints[i].rollPhase = 0;
                animation.joints[i].pitchOffset = 0;
                animation.joints[i].yawOffset = 0;
                animation.joints[i].rollOffset = 0;
                if (i === 0) {
                    // Hips only
                    animation.joints[i].thrust = 0;
                    animation.joints[i].sway = 0;
                    animation.joints[i].bob = 0;
                    animation.joints[i].thrustPhase = 0;
                    animation.joints[i].swayPhase = 0;
                    animation.joints[i].bobPhase = 0;
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
                if (i === 0) {
                    // Hips only
                    targetAnimation.joints[i].thrust += percent * sourceAnimation.joints[i].thrust;
                    targetAnimation.joints[i].sway += percent * sourceAnimation.joints[i].sway;
                    targetAnimation.joints[i].bob += percent * sourceAnimation.joints[i].bob;
                    targetAnimation.joints[i].thrustPhase += percent * sourceAnimation.joints[i].thrustPhase;
                    targetAnimation.joints[i].swayPhase += percent * sourceAnimation.joints[i].swayPhase;
                    targetAnimation.joints[i].bobPhase += percent * sourceAnimation.joints[i].bobPhase;
                }
            }
        }
    }
})(); // end animation object literal

// finite state machine. Avatar locomotion modes represent states in the FSM
state = (function () {

    return {

        // the finite list of states
        STANDING: 1,
        WALKING: 2,
        SIDE_STEP: 3,
        FLYING: 4,
        currentState: this.STANDING,

        // status vars
        powerOn: true,

        setInternalState: function(newInternalState) {

            motion.elapsedFTDegrees = 0;

            switch (newInternalState) {

                case this.WALKING:

                    this.currentState = this.WALKING;
                    return;

                case this.FLYING:

                    this.currentState = this.FLYING;
                    return;

                case this.SIDE_STEP:

                    this.currentState = this.SIDE_STEP;
                    return;

                case this.STANDING:
                default:

                    this.currentState = this.STANDING;
                    return;
            }
        }
    }
})(); // end state object literal

// constructor for animation Transition
Transition = function(nextAnimation, lastAnimation, lastTransition) {

    // record the current state of animation
    this.nextAnimation = nextAnimation;
    this.lastAnimation = lastAnimation;
    this.lastTransition = lastTransition;

    // deal with transition recursion (overlapping transitions)
    this.id = motion.transitionCount++; // serial number for this transition
    this.recursionDepth = 0;
    this.incrementRecursion = function() {

        this.recursionDepth += 1;

        if(this.lastTransition !== nullTransition) {

            this.lastTransition.incrementRecursion();
            if(this.lastTransition.recursionDepth > 5) {

                delete this.lastTransition;
                this.lastTransition = nullTransition;
            }
        }
    }
    if(lastTransition !== nullTransition) {

        this.lastTransition.incrementRecursion();
    }

    this.reachPoses = []; // placeholder / stub: work in progress - array of reach poses for squash and stretch techniques
    this.transitionDuration = 0.5; // length of transition (seconds)
    this.easingLower = {x:0.60, y:0.03}; // Bezier curve handle
    this.easingUpper = {x:0.87, y:1.35}; // Bezier curve handle
    this.startTime = new Date().getTime(); // Starting timestamp (seconds)
    this.progress = 0; // how far are we through the transition?
    this.lastDirection = motion.lastDirection;

    // collect information about the currently playing animation
    this.lastDirection = motion.lastDirection;
    this.lastFrequencyTimeWheelPos = motion.frequencyTimeWheelPos;
    this.lastFrequencyTimeIncrement = motion.averageFrequencyTimeIncrement;
    this.lastFrequencyTimeWheelRadius = motion.frequencyTimeWheelRadius;
    this.stopAngle = 0; // what angle should we stop turning this frequency time wheel?
    this.percentToMove = 0; // if we need to keep moving to complete a step, this will be set to 100
    this.lastElapsedFTDegrees = motion.elapsedFTDegrees;
    this.advancePreviousFrequencyTimeWheel = function() {

        this.lastFrequencyTimeWheelPos += this.lastFrequencyTimeIncrement;

        if (this.lastFrequencyTimeWheelPos >= 360) {

            this.lastFrequencyTimeWheelPos = this.lastFrequencyTimeWheelPos % 360;
        }

        if(this.lastTransition !== nullTransition) {

            this.lastTransition.advancePreviousFrequencyTimeWheel();
        }
    };

    this.updateProgress = function() {

        var elapasedTime = (new Date().getTime() - this.startTime) / 1000;
        this.progress = elapasedTime / this.transitionDuration;

        this.progress = filter.bezier((1 - this.progress),
                            {x: 0, y: 0},
                            this.easingLower,
                            this.easingUpper,
                            {x: 1, y: 1}).y;

        // ensure there is at least some progress
        if(this.progress <= 0) {

            this.progress = VERY_SHORT_TIME;
        }
        if(this.lastTransition !== nullTransition) {

            if(this.lastTransition.updateProgress() >= 1) {

                // the previous transition is now complete
                delete this.lastTransition;
                this.lastTransition = nullTransition;
            }
        }
        return this.progress;
    };

    this.blendTranslations = function(frequencyTimeWheelPos, direction) {

        var lastTranslations = {x:0, y:0, z:0};
        if(!isDefined(this.nextAnimation)) {

            return lastTranslations;
        }
        var nextTranslations = calculateTranslations(this.nextAnimation,
                                                     frequencyTimeWheelPos,
                                                     direction);

        // are we blending with a previous, still live transition?
        if(this.lastTransition !== nullTransition) {

            lastTranslations = this.lastTransition.blendTranslations(this.lastFrequencyTimeWheelPos,
                                                                     this.lastDirection);

        } else {

            lastTranslations = calculateTranslations(this.lastAnimation,
                                                     this.lastFrequencyTimeWheelPos,
                                                     this.lastDirection);
        }

        nextTranslations.x = this.progress * nextTranslations.x +
                             (1 - this.progress) * lastTranslations.x;

        nextTranslations.y = this.progress * nextTranslations.y +
                             (1 - this.progress) * lastTranslations.y;

        nextTranslations.z = this.progress * nextTranslations.z +
                             (1 - this.progress) * lastTranslations.z;

        return nextTranslations;
    };

    this.blendRotations = function(jointName, frequencyTimeWheelPos, direction) {

        var lastRotations = {x:0, y:0, z:0};
        var nextRotations = calculateRotations(jointName,
                                               this.nextAnimation,
                                               frequencyTimeWheelPos,
                                               direction);

        // are we blending with a previous, still live transition?
        if(this.lastTransition !== nullTransition) {

            lastRotations = this.lastTransition.blendRotations(jointName,
                                                               this.lastFrequencyTimeWheelPos,
                                                               this.lastDirection);
        } else {

            lastRotations = calculateRotations(jointName,
                                               this.lastAnimation,
                                               this.lastFrequencyTimeWheelPos,
                                               this.lastDirection);
        }

        nextRotations.x = this.progress * nextRotations.x +
                          (1 - this.progress) * lastRotations.x;

        nextRotations.y = this.progress * nextRotations.y +
                          (1 - this.progress) * lastRotations.y;

        nextRotations.z = this.progress * nextRotations.z +
                          (1 - this.progress) * lastRotations.z;

        return nextRotations;
    };

}; // end Transition constructor


// individual joint modiers (mostly used to provide symmetry between left and right limbs)
JointModifiers = function(joint, direction) {

    // gather modifiers and multipliers
    this.pitchFrequencyMultiplier = 1;
    this.pitchPhaseModifier = 0;
    this.pitchReverseModifier = 0;
    this.yawReverseModifier = 0;
    this.rollReverseModifier = 0;
    this.pitchSign = 1; // for sidestepping and incorrectly rigged Ron ToeBases
    this.yawSign = 1;
    this.rollSign = 1;
    this.pitchReverseInvert = 1;
    this.pitchOffsetSign = 1;
    this.yawOffsetSign = 1;
    this.rollOffsetSign = 1;
    this.bobReverseModifier = 0;
    this.bobFrequencyMultiplier = 1;
    this.thrustFrequencyMultiplier = 1;

    if (isDefined(joint.pitchFrequencyMultiplier)) {
        this.pitchFrequencyMultiplier = joint.pitchFrequencyMultiplier;
    }
    if (isDefined(joint.pitchPhaseModifier)) {
        this.pitchPhaseModifier = joint.pitchPhaseModifier;
    }
    if (isDefined(joint.pitchSign)) {
        this.pitchSign = joint.pitchSign;
    }
    if (isDefined(joint.yawSign)) {
        this.yawSign = joint.yawSign;
    }
    if (isDefined(joint.rollSign)) {
        this.rollSign = joint.rollSign;
    }
    if (isDefined(joint.pitchReverseInvert) && direction === BACKWARDS) {
        this.pitchReverseInvert = joint.pitchReverseInvert;
    }
    if (isDefined(joint.pitchReverseModifier) && direction === BACKWARDS) {
        this.pitchReverseModifier = joint.pitchReverseModifier;
    }
    if (isDefined(joint.yawReverseModifier) && direction === BACKWARDS) {
        this.yawReverseModifier = joint.yawReverseModifier;
    }
    if (isDefined(joint.rollReverseModifier) && direction === BACKWARDS) {
        this.rollReverseModifier = joint.rollReverseModifier;
    }
    if (isDefined(joint.pitchOffsetSign)) {
        this.pitchOffsetSign = joint.pitchOffsetSign;
    }
    if (isDefined(joint.yawOffsetSign)) {
        this.yawOffsetSign = joint.yawOffsetSign;
    }
    if (isDefined(joint.rollOffsetSign)) {
        this.rollOffsetSign = joint.rollOffsetSign;
    }
    if (isDefined(joint.bobReverseModifier) && direction === BACKWARDS) {
        this.bobReverseModifier = joint.bobReverseModifier;
    }
    if (isDefined(joint.bobFrequencyMultiplier)) {
        this.bobFrequencyMultiplier = joint.bobFrequencyMultiplier;
    }
    if (isDefined(joint.thrustFrequencyMultiplier)) {
        this.thrustFrequencyMultiplier = joint.thrustFrequencyMultiplier;
    }
};

walkAssets = (function () {

    // load the sounds - currently disabled due to breaking changes in js audio native objects
    //var _pathToSounds = 'https://s3.amazonaws.com/hifi-public/sounds/Footsteps/';

    //var _footsteps = [];
    //_footsteps.push(new Sound(_pathToSounds+"FootstepW2Left-12db.wav"));
    //_footsteps.push(new Sound(_pathToSounds+"FootstepW2Right-12db.wav"));
    //_footsteps.push(new Sound(_pathToSounds+"FootstepW3Left-12db.wav"));
    //_footsteps.push(new Sound(_pathToSounds+"FootstepW3Right-12db.wav"));
    //_footsteps.push(new Sound(_pathToSounds+"FootstepW5Left-12db.wav"));
    //_footsteps.push(new Sound(_pathToSounds+"FootstepW5Right-12db.wav"));

    // load the animation datafiles
    Script.include(pathToAssets+"animations/dd-female-standard-walk-animation.js");
    Script.include(pathToAssets+"animations/dd-female-standing-one-animation.js");
    Script.include(pathToAssets+"animations/dd-female-flying-up-animation.js");
    Script.include(pathToAssets+"animations/dd-female-flying-animation.js");
    Script.include(pathToAssets+"animations/dd-female-flying-down-animation.js");
    Script.include(pathToAssets+"animations/dd-female-flying-blend-animation.js");
    Script.include(pathToAssets+"animations/dd-female-hovering-animation.js");
    Script.include(pathToAssets+"animations/dd-female-sidestep-left-animation.js");
    Script.include(pathToAssets+"animations/dd-female-sidestep-right-animation.js");
    Script.include(pathToAssets+"animations/dd-male-standard-walk-animation.js");
    Script.include(pathToAssets+"animations/dd-male-standing-one-animation.js");
    Script.include(pathToAssets+"animations/dd-male-flying-up-animation.js");
    Script.include(pathToAssets+"animations/dd-male-flying-animation.js");
    Script.include(pathToAssets+"animations/dd-male-flying-down-animation.js");
    Script.include(pathToAssets+"animations/dd-male-flying-blend-animation.js");
    Script.include(pathToAssets+"animations/dd-male-hovering-animation.js");
    Script.include(pathToAssets+"animations/dd-male-sidestep-left-animation.js");
    Script.include(pathToAssets+"animations/dd-male-sidestep-right-animation.js");
    Script.include(pathToAssets+"animations/dd-animation-reference.js");

    var _femaleStandardWalk = new FemaleStandardWalk();
    var _femaleFlyingUp = new FemaleFlyingUp();
    var _femaleFlying = new FemaleFlying();
    var _femaleFlyingDown = new FemaleFlyingDown();
    var _femaleStandOne = new FemaleStandingOne();
    var _femaleSideStepLeft = new FemaleSideStepLeft();
    var _femaleSideStepRight = new FemaleSideStepRight();
    var _femaleFlyingBlend = new FemaleFlyingBlend();
    var _femaleHovering = new FemaleHovering();

    var _maleStandardWalk = new MaleStandardWalk(filter);
    var _maleStandOne = new MaleStandingOne();
    var _maleSideStepLeft = new MaleSideStepLeft();
    var _maleSideStepRight = new MaleSideStepRight();
    var _maleFlying = new MaleFlying();
    var _maleFlyingDown = new MaleFlyingDown();
    var _maleFlyingUp = new MaleFlyingUp();
    var _maleFlyingBlend = new MaleFlyingBlend();
    var _maleHovering = new MaleHovering();

    var _animationReference = new AnimationReference();

    return {

        // expose the sound assets
        //footsteps: _footsteps,

        // expose the animation assets
        femaleStandardWalk: _femaleStandardWalk,
        femaleFlyingUp: _femaleFlyingUp,
        femaleFlying: _femaleFlying,
        femaleFlyingDown: _femaleFlyingDown,
        femaleFlyingBlend: _femaleFlyingBlend,
        femaleHovering: _femaleHovering,
        femaleStandOne: _femaleStandOne,
        femaleSideStepLeft: _femaleSideStepLeft,
        femaleSideStepRight: _femaleSideStepRight,
        maleStandardWalk: _maleStandardWalk,
        maleFlyingUp: _maleFlyingUp,
        maleFlying: _maleFlying,
        maleFlyingDown: _maleFlyingDown,
        maleFlyingBlend: _maleFlyingBlend,
        maleHovering: _maleHovering,
        maleStandOne: _maleStandOne,
        maleSideStepLeft: _maleSideStepLeft,
        maleSideStepRight: _maleSideStepRight,
        animationReference: _animationReference,
    }

})();