//
//  walkObjects.js
//
//  version 1.000
//
//  Created by David Wooldridge, Autumn 2014
//
//  Motion, state and Transition objects for use by the walk.js script v1.1
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// constructor for the Motion object
Motion = function() {

    this.setGender = function(gender) {

        this.avatarGender = gender;

        switch(this.avatarGender) {

            case MALE:

                this.selWalk = walkAssets.maleStandardWalk;
                this.selStand = walkAssets.maleStandOne;
                this.selFlyUp = walkAssets.maleFlyingUp;
                this.selFly = walkAssets.maleFlying;
                this.selFlyDown = walkAssets.maleFlyingDown;
                this.selSideStepLeft = walkAssets.maleSideStepLeft;
                this.selSideStepRight = walkAssets.maleSideStepRight;
                this.curAnim = this.selStand;
                return;

            case FEMALE:

                this.selWalk = walkAssets.femaleStandardWalk;
                this.selStand = walkAssets.femaleStandOne;
                this.selFlyUp = walkAssets.femaleFlyingUp;
                this.selFly = walkAssets.femaleFlying;
                this.selFlyDown = walkAssets.femaleFlyingDown;
                this.selSideStepLeft = walkAssets.femaleSideStepLeft;
                this.selSideStepRight = walkAssets.femaleSideStepRight;
                this.curAnim = this.selStand;
                return;
        }
    }

    this.hydraCheck = function() {

        // function courtesy of Thijs Wenker, frisbee.js
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
    this.avatarGender = MALE;
    this.motionPitchMax = 60;
    this.motionRollMax = 40;

    // timing
    this.frameStartTime = 0; // used for measuring frame execution times
    this.frameExecutionTimeMax = 0; // keep track of the longest frame execution time
    this.cumulativeTime = 0.0;
    this.lastWalkStartTime = 0;

    // selected animations
    this.selWalk = walkAssets.maleStandardWalk;
    this.selStand = walkAssets.maleStandOne;
    this.selFlyUp = walkAssets.maleFlyingUp;
    this.selFly = walkAssets.maleFlying;
    this.selFlyDown = walkAssets.maleFlyingDown;
    this.selSideStepLeft = walkAssets.maleSideStepLeft;
    this.selSideStepRight = walkAssets.maleSideStepRight;

    // the currently selected animation, joint and transition
    this.curAnim = this.selStand;
    this.curJointIndex = 0;
    this.curTransition = null;

    // zero out avi's joints, curl the fingers nicely then take some measurements
    this.avatarJointNames = MyAvatar.getJointNames();
    if (!this.armsFree) {

        for (var i = 0; i < this.avatarJointNames.length; i++) {

            if (i > 17 || i < 34)
                // left hand fingers
                MyAvatar.setJointData(this.avatarJointNames[i], Quat.fromPitchYawRollDegrees(16, 0, 0));

            else if (i > 33 || i < 38)
                // left hand thumb
                MyAvatar.setJointData(this.avatarJointNames[i], Quat.fromPitchYawRollDegrees(4, 0, 0));

            else if (i > 41 || i < 58)
                // right hand fingers
                MyAvatar.setJointData(this.avatarJointNames[i], Quat.fromPitchYawRollDegrees(16, 0, 0));

            else if (i > 57 || i < 62)
                // right hand thumb
                MyAvatar.setJointData(this.avatarJointNames[i], Quat.fromPitchYawRollDegrees(4, 0, 0));

            else
                // zero out the remaining joints
                MyAvatar.clearJointData(this.avatarJointNames[i]);
        }
    }

    this.footRPos = MyAvatar.getJointPosition("RightFoot");
    this.hipsToFeet = MyAvatar.getJointPosition("Hips").y - this.footRPos.y;

    // walkwheel (foot / ground speed matching)
    this.direction = FORWARDS;
    this.nextStep = RIGHT;
    this.nFrames = 0;
    this.strideLength = this.selWalk.calibration.strideLengthForwards;
    this.walkWheelPos = 0;

    this.advanceWalkWheel = function(angle){

        this.walkWheelPos += angle;
        if (motion.walkWheelPos >= 360)
            this.walkWheelPos = this.walkWheelPos % 360;
    }

    // last frame history
    this.lastDirection = 0;
    this.lastVelocity = 0;

};  // end Motion constructor

// finite state machine
state = (function () {

    return {

        // the finite list of states
        STANDING: 1,
        WALKING: 2,
        SIDE_STEP: 3,
        FLYING: 4,
        EDIT_WALK_STYLES: 5,
        EDIT_WALK_TWEAKS: 6,
        EDIT_WALK_JOINTS: 7,
        EDIT_STANDING: 8,
        EDIT_FLYING: 9,
        EDIT_FLYING_UP: 10,
        EDIT_FLYING_DOWN: 11,
        EDIT_SIDESTEP_LEFT: 12,
        EDIT_SIDESTEP_RIGHT: 14,
        currentState: this.STANDING,

        // status vars
        powerOn: true,
        minimised: true,
        editing:false,
        editingTranslation: false,

        setInternalState: function(newInternalState) {

            switch (newInternalState) {

                case this.WALKING:

                    this.currentState = this.WALKING;
                    this.editing = false;
                    motion.lastWalkStartTime = new Date().getTime();
                    walkInterface.updateMenu();
                    return;

                case this.FLYING:

                    this.currentState = this.FLYING;
                    this.editing = false;
                    motion.lastWalkStartTime = 0;
                    walkInterface.updateMenu();
                    return;

                case this.SIDE_STEP:

                    this.currentState = this.SIDE_STEP;
                    this.editing = false;
                    motion.lastWalkStartTime = new Date().getTime();
                    walkInterface.updateMenu();
                    return;

                case this.EDIT_WALK_STYLES:

                    this.currentState = this.EDIT_WALK_STYLES;
                    this.editing = true;
                    motion.lastWalkStartTime = new Date().getTime();
                    motion.curAnim = motion.selWalk;
                    walkInterface.updateMenu();
                    return;

                case this.EDIT_WALK_TWEAKS:

                    this.currentState = this.EDIT_WALK_TWEAKS;
                    this.editing = true;
                    motion.lastWalkStartTime = new Date().getTime();
                    motion.curAnim = motion.selWalk;
                    walkInterface.updateMenu();
                    return;

                case this.EDIT_WALK_JOINTS:

                    this.currentState = this.EDIT_WALK_JOINTS;
                    this.editing = true;
                    motion.lastWalkStartTime = new Date().getTime();
                    motion.curAnim = motion.selWalk;
                    walkInterface.updateMenu();
                    return;

                case this.EDIT_STANDING:

                    this.currentState = this.EDIT_STANDING;
                    this.editing = true;
                    motion.lastWalkStartTime = 0;
                    motion.curAnim = motion.selStand;
                    walkInterface.updateMenu();
                    return;

                case this.EDIT_SIDESTEP_LEFT:

                    this.currentState = this.EDIT_SIDESTEP_LEFT;
                    this.editing = true;
                    motion.lastWalkStartTime = new Date().getTime();
                    motion.curAnim = motion.selSideStepLeft;
                    walkInterface.updateMenu();
                    return;

                case this.EDIT_SIDESTEP_RIGHT:

                    this.currentState = this.EDIT_SIDESTEP_RIGHT;
                    this.editing = true;
                    motion.lastWalkStartTime = new Date().getTime();
                    motion.curAnim = motion.selSideStepRight;
                    walkInterface.updateMenu();
                    return;

                case this.EDIT_FLYING:

                    this.currentState = this.EDIT_FLYING;
                    this.editing = true;
                    motion.lastWalkStartTime = 0;
                    motion.curAnim = motion.selFly;
                    walkInterface.updateMenu();
                    return;

                case this.EDIT_FLYING_UP:

                    this.currentState = this.EDIT_FLYING_UP;
                    this.editing = true;
                    motion.lastWalkStartTime = 0;
                    motion.curAnim = motion.selFlyUp;
                    walkInterface.updateMenu();
                    return;

                case this.EDIT_FLYING_DOWN:

                    this.currentState = this.EDIT_FLYING_DOWN;
                    this.editing = true;
                    motion.lastWalkStartTime = 0;
                    motion.curAnim = motion.selFlyDown;
                    walkInterface.updateMenu();
                    return;

                case this.STANDING:
                default:

                    this.currentState = this.STANDING;
                    this.editing = false;
                    motion.lastWalkStartTime = 0;
                    motion.curAnim = motion.selStand;
                    walkInterface.updateMenu();

                    // initialisation - runs at script startup only
                    if (motion.strideLength === 0) {

                        motion.setGender(MALE);
                        if (motion.direction === BACKWARDS)
                            motion.strideLength = motion.selWalk.calibration.strideLengthBackwards;
                        else
                            motion.strideLength = motion.selWalk.calibration.strideLengthForwards;
                    }
                    return;
            }
        }
    }
})(); // end state object literal

// constructor for animation Transition
Transition = function(lastAnimation, nextAnimation, reachPoses, transitionDuration, easingLower, easingUpper) {

    this.lastAnim = lastAnimation; // name of last animation
    if (lastAnimation === motion.selWalk ||
        nextAnimation === motion.selSideStepLeft ||
        nextAnimation === motion.selSideStepRight)
        this.walkingAtStart = true; // boolean - is the last animation a walking animation?
    else
        this.walkingAtStart = false; // boolean - is the last animation a walking animation?
    this.nextAnimation = nextAnimation; // name of next animation
    if (nextAnimation === motion.selWalk ||
        nextAnimation === motion.selSideStepLeft ||
        nextAnimation === motion.selSideStepRight)
        this.walkingAtEnd = true; // boolean - is the next animation a walking animation?
    else
        this.walkingAtEnd = false; // boolean - is the next animation a walking animation?
    this.reachPoses = reachPoses; // placeholder / stub: array of reach poses for squash and stretch techniques
    this.transitionDuration = transitionDuration; // length of transition (seconds)
    this.easingLower = easingLower; // Bezier curve handle (normalised)
    this.easingUpper = easingUpper; // Bezier curve handle (normalised)
    this.startTime = new Date().getTime(); // Starting timestamp (seconds)
    this.progress = 0; // how far are we through the transition?
    this.walkWheelIncrement = 6; // how much to turn the walkwheel each frame when transitioning to / from walking
    this.walkWheelAdvance = 0; // how many degrees the walk wheel has been advanced during the transition
    this.walkStopAngle = 0; // what angle should we stop the walk cycle?

}; // end Transition constructor


walkAssets = (function () {

    // path to the sounds used for the footsteps
    var _pathToSounds = 'https://s3.amazonaws.com/hifi-public/sounds/Footsteps/';

    // read in the sounds
    var _footsteps = [];
    _footsteps.push(new Sound(_pathToSounds+"FootstepW2Left-12db.wav"));
    _footsteps.push(new Sound(_pathToSounds+"FootstepW2Right-12db.wav"));
    _footsteps.push(new Sound(_pathToSounds+"FootstepW3Left-12db.wav"));
    _footsteps.push(new Sound(_pathToSounds+"FootstepW3Right-12db.wav"));
    _footsteps.push(new Sound(_pathToSounds+"FootstepW5Left-12db.wav"));
    _footsteps.push(new Sound(_pathToSounds+"FootstepW5Right-12db.wav"));

    // load the animation datafiles
    Script.include(pathToAssets+"animations/dd-female-standard-walk-animation.js");
    Script.include(pathToAssets+"animations/dd-female-flying-up-animation.js");
    Script.include(pathToAssets+"animations/dd-female-flying-animation.js");
    Script.include(pathToAssets+"animations/dd-female-flying-down-animation.js");
    Script.include(pathToAssets+"animations/dd-female-standing-one-animation.js");
    Script.include(pathToAssets+"animations/dd-female-sidestep-left-animation.js");
    Script.include(pathToAssets+"animations/dd-female-sidestep-right-animation.js");
    Script.include(pathToAssets+"animations/dd-male-standard-walk-animation.js");
    Script.include(pathToAssets+"animations/dd-male-flying-up-animation.js");
    Script.include(pathToAssets+"animations/dd-male-flying-animation.js");
    Script.include(pathToAssets+"animations/dd-male-flying-down-animation.js");
    Script.include(pathToAssets+"animations/dd-male-standing-one-animation.js");
    Script.include(pathToAssets+"animations/dd-male-sidestep-left-animation.js");
    Script.include(pathToAssets+"animations/dd-male-sidestep-right-animation.js");

    // read in the animation files
    var _FemaleStandardWalkFile = new FemaleStandardWalk();
    var _femaleStandardWalk = _FemaleStandardWalkFile.loadAnimation();
    var _FemaleFlyingUpFile = new FemaleFlyingUp();
    var _femaleFlyingUp = _FemaleFlyingUpFile.loadAnimation();
    var _FemaleFlyingFile = new FemaleFlying();
    var _femaleFlying = _FemaleFlyingFile.loadAnimation();
    var _FemaleFlyingDownFile = new FemaleFlyingDown();
    var _femaleFlyingDown = _FemaleFlyingDownFile.loadAnimation();
    var _FemaleStandOneFile = new FemaleStandingOne();
    var _femaleStandOne = _FemaleStandOneFile.loadAnimation();
    var _FemaleSideStepLeftFile = new FemaleSideStepLeft();
    var _femaleSideStepLeft = _FemaleSideStepLeftFile.loadAnimation();
    var _FemaleSideStepRightFile = new FemaleSideStepRight();
    var _femaleSideStepRight = _FemaleSideStepRightFile.loadAnimation();
    var _MaleStandardWalkFile = new MaleStandardWalk(filter);
    var _maleStandardWalk = _MaleStandardWalkFile.loadAnimation();
    var _MaleFlyingUpFile = new MaleFlyingUp();
    var _maleFlyingUp = _MaleFlyingUpFile.loadAnimation();
    var _MaleFlyingFile = new MaleFlying();
    var _maleFlying = _MaleFlyingFile.loadAnimation();
    var _MaleFlyingDownFile = new MaleFlyingDown();
    var _maleFlyingDown = _MaleFlyingDownFile.loadAnimation();
    var _MaleStandOneFile = new MaleStandingOne();
    var _maleStandOne = _MaleStandOneFile.loadAnimation();
    var _MaleSideStepLeftFile = new MaleSideStepLeft();
    var _maleSideStepLeft = _MaleSideStepLeftFile.loadAnimation();
    var _MaleSideStepRightFile = new MaleSideStepRight();
    var _maleSideStepRight = _MaleSideStepRightFile.loadAnimation();

    return {

        // expose the sound assets
        footsteps: _footsteps,

        // expose the animation assets
        femaleStandardWalk: _femaleStandardWalk,
        femaleFlyingUp: _femaleFlyingUp,
        femaleFlying: _femaleFlying,
        femaleFlyingDown: _femaleFlyingDown,
        femaleStandOne: _femaleStandOne,
        femaleSideStepLeft: _femaleSideStepLeft,
        femaleSideStepRight: _femaleSideStepRight,
        maleStandardWalk: _maleStandardWalk,
        maleFlyingUp: _maleFlyingUp,
        maleFlying: _maleFlying,
        maleFlyingDown: _maleFlyingDown,
        maleStandOne: _maleStandOne,
        maleSideStepLeft: _maleSideStepLeft,
        maleSideStepRight: _maleSideStepRight,
    }
})();