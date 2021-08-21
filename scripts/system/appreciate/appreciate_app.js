/*
    Appreciate
    Created by Zach Fox on 2019-01-30
    Copyright 2019 High Fidelity, Inc.

    Distributed under the Apache License, Version 2.0.
    See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
*/

(function () {
    // *************************************
    // START UTILITY FUNCTIONS
    // *************************************
    // #region Utilities
    var MS_PER_S = 1000;
    var CM_PER_M = 100;
    var HALF = 0.5;


    // Returns the first valid joint position from the list of supplied test joint positions.
    // If none are valid, returns MyAvatar.position.
    function getValidJointPosition(jointsToTest) {
        var currentJointIndex;

        for (var i = 0; i < jointsToTest.length; i++) {
            currentJointIndex = MyAvatar.getJointIndex(jointsToTest[i]);

            if (currentJointIndex > -1) {
                return MyAvatar.getJointPosition(jointsToTest[i]);
            }
        }

        return MyAvatar.position;
    } 


    // Returns the world position halfway between the user's hands
    function getAppreciationPosition() {
        var validLeftJoints = ["LeftHandMiddle2", "LeftHand", "LeftArm"];
        var leftPosition = getValidJointPosition(validLeftJoints);

        var validRightJoints = ["RightHandMiddle2", "RightHand", "RightArm"];;
        var rightPosition = getValidJointPosition(validRightJoints);

        var centerPosition = Vec3.sum(leftPosition, rightPosition);
        centerPosition = Vec3.multiply(centerPosition, HALF);

        return centerPosition;
    }


    // Returns a linearly scaled value based on `factor` and the other inputs
    function linearScale(factor, minInput, maxInput, minOutput, maxOutput) {
        return minOutput + (maxOutput - minOutput) *
        (factor - minInput) / (maxInput - minInput);
    }
    

    // Linearly scales an RGB color between 0 and 1 based on RGB color values
    // between 0 and 255.
    function linearScaleColor(intensity, min, max) {
        var output = {
            "red": 0,
            "green": 0,
            "blue": 0
        };

        output.red = linearScale(intensity, 0, 1, min.red, max.red);
        output.green = linearScale(intensity, 0, 1, min.green, max.green);
        output.blue = linearScale(intensity, 0, 1, min.blue, max.blue);

        return output;
    }


    function randomFloat(min, max) {
        return Math.random() * (max - min) + min;
    }


    // Updates the Current Intensity Meter UI element. Called when intensity changes.
    function updateCurrentIntensityUI() {
        ui.sendMessage({method: "updateCurrentIntensityUI", currentIntensity: currentIntensity});
    }
    // #endregion
    // *************************************
    // END UTILITY FUNCTIONS
    // *************************************

    // If the interval that updates the intensity interval exists,
    // clear it.
    var updateIntensityEntityInterval = false;
    var UPDATE_INTENSITY_ENTITY_INTERVAL_MS = 75;
    function maybeClearUpdateIntensityEntityInterval() {
        if (updateIntensityEntityInterval) {
            Script.clearInterval(updateIntensityEntityInterval);
            updateIntensityEntityInterval = false;
        }
        
        if (intensityEntity) {
            Entities.deleteEntity(intensityEntity);
            intensityEntity = false;
        }
    }


    // Determines if any XYZ JSON object has changed "enough" based on
    // last xyz values and current xyz values.
    // Used for determining if angular velocity and dimensions have changed enough.
    var lastAngularVelocity = {
        "x": 0,
        "y": 0,
        "z": 0
    };
    var ANGVEL_DISTANCE_THRESHOLD_PERCENT_CHANGE = 0.35;
    var lastDimensions= {
        "x": 0,
        "y": 0,
        "z": 0
    };
    var DIMENSIONS_DISTANCE_THRESHOLD_PERCENT_CHANGE = 0.2;
    function xyzVecChangedEnough(current, last, thresh) {
        var currentLength = Math.sqrt(
            Math.pow(current.x, TWO) + Math.pow(current.y, TWO) + Math.pow(current.z, TWO));
        var lastLength = Math.sqrt(
            Math.pow(last.x, TWO) + Math.pow(last.y, TWO) + Math.pow(last.z, TWO));

        var change = Math.abs(currentLength - lastLength);
        if (change/lastLength > thresh) {
            return true;
        }

        return false;
    }


    // Determines if color values have changed "enough" based on
    // last color and current color
    var lastColor = {
        "red": 0,
        "blue": 0,
        "green": 0
    };
    var COLOR_DISTANCE_THRESHOLD_PERCENT_CHANGE = 0.35;
    var TWO = 2;
    function colorChangedEnough(current, last, thresh) {
        var currentLength = Math.sqrt(
            Math.pow(current.red, TWO) + Math.pow(current.green, TWO) + Math.pow(current.blue, TWO));
        var lastLength = Math.sqrt(
            Math.pow(last.red, TWO) + Math.pow(last.green, TWO) + Math.pow(last.blue, TWO));

        var change = Math.abs(currentLength - lastLength);
        if (change/lastLength > thresh) {
            return true;
        }

        return false;
    }


    // Updates the intensity entity based on the user's avatar's hand position and the
    // current intensity of their appreciation.
    // Many of these property values are empirically determined.
    var intensityEntity = false;
    var INTENSITY_ENTITY_MAX_DIMENSIONS = {
        "x": 0.24,
        "y": 0.24,
        "z": 0.24
    };
    var INTENSITY_ENTITY_MIN_ANGULAR_VELOCITY = {
        "x": -0.21,
        "y": -0.21,
        "z": -0.21
    };
    var INTENSITY_ENTITY_MAX_ANGULAR_VELOCITY = {
        "x": 0.21,
        "y": 0.21,
        "z": 0.21
    };
    var intensityEntityColorMin = {
        "red": 82,
        "green": 196,
        "blue": 145
    };
    var INTENSITY_ENTITY_COLOR_MAX_DEFAULT = {
        "red": 5,
        "green": 255,
        "blue": 5
    };
    var MIN_COLOR_MULTIPLIER = 0.4;
    var intensityEntityColorMax = JSON.parse(Settings.getValue("appreciate/entityColor",
        JSON.stringify(INTENSITY_ENTITY_COLOR_MAX_DEFAULT)));
    var ANGVEL_ENTITY_MULTIPLY_FACTOR = 62;
    var INTENSITY_ENTITY_NAME = "Appreciation Dodecahedron";
    var INTENSITY_ENTITY_PROPERTIES = {
        "name": INTENSITY_ENTITY_NAME,
        "type": "Shape",
        "shape": "Dodecahedron",
        "dimensions": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "angularVelocity": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "angularDamping": 0,
        "grab": {
            "grabbable": false,
            "equippableLeftRotation": {
                "x": -0.0000152587890625,
                "y": -0.0000152587890625,
                "z": -0.0000152587890625,
                "w": 1
            },
            "equippableRightRotation": {
                "x": -0.0000152587890625,
                "y": -0.0000152587890625,
                "z": -0.0000152587890625,
                "w": 1
            }
        },
        "collisionless": true,
        "ignoreForCollisions": true,
        "queryAACube": {
            "x": -0.17320507764816284,
            "y": -0.17320507764816284,
            "z": -0.17320507764816284,
            "scale": 0.3464101552963257
        },
        "damping": 0,
        "color": intensityEntityColorMin,
        "clientOnly": false,
        "avatarEntity": true,
        "localEntity": false,
        "faceCamera": false,
        "isFacingAvatar": false
    };
    var currentInitialAngularVelocity = {
        "x": 0,
        "y": 0,
        "z": 0
    };
    function updateIntensityEntity() {
        if (!showAppreciationEntity) {
            return;
        }

        if (currentIntensity > 0) {
            if (intensityEntity) {
                intensityEntityColorMin.red = intensityEntityColorMax.red * MIN_COLOR_MULTIPLIER;
                intensityEntityColorMin.green = intensityEntityColorMax.green * MIN_COLOR_MULTIPLIER;
                intensityEntityColorMin.blue = intensityEntityColorMax.blue * MIN_COLOR_MULTIPLIER;

                var color = linearScaleColor(currentIntensity, intensityEntityColorMin, intensityEntityColorMax);

                var propsToUpdate = {
                    position: getAppreciationPosition()
                };

                var currentDimensions = Vec3.multiply(INTENSITY_ENTITY_MAX_DIMENSIONS, currentIntensity);
                if (xyzVecChangedEnough(currentDimensions, lastDimensions, DIMENSIONS_DISTANCE_THRESHOLD_PERCENT_CHANGE)) {
                    propsToUpdate.dimensions = currentDimensions;
                    lastDimensions = currentDimensions;
                }

                var currentAngularVelocity = Vec3.multiply(currentInitialAngularVelocity,
                    currentIntensity * ANGVEL_ENTITY_MULTIPLY_FACTOR);
                if (xyzVecChangedEnough(currentAngularVelocity, lastAngularVelocity, ANGVEL_DISTANCE_THRESHOLD_PERCENT_CHANGE)) {
                    propsToUpdate.angularVelocity = currentAngularVelocity;
                    lastAngularVelocity = currentAngularVelocity;
                }

                var currentColor = color;
                if (colorChangedEnough(currentColor, lastColor, COLOR_DISTANCE_THRESHOLD_PERCENT_CHANGE)) {
                    propsToUpdate.color = currentColor;
                    lastColor = currentColor;
                }

                Entities.editEntity(intensityEntity, propsToUpdate);
            } else {
                var props = INTENSITY_ENTITY_PROPERTIES;
                props.position = getAppreciationPosition();

                currentInitialAngularVelocity.x =
                    randomFloat(INTENSITY_ENTITY_MIN_ANGULAR_VELOCITY.x, INTENSITY_ENTITY_MAX_ANGULAR_VELOCITY.x);
                currentInitialAngularVelocity.y =
                    randomFloat(INTENSITY_ENTITY_MIN_ANGULAR_VELOCITY.y, INTENSITY_ENTITY_MAX_ANGULAR_VELOCITY.y);
                currentInitialAngularVelocity.z =
                    randomFloat(INTENSITY_ENTITY_MIN_ANGULAR_VELOCITY.z, INTENSITY_ENTITY_MAX_ANGULAR_VELOCITY.z);
                props.angularVelocity = currentInitialAngularVelocity;

                intensityEntity = Entities.addEntity(props, "avatar");
            }
        } else {
            if (intensityEntity) {
                Entities.deleteEntity(intensityEntity);
                intensityEntity = false;
            }
            
            maybeClearUpdateIntensityEntityInterval();
        }
    }


    // Function that AppUI calls when the App's UI opens
    function onOpened() {
        updateCurrentIntensityUI();
    }


    // Locally pre-caches all of the sounds in the sounds/claps and sounds/whistles
    // directories.
    var NUM_CLAP_SOUNDS = 16;
    var NUM_WHISTLE_SOUNDS = 17;
    var clapSounds = [];
    var whistleSounds = [];
    function getSounds() {
        for (var i = 1; i < NUM_CLAP_SOUNDS + 1; i++) {
            clapSounds.push(SoundCache.getSound(Script.resolvePath(
                "resources/sounds/claps/" + ("0" + i).slice(-2) + ".wav")));
        }
        for (i = 1; i < NUM_WHISTLE_SOUNDS + 1; i++) {
            whistleSounds.push(SoundCache.getSound(Script.resolvePath(
                "resources/sounds/whistles/" + ("0" + i).slice(-2) + ".wav")));
        }
    }


    // Locally pre-caches the Cheering and Clapping animations
    var whistlingAnimation = false;
    var clappingAnimation = false;
    function getAnimations() {
        var animationURL = Script.resolvePath("resources/animations/Cheering.fbx");
        var resource = AnimationCache.prefetch(animationURL);
        var animation = AnimationCache.getAnimation(animationURL);
        whistlingAnimation = {url: animationURL, animation: animation, resource: resource};

        animationURL = Script.resolvePath("resources/animations/Clapping.fbx");
        resource = AnimationCache.prefetch(animationURL);
        animation = AnimationCache.getAnimation(animationURL);
        clappingAnimation = {url: animationURL, animation: animation, resource: resource};
    }


    // If we're currently fading out the appreciation sounds on an interval,
    // clear that interval.
    function maybeClearSoundFadeInterval() {
        if (soundFadeInterval) {
            Script.clearInterval(soundFadeInterval);
            soundFadeInterval = false;
        }
    }


    // Fade out the appreciation sounds by quickly
    // lowering the global current intensity.
    var soundFadeInterval = false;
    var FADE_INTERVAL_MS = 20;
    var FADE_OUT_STEP_SIZE = 0.05; // Unitless
    function fadeOutAndStopSound() {
        maybeClearSoundFadeInterval();

        soundFadeInterval = Script.setInterval(function() {
            currentIntensity -= FADE_OUT_STEP_SIZE;

            if (currentIntensity <= 0) {
                if (soundInjector) {
                    soundInjector.stop();
                    soundInjector = false;
                }

                updateCurrentIntensityUI();

                maybeClearSoundFadeInterval();
            }

            fadeIntensity(currentIntensity, INTENSITY_MAX_STEP_SIZE_DESKTOP);
        }, FADE_INTERVAL_MS);
    }


    // Calculates the audio injector volume based on 
    // the current global appreciation intensity and some min/max values.
    var MIN_VOLUME_CLAP = 0.05;
    var MAX_VOLUME_CLAP = 1.0;
    var MIN_VOLUME_WHISTLE = 0.07;
    var MAX_VOLUME_WHISTLE = 0.16;
    function calculateInjectorVolume() {
        var minInputVolume = 0;
        var maxInputVolume = MAX_CLAP_INTENSITY;
        var minOutputVolume = MIN_VOLUME_CLAP;
        var maxOutputVolume = MAX_VOLUME_CLAP;

        if (currentSound === "whistle") {
            minInputVolume = MAX_CLAP_INTENSITY;
            maxInputVolume = MAX_WHISTLE_INTENSITY;
            minOutputVolume = MIN_VOLUME_WHISTLE;
            maxOutputVolume = MAX_VOLUME_WHISTLE;
        }

        var vol = linearScale(currentIntensity, minInputVolume,
            maxInputVolume, minOutputVolume, maxOutputVolume);
        return vol;
    }


    // Modifies the global currentIntensity. Moves towards the targetIntensity,
    // but never moves faster than a given max step size per function call.
    // Also clamps the intensity to a min of 0 and a max of 1.0. 
    var currentIntensity = 0;
    var INTENSITY_MAX_STEP_SIZE = 0.003; // Unitless, determined empirically
    var INTENSITY_MAX_STEP_SIZE_DESKTOP = 1; // Unitless, determined empirically
    var MAX_CLAP_INTENSITY = 0.55; // Unitless, determined empirically
    var MAX_WHISTLE_INTENSITY = 1.0; // Unitless, determined empirically
    function fadeIntensity(targetIntensity, maxStepSize) {
        if (!maxStepSize) {
            maxStepSize = INTENSITY_MAX_STEP_SIZE;
        }

        var volumeDelta = targetIntensity - currentIntensity;
        volumeDelta = Math.min(Math.abs(volumeDelta), maxStepSize);

        if (targetIntensity < currentIntensity) {
            volumeDelta *= -1;
        }

        currentIntensity += volumeDelta;

        currentIntensity = Math.max(0.0, Math.min(
            neverWhistleEnabled ? MAX_CLAP_INTENSITY : MAX_WHISTLE_INTENSITY, currentIntensity));

        updateCurrentIntensityUI();

        // Don't adjust volume or position while a sound is playing.
        if (!soundInjector || soundInjector.isPlaying()) {
            return;
        }

        var injectorOptions = {
            position: getAppreciationPosition(),
            volume: calculateInjectorVolume()
        };

        soundInjector.setOptions(injectorOptions);
    }


    // Call this function to actually play a sound.
    // Doesn't play a new sound if a sound is playing AND (you're whistling OR you're in HMD)
    // Injectors are placed between the user's hands (at the same location as the apprecation
    // entity) and are randomly pitched between a MIN and MAX value.
    // Only uses one injector, ever.
    var soundInjector = false;
    var MINIMUM_PITCH = 0.85;
    var MAXIMUM_PITCH = 1.15;
    function playSound(sound) {
        if (soundInjector && soundInjector.isPlaying() && (currentSound === "whistle" || HMD.active)) {
            return;
        }

        if (soundInjector) {
            soundInjector.stop();
            soundInjector = false;
        }

        soundInjector = Audio.playSound(sound, {
            position: getAppreciationPosition(),
            volume: calculateInjectorVolume(),
            pitch: randomFloat(MINIMUM_PITCH, MAXIMUM_PITCH)
        });
    }


    // Returns true if the global intensity and user settings dictate that clapping is the
    // right thing to do.
    function shouldClap() {
        return (currentIntensity > 0.0 && neverWhistleEnabled) ||
            (currentIntensity > 0.0 && currentIntensity <= MAX_CLAP_INTENSITY);
    }


    // Returns true if the global intensity and user settings dictate that whistling is the
    // right thing to do.
    function shouldWhistle() {
        return currentIntensity > MAX_CLAP_INTENSITY &&
            currentIntensity <= MAX_WHISTLE_INTENSITY;
    }


    // Selects the correct sound, then plays it.
    var currentSound;
    function selectAndPlaySound() {
        if (shouldClap()) {
            currentSound = "clap";
            playSound(clapSounds[Math.floor(Math.random() * clapSounds.length)]);
        } else if (shouldWhistle()) {
            currentSound = "whistle";
            playSound(whistleSounds[Math.floor(Math.random() * whistleSounds.length)]);
        }
    }
    

    // If there exists a VR debounce timer (used for not playing sounds too often),
    // clear it.
    function maybeClearVRDebounceTimer() {
        if (vrDebounceTimer) {
            Script.clearTimeout(vrDebounceTimer);
            vrDebounceTimer = false;
        }
    }


    // Calculates the current intensity of appreciation based on the user's
    // hand velocity (rotational and linear).
    // Each type of velocity is weighted differently when determining the final intensity.
    // The VR debounce timer length changes based on current intensity. This forces
    // sounds to play further apart when the user isn't appreciating hard.
    var MAX_VELOCITY_CM_PER_SEC = 110; // determined empirically
    var MAX_ANGULAR_VELOCITY_LENGTH = 1.5; // determined empirically
    var LINEAR_VELOCITY_WEIGHT = 0.7; // This and the line below must add up to 1.0
    var ANGULAR_VELOCITY_LENGTH_WEIGHT = 0.3; // This and the line below must add up to 1.0
    var vrDebounceTimer = false;
    var VR_DEBOUNCE_TIMER_TIMEOUT_MIN_MS = 20; // determined empirically
    var VR_DEBOUNCE_TIMER_TIMEOUT_MAX_MS = 200; // determined empirically
    function calculateHandEffect(linearVelocity, angularVelocity){
        var leftHandLinearVelocityCMPerSec = linearVelocity.left;
        var rightHandLinearVelocityCMPerSec = linearVelocity.right;
        var averageLinearVelocity = (leftHandLinearVelocityCMPerSec + rightHandLinearVelocityCMPerSec) / 2;
        averageLinearVelocity = Math.min(averageLinearVelocity, MAX_VELOCITY_CM_PER_SEC);

        var leftHandAngularVelocityLength = Vec3.length(angularVelocity.left);
        var rightHandAngularVelocityLength = Vec3.length(angularVelocity.right);
        var averageAngularVelocityIntensity = (leftHandAngularVelocityLength + rightHandAngularVelocityLength) / 2;
        averageAngularVelocityIntensity = Math.min(averageAngularVelocityIntensity, MAX_ANGULAR_VELOCITY_LENGTH);

        var appreciationIntensity =
            averageLinearVelocity / MAX_VELOCITY_CM_PER_SEC * LINEAR_VELOCITY_WEIGHT +
            averageAngularVelocityIntensity / MAX_ANGULAR_VELOCITY_LENGTH * ANGULAR_VELOCITY_LENGTH_WEIGHT;

        fadeIntensity(appreciationIntensity);
        
        var vrDebounceTimeout = VR_DEBOUNCE_TIMER_TIMEOUT_MIN_MS +
            (VR_DEBOUNCE_TIMER_TIMEOUT_MAX_MS - VR_DEBOUNCE_TIMER_TIMEOUT_MIN_MS) * (1.0 - appreciationIntensity);
        // This timer forces a minimum tail duration for all sound clips
        if (!vrDebounceTimer) {
            selectAndPlaySound();
            vrDebounceTimer = Script.setTimeout(function() {
                vrDebounceTimer = false;
            }, vrDebounceTimeout);
        }
    }


    // Gets both hands' linear velocity.
    var lastLeftHandPosition = false;
    var lastRightHandPosition = false;
    function getHandsLinearVelocity() {
        var linearVelocity = {
            left: 0,
            right: 0
        };

        var leftHandPosition = MyAvatar.getJointPosition("LeftHand");
        var rightHandPosition = MyAvatar.getJointPosition("RightHand");

        if (!lastLeftHandPosition || !lastRightHandPosition) {
            lastLeftHandPosition = leftHandPosition;
            lastRightHandPosition = rightHandPosition;
            return linearVelocity;
        }

        var leftHandDistanceCM = Vec3.distance(leftHandPosition, lastLeftHandPosition) * CM_PER_M;
        var rightHandDistanceCM = Vec3.distance(rightHandPosition, lastRightHandPosition) * CM_PER_M;

        linearVelocity.left = leftHandDistanceCM / HAND_VELOCITY_CHECK_INTERVAL_MS * MS_PER_S;
        linearVelocity.right = rightHandDistanceCM / HAND_VELOCITY_CHECK_INTERVAL_MS * MS_PER_S;
        
        lastLeftHandPosition = leftHandPosition;
        lastRightHandPosition = rightHandPosition;

        return linearVelocity;
    }


    // Gets both hands' angular velocity.
    var lastLeftHandRotation = false;
    var lastRightHandRotation = false;
    function getHandsAngularVelocity() {
        var angularVelocity = {
            left: {x: 0, y: 0, z: 0},
            right: {x: 0, y: 0, z: 0}
        };

        var leftHandRotation = MyAvatar.getJointRotation(MyAvatar.getJointIndex("LeftHand"));
        var rightHandRotation = MyAvatar.getJointRotation(MyAvatar.getJointIndex("RightHand"));

        if (!lastLeftHandRotation || !lastRightHandRotation) {
            lastLeftHandRotation = leftHandRotation;
            lastRightHandRotation = rightHandRotation;
            return angularVelocity;
        }

        var leftHandAngleDelta = Quat.multiply(leftHandRotation, Quat.inverse(lastLeftHandRotation)); 
        var rightHandAngleDelta = Quat.multiply(rightHandRotation, Quat.inverse(lastRightHandRotation));

        leftHandAngleDelta = Quat.safeEulerAngles(leftHandAngleDelta);
        rightHandAngleDelta = Quat.safeEulerAngles(rightHandAngleDelta);

        angularVelocity.left = Vec3.multiply(leftHandAngleDelta, 1 / HAND_VELOCITY_CHECK_INTERVAL_MS);
        angularVelocity.right = Vec3.multiply(rightHandAngleDelta, 1 / HAND_VELOCITY_CHECK_INTERVAL_MS);

        lastLeftHandRotation = leftHandRotation;
        lastRightHandRotation = rightHandRotation;

        return angularVelocity;
    }


    // Calculates the hand effect (see above). Gets called on an interval,
    // but only if the user's hands are above their head. This saves processing power.
    // Also sets up the `updateIntensityEntity` interval.
    function handVelocityCheck() {
        if (!handsAreAboveHead) {
            return;
        }

        var handsLinearVelocity = getHandsLinearVelocity();
        var handsAngularVelocity = getHandsAngularVelocity();

        calculateHandEffect(handsLinearVelocity, handsAngularVelocity);

        if (!updateIntensityEntityInterval && showAppreciationEntity) {
            updateIntensityEntityInterval = Script.setInterval(updateIntensityEntity, UPDATE_INTENSITY_ENTITY_INTERVAL_MS);
        }
    }


    // If handVelocityCheckInterval is set up, clear it.
    function maybeClearHandVelocityCheck() {
        if (handVelocityCheckInterval) {
            Script.clearInterval(handVelocityCheckInterval);
            handVelocityCheckInterval = false;
        }
    }


    // If handVelocityCheckInterval is set up, clear it.
    // Also stop the sound injector and set currentIntensity to 0.
    function maybeClearHandVelocityCheckIntervalAndStopSound() {
        maybeClearHandVelocityCheck();

        if (soundInjector) {
            soundInjector.stop();
            soundInjector = false;
        }
        
        currentIntensity = 0.0;
    }


    // Sets up an interval that'll check the avatar's hand's velocities.
    // This is used for calculating the effect.
    // If the user isn't in HMD, we'll never set up this interval.
    var handVelocityCheckInterval = false;
    var HAND_VELOCITY_CHECK_INTERVAL_MS = 10;
    function maybeSetupHandVelocityCheckInterval() {
        // `!HMD.active` clause isn't really necessary, just extra protection
        if (handVelocityCheckInterval || !HMD.active) {
            return;
        }

        handVelocityCheckInterval = Script.setInterval(handVelocityCheck, HAND_VELOCITY_CHECK_INTERVAL_MS);
    }


    // Checks the position of the user's hands to determine if they're above their head.
    // If they are, sets up the hand velocity check interval (see above).
    // If they aren't, clears that interval and stops the apprecation sound.
    var handsAreAboveHead = false;
    function handPositionCheck() {
        var leftHandPosition = MyAvatar.getJointPosition("LeftHand");
        var rightHandPosition = MyAvatar.getJointPosition("RightHand");
        var headJointPosition = MyAvatar.getJointPosition("Head");

        var headY = headJointPosition.y;

        handsAreAboveHead = (rightHandPosition.y > headY && leftHandPosition.y > headY);

        if (handsAreAboveHead) {
            maybeSetupHandVelocityCheckInterval();
        } else {
            maybeClearHandVelocityCheck();
            fadeOutAndStopSound();
        }
    }


    // If handPositionCheckInterval is set up, clear it.
    function maybeClearHandPositionCheckInterval() {
        if (handPositionCheckInterval) {
            Script.clearInterval(handPositionCheckInterval);
            handPositionCheckInterval = false;
        }
    }


    // If the app is enabled, sets up an interval that'll check if the avatar's hands are above their head.
    var handPositionCheckInterval = false;
    var HAND_POSITION_CHECK_INTERVAL_MS = 200;
    function maybeSetupHandPositionCheckInterval() {
        if (!appreciateEnabled || !HMD.active) {
            return;
        }

        maybeClearHandPositionCheckInterval();

        handPositionCheckInterval = Script.setInterval(handPositionCheck, HAND_POSITION_CHECK_INTERVAL_MS);
    }


    // If the interval that periodically lowers the apprecation volume is set up, clear it.
    function maybeClearSlowAppreciationInterval() {
        if (slowAppreciationInterval) {
            Script.clearInterval(slowAppreciationInterval);
            slowAppreciationInterval = false;
        }
    }


    // Stop appreciating. Called when Appreciating from Desktop mode.
    function stopAppreciating() {
        maybeClearStopAppreciatingTimeout();
        maybeClearSlowAppreciationInterval();
        maybeClearUpdateIntensityEntityInterval();
        MyAvatar.restoreAnimation();
        currentAnimationFPS = INITIAL_ANIMATION_FPS;
        currentlyPlayingFrame = 0;
        currentAnimationTimestamp = 0;
    }


    // If the timeout that stops the user's apprecation is set up, clear it.
    function maybeClearStopAppreciatingTimeout() {
        if (stopAppreciatingTimeout) {
            Script.clearTimeout(stopAppreciatingTimeout);
            stopAppreciatingTimeout = false;
        }
    }


    function calculateCurrentAnimationFPS(frameCount) {
        var animationTimestampDeltaMS = Date.now() - currentAnimationTimestamp;
        var frameDelta = animationTimestampDeltaMS / MS_PER_S * currentAnimationFPS;

        currentlyPlayingFrame = (currentlyPlayingFrame + frameDelta) % frameCount;

        currentAnimationFPS = currentIntensity * CHEERING_FPS_MAX + INITIAL_ANIMATION_FPS;

        currentAnimationFPS = Math.min(currentAnimationFPS, CHEERING_FPS_MAX);

        if (currentAnimation === clappingAnimation) {
            currentAnimationFPS += CLAP_ANIMATION_FPS_BOOST;
        }
    }


    // Called on an interval. Slows down the user's appreciation!
    var VOLUME_STEP_DOWN_DESKTOP = 0.01; // Unitless, determined empirically
    function slowAppreciation() {
        currentIntensity -= VOLUME_STEP_DOWN_DESKTOP;
        fadeIntensity(currentIntensity, INTENSITY_MAX_STEP_SIZE_DESKTOP);

        currentAnimation = selectAnimation();

        if (!currentAnimation) {
            stopAppreciating();
            return;
        }

        var frameCount = currentAnimation.animation.frames.length;

        calculateCurrentAnimationFPS(frameCount);

        MyAvatar.overrideAnimation(currentAnimation.url, currentAnimationFPS, true, currentlyPlayingFrame, frameCount);

        currentAnimationTimestamp = Date.now();
    }


    // Selects the proper animation to use when Appreciating in Desktop mode.
    function selectAnimation() {
        if (shouldClap()) {
            if (currentAnimation === whistlingAnimation) {
                currentAnimationTimestamp = 0;
            }
            return clappingAnimation;
        } else if (shouldWhistle()) {
            if (currentAnimation === clappingAnimation) {
                currentAnimationTimestamp = 0;
            }
            return whistlingAnimation;
        } else {
            return false;
        }
    }


    // Called when the Z key is pressed (and some other conditions are met).
    // 1. (Maybe) clears old intervals
    // 2. Steps up the global currentIntensity, then forces the effect/sound to fade/play immediately
    // 3. Selects an animation to play based on various factors, then plays it
    //     - Stops appreciating if the selected animation is falsey
    // 4. Sets up the "Slow Appreciation" interval which slows appreciation over time
    // 5. Modifies the avatar's animation based on the current appreciation intensity
    //     - Since there's no way to modify animation FPS on-the-fly, we have to calculate
    //         where the animation should start based on where it was before changing FPS
    // 6. Sets up the `updateIntensityEntity` interval if one isn't already setup
    var INITIAL_ANIMATION_FPS = 7;
    var SLOW_APPRECIATION_INTERVAL_MS = 100;
    var CHEERING_FPS_MAX = 80;
    var VOLUME_STEP_UP_DESKTOP = 0.035; // Unitless, determined empirically
    var CLAP_ANIMATION_FPS_BOOST = 15;
    var currentAnimation = false;
    var currentAnimationFPS = INITIAL_ANIMATION_FPS;
    var slowAppreciationInterval = false;
    var currentlyPlayingFrame = 0;
    var currentAnimationTimestamp;
    function keyPressed() {
        // Don't do anything if the animations aren't cached.
        if (!whistlingAnimation || !clappingAnimation) {
            return;
        }

        maybeClearSoundFadeInterval();
        maybeClearStopAppreciatingTimeout();

        currentIntensity += VOLUME_STEP_UP_DESKTOP;
        fadeIntensity(currentIntensity, INTENSITY_MAX_STEP_SIZE_DESKTOP);
        selectAndPlaySound();

        currentAnimation = selectAnimation();

        if (!currentAnimation) {
            stopAppreciating();
            return;
        }

        if (!slowAppreciationInterval) {
            slowAppreciationInterval = Script.setInterval(slowAppreciation, SLOW_APPRECIATION_INTERVAL_MS);
        }

        var frameCount = currentAnimation.animation.frames.length;

        if (currentAnimationTimestamp > 0) {
            calculateCurrentAnimationFPS(frameCount);
        } else {
            currentlyPlayingFrame = 0;
        }

        MyAvatar.overrideAnimation(currentAnimation.url, currentAnimationFPS, true, currentlyPlayingFrame, frameCount);
        currentAnimationTimestamp = Date.now();
        
        if (!updateIntensityEntityInterval && showAppreciationEntity) {
            updateIntensityEntityInterval = Script.setInterval(updateIntensityEntity, UPDATE_INTENSITY_ENTITY_INTERVAL_MS);
        }
    }

    
    // The listener for all in-app keypresses. Listens for an unshifted, un-alted, un-ctrl'd
    // "Z" keypress. Only listens when in Desktop mode. If the user is holding the key down,
    // we make sure not to call the `keyPressed()` handler too often using the `desktopDebounceTimer`.
    var desktopDebounceTimer = false;
    var DESKTOP_DEBOUNCE_TIMEOUT_MS = 160;
    function keyPressEvent(event) {
        if (!appreciateEnabled) {
            return;
        }

        if ((event.text.toUpperCase() === "Z") &&
            !event.isShifted &&
            !event.isMeta &&
            !event.isControl &&
            !event.isAlt &&
            !HMD.active) {

            if (event.isAutoRepeat) {
                if (!desktopDebounceTimer) {
                    keyPressed();

                    desktopDebounceTimer = Script.setTimeout(function() {
                        desktopDebounceTimer = false;
                    }, DESKTOP_DEBOUNCE_TIMEOUT_MS);
                }
            } else {
                keyPressed();
            }
        }
    }

    
    // Sets up a timeout that will fade out the appreciation sound, then stop it.
    var stopAppreciatingTimeout = false;
    var STOP_APPRECIATING_TIMEOUT_MS = 1000;
    function stopAppreciatingSoon() {
        maybeClearStopAppreciatingTimeout();

        if (currentIntensity > 0) {
            stopAppreciatingTimeout = Script.setTimeout(fadeOutAndStopSound, STOP_APPRECIATING_TIMEOUT_MS);
        }
    }

    
    // When the "Z" key is released, we want to stop appreciating a short time later.
    function keyReleaseEvent(event) {
        if (!appreciateEnabled) {
            return;
        }

        if ((event.text.toUpperCase() === "Z") &&
            !event.isAutoRepeat) {
            stopAppreciatingSoon();
        }
    }


    // Enables or disables the app's main functionality
    var appreciateEnabled = Settings.getValue("appreciate/enabled", false);
    var neverWhistleEnabled = Settings.getValue("appreciate/neverWhistle", false);
    var showAppreciationEntity = Settings.getValue("appreciate/showAppreciationEntity", true);
    var keyEventsWired = false;
    function enableOrDisableAppreciate() {
        maybeClearHandPositionCheckInterval();
        maybeClearHandVelocityCheckIntervalAndStopSound();

        if (appreciateEnabled) {
            maybeSetupHandPositionCheckInterval();
            
            if (!keyEventsWired && !HMD.active) {
                Controller.keyPressEvent.connect(keyPressEvent);
                Controller.keyReleaseEvent.connect(keyReleaseEvent);
                keyEventsWired = true;
            }
        } else {
            stopAppreciating();

            if (keyEventsWired) {
                Controller.keyPressEvent.disconnect(keyPressEvent);
                Controller.keyReleaseEvent.disconnect(keyReleaseEvent);
                keyEventsWired = false;
            }
        }
    }


    // Handles incoming messages from the UI
    // - "eventBridgeReady" - The App's UI will send this when it's ready to
    //     receive events over the Event Bridge
    // - "appreciateSwitchClicked" - The App's UI will send this when the user
    //     clicks the main toggle switch in the top right of the app
    // - "neverWhistleCheckboxClicked" - Sent when the user clicks the
    //     "Never Whistle" checkbox
    // - "setEntityColor" - Sent when the user chooses a new Entity Color.
    function onMessage(message) {
        if (message.app !== "appreciate") {
            return;
        }

        switch (message.method) {
            case "eventBridgeReady":
                ui.sendMessage({
                    method: "updateUI",
                    appreciateEnabled: appreciateEnabled,
                    neverWhistleEnabled: neverWhistleEnabled,
                    showAppreciationEntity: showAppreciationEntity,
                    isFirstRun: Settings.getValue("appreciate/firstRun", true),
                    entityColor: intensityEntityColorMax
                });
                break;

            case "appreciateSwitchClicked":
                Settings.setValue("appreciate/firstRun", false);
                appreciateEnabled = message.appreciateEnabled;
                Settings.setValue("appreciate/enabled", appreciateEnabled);
                enableOrDisableAppreciate();
                break;

            case "neverWhistleCheckboxClicked":
                neverWhistleEnabled = message.neverWhistle;
                Settings.setValue("appreciate/neverWhistle", neverWhistleEnabled);
                break;

            case "showAppreciationEntityCheckboxClicked":
                showAppreciationEntity = message.showAppreciationEntity;
                Settings.setValue("appreciate/showAppreciationEntity", showAppreciationEntity);
                break;

            case "setEntityColor":
                intensityEntityColorMax = message.entityColor;
                Settings.setValue("appreciate/entityColor", JSON.stringify(intensityEntityColorMax));
                break;

            case "zKeyDown":
                var pressEvent = {
                    "text": "Z",
                    "isShifted": false,
                    "isMeta": false,
                    "isControl": false,
                    "isAlt": false,
                    "isAutoRepeat": message.repeat
                };
                keyPressEvent(pressEvent);
                break;

            case "zKeyUp":
                var releaseEvent = {
                    "text": "Z",
                    "isShifted": false,
                    "isMeta": false,
                    "isControl": false,
                    "isAlt": false,
                    "isAutoRepeat": false
                };
                keyReleaseEvent(releaseEvent);
                break;

            default:
                console.log("Unhandled message from appreciate_ui.js: " + JSON.stringify(message));
                break;
        }
    }


    // Searches through all of your avatar entities and deletes any with the name
    // that equals the one set when rezzing the Intensity Entity
    function cleanupOldIntensityEntities() {
        MyAvatar.getAvatarEntitiesVariant().forEach(function(avatarEntity) {
            var name = Entities.getEntityProperties(avatarEntity.id, 'name').name;
            if (name === INTENSITY_ENTITY_NAME && avatarEntity.id !== intensityEntity) {
                Entities.deleteEntity(avatarEntity.id);
            }
        });
    }

    
    // Called when the script is stopped. STOP ALL THE THINGS!
    function onScriptEnding() {
        maybeClearHandPositionCheckInterval();
        maybeClearHandVelocityCheckIntervalAndStopSound();
        maybeClearSoundFadeInterval();
        maybeClearVRDebounceTimer();
        maybeClearUpdateIntensityEntityInterval();
        cleanupOldIntensityEntities();

        maybeClearStopAppreciatingTimeout();
        stopAppreciating();

        if (desktopDebounceTimer) {
            Script.clearTimeout(desktopDebounceTimer);
            desktopDebounceTimer = false;
        }

        if (keyEventsWired) {
            Controller.keyPressEvent.disconnect(keyPressEvent);
            Controller.keyReleaseEvent.disconnect(keyReleaseEvent);
            keyEventsWired = false;
        }
        
        if (intensityEntity) {
            Entities.deleteEntity(intensityEntity);
            intensityEntity = false;
        }

        HMD.displayModeChanged.disconnect(enableOrDisableAppreciate);
    }


    // When called, this function will stop the versions of this script that are
    // baked into the client installation IF there's another version of the script
    // running that ISN'T the baked version.
    function maybeStopBakedScriptVersions() {
        var THIS_SCRIPT_FILENAME = "appreciate_app.js";
        var RELATIVE_PATH_TO_BAKED_SCRIPT = "system/experiences/appreciate/appResources/appData/" + THIS_SCRIPT_FILENAME;
        var bakedLocalScriptPaths = [];
        var alsoRunningNonBakedVersion = false;

        var runningScripts = ScriptDiscoveryService.getRunning();
        runningScripts.forEach(function(scriptObject) {
            if (scriptObject.local && scriptObject.url.indexOf(RELATIVE_PATH_TO_BAKED_SCRIPT) > -1) {
                bakedLocalScriptPaths.push(scriptObject.path);
            }

            if (scriptObject.name === THIS_SCRIPT_FILENAME && scriptObject.url.indexOf(RELATIVE_PATH_TO_BAKED_SCRIPT) === -1) {
                alsoRunningNonBakedVersion = true;
            }
        });

        if (alsoRunningNonBakedVersion && bakedLocalScriptPaths.length > 0) {
            for (var i = 0; i < bakedLocalScriptPaths.length; i++) {
                ScriptDiscoveryService.stopScript(bakedLocalScriptPaths[i]);
            }
        }
    }


    // Called when the script starts up
    var BUTTON_NAME = "APPRECIATE";
    var APP_UI_URL = Script.resolvePath('resources/appreciate_ui.html');
    var CLEANUP_INTENSITY_ENTITIES_STARTUP_DELAY_MS = 5000;
    var AppUI = Script.require('appUi');
    var ui;
    function startup() {
        ui = new AppUI({
            buttonName: BUTTON_NAME,
            home: APP_UI_URL,
            // clap by Rena from the Noun Project
            graphicsDirectory: Script.resolvePath("./resources/images/icons/"),
            onOpened: onOpened,
            onMessage: onMessage
        });
        
        cleanupOldIntensityEntities();
        // We need this because sometimes avatar entities load after this script does.
        Script.setTimeout(cleanupOldIntensityEntities, CLEANUP_INTENSITY_ENTITIES_STARTUP_DELAY_MS);
        enableOrDisableAppreciate();
        getSounds();
        getAnimations();
        HMD.displayModeChanged.connect(enableOrDisableAppreciate);
        maybeStopBakedScriptVersions();
    }


    Script.scriptEnding.connect(onScriptEnding);
    startup();
})();

