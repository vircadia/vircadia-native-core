"use strict";
/*jslint vars:true, plusplus:true, forin:true*/
/*global Window, Script, Controller, MyAvatar, AvatarList, Entities, Messages, Audio, SoundCache, Account, UserActivityLogger, Vec3, Quat, XMLHttpRequest, location, print*/
//
//  makeUserConnection.js
//  scripts/system
//
//  Created by David Kelly on 3/7/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () { // BEGIN LOCAL_SCOPE

    var request = Script.require('request').request;

    var WANT_DEBUG = Settings.getValue('MAKE_USER_CONNECTION_DEBUG', false);
    var LABEL = "makeUserConnection";
    var MAX_AVATAR_DISTANCE = 0.2; // m
    var GRIP_MIN = 0.75; // goes from 0-1, so 75% pressed is pressed
    var MESSAGE_CHANNEL = "io.highfidelity.makeUserConnection";
    var STATES = {
        INACTIVE: 0,
        WAITING: 1,
        CONNECTING: 2,
        MAKING_CONNECTION: 3
    };
    var STATE_STRINGS = ["inactive", "waiting", "connecting", "makingConnection"];
    var HAND_STRING_PROPERTY = 'hand'; // Used in our message protocol. IWBNI we changed it to handString, but that would break compatability.
    var WAITING_INTERVAL = 100; // ms
    var CONNECTING_INTERVAL = 100; // ms
    var MAKING_CONNECTION_TIMEOUT = 800; // ms
    var CONNECTING_TIME = 100; // ms One interval.
    var PARTICLE_RADIUS = 0.15; // m
    var PARTICLE_ANGLE_INCREMENT = 360 / 45; // 1hz
    var HANDSHAKE_SOUND_URL = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/davidkelly/production/audio/4beat_sweep.wav");
    var SUCCESSFUL_HANDSHAKE_SOUND_URL = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/davidkelly/production/audio/3rdbeat_success_bell.wav");
    var PREFERRER_HAND_JOINT_POSTFIX_ORDER = ['Middle1', 'Index1', ''];
    var HAPTIC_DATA = {
        initial: { duration: 20, strength: 0.6 }, // duration is in ms
        background: { duration: 100, strength: 0.3 }, // duration is in ms
        success: { duration: 60, strength: 1.0 } // duration is in ms
    };
    var PARTICLE_EFFECT_PROPS = {
        "alpha": 0.8,
        "azimuthFinish": Math.PI,
        "azimuthStart": -Math.PI,
        "emitRate": 500,
        "emitterShouldTrail": 1,
        "isEmitting": 1,
        "lifespan": 3,
        "lifetime": 5,
        "maxParticles": 1000,
        "particleRadius": 0.003,
        "polarStart": Math.PI / 2,
        "polarFinish": Math.PI / 2,
        "radiusFinish": 0.008,
        "radiusStart": 0.0025,
        "emitSpeed": 0.02,
        "speedSpread": 0.015,
        "textures": Script.getExternalPath(Script.ExternalPaths.HF_Content, "/alan/dev/Particles/Bokeh-Particle.png"),
        "color": {"red": 255, "green": 255, "blue": 255},
        "colorFinish": {"red": 0, "green": 164, "blue": 255},
        "colorStart": {"red": 255, "green": 255, "blue": 255},
        "emitOrientation": {"w": -0.71, "x": 0.0, "y": 0.0, "z": 0.71},
        "emitAcceleration": {"x": 0.0, "y": 0.0, "z": 0.0},
        "emitDimensions": { "x": 0.15, "y": 0.15, "z": 0.01 },
        "accelerationSpread": {"x": 0.0, "y": 0.0, "z": 0.0},
        "dimensions": {"x": 0.05, "y": 0.05, "z": 0.05},
        "type": "ParticleEffect"
    };
    var MAKING_CONNECTION_PARTICLE_PROPS = {
        "alpha": 0.07,
        "alphaStart": 0.011,
        "alphaSpread": 0,
        "alphaFinish": 0,
        "azimuthFinish": Math.PI,
        "azimuthStart": -1 * Math.PI,
        "emitRate": 2000,
        "emitSpeed": 0.0,
        "emitterShouldTrail": 1,
        "isEmitting": 1,
        "lifespan": 3.6,
        "lifetime": 5,
        "maxParticles": 4000,
        "particleRadius": 0.048,
        "polarStart": 0,
        "polarFinish": 1,
        "radiusFinish": 0.3,
        "radiusStart": 0.04,
        "speedSpread": 0.00,
        "radiusSpread": 0.0,
        "textures": Script.getExternalPath(Script.ExternalPaths.HF_Content, "/alan/dev/Particles/Bokeh-Particle.png"),
        "color": {"red": 200, "green": 170, "blue": 255},
        "colorFinish": {"red": 0, "green": 134, "blue": 255},
        "colorStart": {"red": 185, "green": 222, "blue": 255},
        "emitOrientation": {"w": -0.71, "x": 0.0, "y": 0.0, "z": 0.71},
        "emitAcceleration": {"x": 0.0, "y": 0.0, "z": 0.0},
        "accelerationSpread": {"x": 0.0, "y": 0.0, "z": 0.0},
        "dimensions": {"x": 0.05, "y": 0.05, "z": 0.05},
        "type": "ParticleEffect"
    };

    var currentHand;
    var currentHandJointIndex = -1;
    var state = STATES.INACTIVE;
    var connectingInterval;
    var waitingInterval;
    var makingConnectionTimeout;
    var animHandlerId;
    var connectingId;
    var connectingHandJointIndex = -1;
    var waitingList = {};
    var particleEffect;
    var particleEmitRate;
    var PARTICLE_INITIAL_EMIT_RATE = 250;
    var PARTICLE_MINIMUM_EMIT_RATE = 50;
    var PARTICLE_DECAY_RATE = 0.5;
    var particleEffectUpdateTimer = null;
    var PARTICLE_EFFECT_UPDATE_INTERVAL = 200;
    var makingConnectionParticleEffect;
    var makingConnectionEmitRate;
    var isMakingConnectionEmitting;
    var MAKING_CONNECTION_INITIAL_EMIT_RATE = 500;
    var MAKING_CONNECTION_MINIMUM_EMIT_RATE = 20;
    var MAKING_CONNECTION_DECAY_RATE = 0.5;
    var makingConnectionUpdateTimer = null;
    var MAKING_CONNECTION_UPDATE_INTERVAL = 200;
    var handshakeInjector;
    var successfulHandshakeInjector;
    var handshakeSound;
    var successfulHandshakeSound;

    function debug() {
        if (!WANT_DEBUG) {
            return;
        }
        var stateString = "<" + STATE_STRINGS[state] + ">";
        var connecting = "[" + connectingId + "/" + connectingHandJointIndex + "]";
        var current = "[" + currentHand + "/" + currentHandJointIndex + "]"
        print.apply(null, [].concat.apply([LABEL, stateString, current, JSON.stringify(waitingList), connecting],
            [].map.call(arguments, JSON.stringify)));
    }

    function cleanId(guidWithCurlyBraces) {
        return guidWithCurlyBraces.slice(1, -1);
    }

    function handToString(hand) {
        if (hand === Controller.Standard.RightHand) {
            return "RightHand";
        }
        if (hand === Controller.Standard.LeftHand) {
            return "LeftHand";
        }
        debug("handToString called without valid hand! value: ", hand);
        return "";
    }

    function handToHaptic(hand) {
        if (hand === Controller.Standard.RightHand) {
            return 1;
        }
        if (hand === Controller.Standard.LeftHand) {
            return 0;
        }
        debug("handToHaptic called without a valid hand!");
        return -1;
    }

    function stopWaiting() {
        if (waitingInterval) {
            waitingInterval = Script.clearInterval(waitingInterval);
        }
    }

    function stopConnecting() {
        if (connectingInterval) {
            connectingInterval = Script.clearInterval(connectingInterval);
        }
    }

    function stopMakingConnection() {
        if (makingConnectionTimeout) {
            makingConnectionTimeout = Script.clearTimeout(makingConnectionTimeout);
        }
    }

    // This returns the ideal hand joint index for the avatar.
    // [handString]middle1 -> [handString]index1 -> [handString]
    function getIdealHandJointIndex(avatar, handString) {
        debug("get hand " + handString + " for avatar " + (avatar && avatar.sessionUUID));
        var suffixIndex, jointName, jointIndex;
        for (suffixIndex = 0; suffixIndex < (avatar ? PREFERRER_HAND_JOINT_POSTFIX_ORDER.length : 0); suffixIndex++) {
            jointName = handString + PREFERRER_HAND_JOINT_POSTFIX_ORDER[suffixIndex];
            jointIndex = avatar.getJointIndex(jointName);
            if (jointIndex !== -1) {
                debug('found joint ' + jointName + ' (' + jointIndex + ')');
                return jointIndex;
            }
        }
        debug('no hand joint found.');
        return -1;
    }

    // This returns the preferred hand position.
    function getHandPosition(avatar, handJointIndex) {
        if (handJointIndex === -1) {
            debug("calling getHandPosition with no hand joint index! (returning avatar position but this is a BUG)");
            return avatar.position;
        }
        return avatar.getJointPosition(handJointIndex);
    }

    var animationData = {};
    function updateAnimationData(verticalOffset) {
        // all we are doing here is moving the right hand to a spot
        // that is in front of and a bit above the hips.  Basing how
        // far in front as scaling with the avatar's height (say hips
        // to head distance)
        var headIndex = MyAvatar.getJointIndex("Head");
        var offset = 0.5; // default distance of hand in front of you
        if (headIndex) {
            offset = 0.8 * MyAvatar.getAbsoluteJointTranslationInObjectFrame(headIndex).y;
        }
        animationData.rightHandPosition = Vec3.multiply(offset, {x: -0.25, y: 0.8, z: 1.3});
        if (verticalOffset) {
            animationData.rightHandPosition.y += verticalOffset;
        }
        animationData.rightHandRotation = Quat.fromPitchYawRollDegrees(90, 0, 90);
        animationData.rightHandType = 0; // RotationAndPosition, see IKTargets.h

        // turn on the right hand grip overlay
        animationData.rightHandOverlayAlpha = 1.0;

        // make sure the right hand grip animation is the "grasp", not pointing or thumbs up.
        animationData.isRightHandGrasp = true;
        animationData.isRightIndexPoint = false;
        animationData.isRightThumbRaise = false;
        animationData.isRightIndexPointAndThumbRaise = false;
    }
    function shakeHandsAnimation() {
        return animationData;
    }
    function endHandshakeAnimation() {
        if (animHandlerId) {
            debug("removing animation");
            animHandlerId = MyAvatar.removeAnimationStateHandler(animHandlerId);
        }
    }
    function startHandshakeAnimation() {
        endHandshakeAnimation(); // just in case order of press/unpress is broken
        debug("adding animation");
        updateAnimationData();
        animHandlerId = MyAvatar.addAnimationStateHandler(shakeHandsAnimation, []);
    }

    function positionFractionallyTowards(posA, posB, frac) {
        return Vec3.sum(posA, Vec3.multiply(frac, Vec3.subtract(posB, posA)));
    }

    function deleteParticleEffect() {
        if (particleEffectUpdateTimer) {
            Script.clearTimeout(particleEffectUpdateTimer);
            particleEffectUpdateTimer = null;
        }
        if (particleEffect) {
            particleEffect = Entities.deleteEntity(particleEffect);
        }
    }

    function deleteMakeConnectionParticleEffect() {
        if (makingConnectionUpdateTimer) {
            Script.clearTimeout(makingConnectionUpdateTimer);
            makingConnectionUpdateTimer = null;
        }
        if (makingConnectionParticleEffect) {
            makingConnectionParticleEffect = Entities.deleteEntity(makingConnectionParticleEffect);
        }
    }

    function stopHandshakeSound() {
        if (handshakeInjector) {
            handshakeInjector.stop();
            handshakeInjector = null;
        }
    }

    function updateMakingConnection() {
        if (!makingConnectionParticleEffect) {
            particleEffectUpdateTimer = null;
            return;
        }

        makingConnectionEmitRate = Math.max(makingConnectionEmitRate * MAKING_CONNECTION_DECAY_RATE,
            MAKING_CONNECTION_MINIMUM_EMIT_RATE);
        isMakingConnectionEmitting = true;
        Entities.editEntity(makingConnectionParticleEffect, {
            emitRate: makingConnectionEmitRate,
            isEmitting: true
        });
        if (makingConnectionEmitRate > MAKING_CONNECTION_MINIMUM_EMIT_RATE) {
            makingConnectionUpdateTimer = Script.setTimeout(makingConnectionUpdateTimer, MAKING_CONNECTION_UPDATE_INTERVAL);
        } else {
            makingConnectionUpdateTimer = null;
        }
    }

    function updateParticleEffect() {
        if (!particleEffect) {
            particleEffectUpdateTimer = null;
            return;
        }

        particleEmitRate = Math.max(PARTICLE_MINIMUM_EMIT_RATE, particleEmitRate * PARTICLE_DECAY_RATE);
        Entities.editEntity(particleEffect, {
            emitRate: particleEmitRate
        });
        if (particleEmitRate > PARTICLE_MINIMUM_EMIT_RATE) {
            particleEffectUpdateTimer = Script.setTimeout(updateParticleEffect, PARTICLE_EFFECT_UPDATE_INTERVAL);
        } else {
            particleEffectUpdateTimer = null;
        }
    }

    // this is called frequently, but usually does nothing
    function updateVisualization() {
        if (state === STATES.INACTIVE) {
            deleteParticleEffect();
            deleteMakeConnectionParticleEffect();
            return;
        }

        var myHandPosition = getHandPosition(MyAvatar, currentHandJointIndex);
        var otherHandPosition;
        var otherOrientation;
        if (connectingId) {
            var other = AvatarList.getAvatar(connectingId);
            if (other) {
                otherOrientation = other.orientation;
                otherHandPosition = getHandPosition(other, connectingHandJointIndex);
            }
        }

        switch (state) {
        case STATES.WAITING:
            // no visualization while waiting
            deleteParticleEffect();
            deleteMakeConnectionParticleEffect();
            stopHandshakeSound();
            break;
        case STATES.CONNECTING:
            var particleProps = {};
            // put the position between the 2 hands, if we have a connectingId.  This
            // helps define the plane in which the particles move.
            positionFractionallyTowards(myHandPosition, otherHandPosition, 0.5);
            // now manage the rest of the entity
            if (!particleEffect) {
                particleEmitRate = PARTICLE_INITIAL_EMIT_RATE;
                particleProps = PARTICLE_EFFECT_PROPS;
                particleProps.position = positionFractionallyTowards(myHandPosition, otherHandPosition, 0.5);
                particleProps.rotation = Vec3.mix(Quat.getFront(MyAvatar.orientation),
                    Quat.inverse(Quat.getFront(otherOrientation)), 0.5);
                particleProps.parentID = MyAvatar.sessionUUID;
                particleEffect = Entities.addEntity(particleProps, true);
            }
            if (!makingConnectionParticleEffect) {
                var props = MAKING_CONNECTION_PARTICLE_PROPS;
                props.parentID = MyAvatar.sessionUUID;
                makingConnectionEmitRate = MAKING_CONNECTION_INITIAL_EMIT_RATE;
                props.emitRate = makingConnectionEmitRate;
                props.isEmitting = false;
                props.position = myHandPosition;
                makingConnectionParticleEffect = Entities.addEntity(props, true);
                makingConnectionUpdateTimer = Script.setTimeout(updateMakingConnection, MAKING_CONNECTION_UPDATE_INTERVAL);
            }
            break;
        case STATES.MAKING_CONNECTION:
            if (makingConnectionUpdateTimer) {
                Script.clearTimeout(makingConnectionUpdateTimer);
                makingConnectionUpdateTimer = null;
            }
            if (isMakingConnectionEmitting) {
                Entities.editEntity(makingConnectionParticleEffect, { isEmitting: false });
                isMakingConnectionEmitting = false;
            }
            if (!particleEffectUpdateTimer && particleEmitRate > PARTICLE_MINIMUM_EMIT_RATE) {
                particleEffectUpdateTimer = Script.setTimeout(updateParticleEffect, PARTICLE_EFFECT_UPDATE_INTERVAL);
            }
            break;
        default:
            debug("unexpected state", state);
            break;
        }
    }

    function isNearby() {
        if (currentHand) {
            var handPosition = getHandPosition(MyAvatar, currentHandJointIndex);
            var avatar = AvatarList.getAvatar(connectingId);
            if (avatar) {
                var distance = Vec3.distance(getHandPosition(avatar, connectingHandJointIndex), handPosition);
                return (distance < MAX_AVATAR_DISTANCE);
            }
        }
        return false;
    }
    function findNearestAvatar() {
        // We only look some max distance away (much larger than the handshake distance, but still...)
        var minDistance = MAX_AVATAR_DISTANCE * 20;
        var closestAvatar;
        AvatarList.getAvatarIdentifiers().forEach(function (id) {
            var avatar = AvatarList.getAvatar(id);
            if (avatar && avatar.sessionUUID != MyAvatar.sessionUUID) {
                var currentDistance = Vec3.distance(avatar.position, MyAvatar.position);
                if (minDistance > currentDistance) {
                    minDistance = currentDistance;
                    closestAvatar = avatar;
                }
            }
        });
        return closestAvatar;
    }
    function adjustAnimationHeight() {
        var avatar = findNearestAvatar();
        if (avatar) {
            var myHeadIndex = MyAvatar.getJointIndex("Head");
            var otherHeadIndex = avatar.getJointIndex("Head");
            var diff = (avatar.getJointPosition(otherHeadIndex).y - MyAvatar.getJointPosition(myHeadIndex).y) / 2;
            debug("head height difference: " + diff);
            updateAnimationData(diff);
        }
    }
    function findNearestWaitingAvatar() {
        var handPosition = getHandPosition(MyAvatar, currentHandJointIndex);
        var minDistance = MAX_AVATAR_DISTANCE;
        var nearestAvatar = {};
        Object.keys(waitingList).forEach(function (identifier) {
            var avatar = AvatarList.getAvatar(identifier);
            if (avatar) {
                var handJointIndex = waitingList[identifier];
                var distance = Vec3.distance(getHandPosition(avatar, handJointIndex), handPosition);
                if (distance < minDistance) {
                    minDistance = distance;
                    nearestAvatar = {avatarId: identifier, jointIndex: handJointIndex};
                }
            }
        });
        return nearestAvatar;
    }
    function messageSend(message) {
        // we always append whether or not we are logged in...
        message.isLoggedIn = Account.isLoggedIn();
        Messages.sendMessage(MESSAGE_CHANNEL, JSON.stringify(message));
    }
    function handStringMessageSend(message) {
        message[HAND_STRING_PROPERTY] = handToString(currentHand);
        messageSend(message);
    }
    function setupCandidate() { // find the closest in-range avatar, send connection request, and return true. (Otherwise falsey)
        var nearestAvatar = findNearestWaitingAvatar();
        if (nearestAvatar.avatarId) {
            connectingId = nearestAvatar.avatarId;
            connectingHandJointIndex = nearestAvatar.jointIndex;
            debug("sending connectionRequest to", connectingId);
            handStringMessageSend({
                key: "connectionRequest",
                id: connectingId
            });
            return true;
        }
    }
    function clearConnecting() {
        connectingId = undefined;
        connectingHandJointIndex = -1;
    }

    function lookForWaitingAvatar() {
        // we started with nobody close enough, but maybe I've moved
        // or they did.  Note that 2 people doing this race, so stop
        // as soon as you have a connectingId (which means you got their
        // message before noticing they were in range in this loop)

        // just in case we re-enter before stopping
        stopWaiting();
        debug("started looking for waiting avatars");
        waitingInterval = Script.setInterval(function () {
            if (state === STATES.WAITING && !connectingId) {
                setupCandidate();
            } else {
                // something happened, stop looking for avatars to connect
                stopWaiting();
                debug("stopped looking for waiting avatars");
            }
        }, WAITING_INTERVAL);
    }

    var pollCount = 0, requestUrl = Account.metaverseServerURL + '/api/v1/user/connection_request';
    // As currently implemented, we select the closest waiting avatar (if close enough) and send
    // them a connectionRequest.  If nobody is close enough we send a waiting message, and wait for a
    // connectionRequest.  If the 2 people who want to connect are both somewhat out of range when they
    // initiate the shake, they will race to see who sends the connectionRequest after noticing the
    // waiting message.  Either way, they will start connecting each other at that point.
    function startHandshake(fromKeyboard) {
        if (fromKeyboard) {
            startHandshakeAnimation();
        }
        debug("starting handshake for", currentHand);
        pollCount = 0;
        state = STATES.WAITING;
        clearConnecting();
        // just in case
        stopWaiting();
        stopConnecting();
        stopMakingConnection();
        if (!setupCandidate()) {
            // send waiting message
            debug("sending waiting message");
            handStringMessageSend({
                key: "waiting",
            });
            // potentially adjust height of handshake
            if (fromKeyboard) {
                adjustAnimationHeight();
            }
            lookForWaitingAvatar();
        }
    }

    function endHandshake() {
        debug("ending handshake for", currentHand);

        deleteParticleEffect();
        deleteMakeConnectionParticleEffect();
        currentHand = undefined;
        currentHandJointIndex = -1;
        // note that setting the state to inactive should really
        // only be done here, unless we change how the triggering works,
        // as we ignore the key release event when inactive.  See updateTriggers
        // below.
        state = STATES.INACTIVE;
        clearConnecting();
        stopWaiting();
        stopConnecting();
        stopMakingConnection();
        stopHandshakeSound();
        // send done to let connection know you are not making connections now
        messageSend({
            key: "done"
        });

        endHandshakeAnimation();
        // No-op if we were successful, but this way we ensure that failures and abandoned handshakes don't leave us
        // in a weird state.
        if (Account.isLoggedIn()) {
            request({ uri: requestUrl, method: 'DELETE' }, debug);
        }
    }

    function updateTriggers(value, fromKeyboard, hand) {
        if (currentHand && hand !== currentHand) {
            debug("currentHand", currentHand, "ignoring messages from", hand); // this can be a lot of spam on Touch. Should guard that someday.
            return;
        }
        // ok now, we are either initiating or quitting...
        var isGripping = value > GRIP_MIN;
        if (isGripping) {
            debug("updateTriggers called - gripping", handToString(hand));
            if (state !== STATES.INACTIVE) {
                return;
            }
            currentHand = hand;
            currentHandJointIndex = getIdealHandJointIndex(MyAvatar, handToString(currentHand)); // Always, in case of changed skeleton.
            startHandshake(fromKeyboard);
        } else {
            // TODO: should we end handshake even when inactive?  Ponder
            debug("updateTriggers called -- no longer gripping", handToString(hand));
            if (state !== STATES.INACTIVE) {
                endHandshake();
            } else {
                return;
            }
        }
    }

    /* There is a mini-state machine after entering STATES.makingConnection.
       We make a request (which might immediately succeed, fail, or neither.
       If we immediately fail, we tell the user.
       Otherwise, we wait MAKING_CONNECTION_TIMEOUT. At that time, we poll until success or fail.
     */
    var result, requestBody;
    function connectionRequestCompleted() { // Final result is in. Do effects.
        if (result.status === 'success') { // set earlier
            if (!successfulHandshakeInjector) {
                successfulHandshakeInjector = Audio.playSound(successfulHandshakeSound, {
                    position: getHandPosition(MyAvatar, currentHandJointIndex),
                    volume: 0.5,
                    localOnly: true
                });
            } else {
                successfulHandshakeInjector.restart();
            }
            Controller.triggerHapticPulse(HAPTIC_DATA.success.strength, HAPTIC_DATA.success.duration,
                handToHaptic(currentHand));
            // don't change state (so animation continues while gripped)
            // but do send a notification, by calling the slot that emits the signal for it
            Window.makeConnection(true,
                                  result.connection.new_connection ?
                                  "You and " + result.connection.username + " are now connected!" :
                                  result.connection.username);
            UserActivityLogger.makeUserConnection(connectingId,
                                                  true,
                                                  result.connection.new_connection ?
                                                  "new connection" :
                                                  "already connected");
            return;
        } // failed
        endHandshake();
        debug("failing with result data", result);
        // IWBNI we also did some fail sound/visual effect.
        Window.makeConnection(false, result.connection);
        if (Account.isLoggedIn()) { // Give extra failure info
            request(Account.metaverseServerURL + '/api/v1/users/' + Account.username + '/location', function (error, response) {
                var message = '';
                if (error || response.status !== 'success') {
                    message = 'Unable to get location.';
                } else if (!response.data || !response.data.location) {
                    message = "Unexpected location value: " + JSON.stringify(response);
                } else if (response.data.location.node_id !== cleanId(MyAvatar.sessionUUID)) {
                    message = 'Session identification does not match database. Maybe you are logged in on another machine? That would prevent handshakes.' + JSON.stringify(response) + MyAvatar.sessionUUID;
                }
                if (message) {
                    Window.makeConnection(false, message);
                }
                debug("account location:", message || 'ok');
            });
        }
        UserActivityLogger.makeUserConnection(connectingId, false, result.connection);
    }
    // This is a bit fragile - but to account for skew in when people actually create the
    // connection request, I've upped this to 2 seconds (plus the round-trip times)
    // TODO: keep track of when the person we are connecting with is done, and don't stop
    // until say 1 second after that.
    var POLL_INTERVAL_MS = 200, POLL_LIMIT = 10;
    function handleConnectionResponseAndMaybeRepeat(error, response) {
        // If response is 'pending', set a short timeout to try again.
        // If we fail other than pending, set result and immediately call connectionRequestCompleted.
        // If we succeed, set result and call connectionRequestCompleted immediately (if we've been polling),
        // and otherwise on a timeout.
        if (response && (response.connection === 'pending')) {
            debug(response, 'pollCount', pollCount);
            if (pollCount++ >= POLL_LIMIT) { // server will expire, but let's not wait that long.
                debug('POLL LIMIT REACHED; TIMEOUT: expired message generated by CLIENT');
                result = {status: 'error', connection: 'No logged-in partner found.'};
                connectionRequestCompleted();
            } else { // poll
                Script.setTimeout(function () {
                    request({
                        uri: requestUrl,
                        // N.B.: server gives bad request if we specify json content type, so don't do that.
                        body: requestBody
                    }, handleConnectionResponseAndMaybeRepeat);
                }, POLL_INTERVAL_MS);
            }
        } else if (error || (response.status !== 'success')) {
            debug('server fail', error, response.status);
            if (response && (response.statusCode === 401)) {
                error = "All participants must be logged in to connect.";
            }
            result = error ? {status: 'error', connection: error} : response;
            connectionRequestCompleted();
        } else {
            result = response;
            debug('server success', result);
            if (pollCount++) {
                connectionRequestCompleted();
            } else { // Wait for other guy, so that final success is at roughly the same time.
                Script.setTimeout(connectionRequestCompleted, MAKING_CONNECTION_TIMEOUT);
            }
        }
    }

    function makeConnection(id, isLoggedIn) {
        // send done to let the connection know you have made connection.
        messageSend({
            key: "done",
            connectionId: id
        });

        state = STATES.MAKING_CONNECTION;

        // continue the haptic background until the timeout fires.
        Controller.triggerHapticPulse(HAPTIC_DATA.background.strength, MAKING_CONNECTION_TIMEOUT, handToHaptic(currentHand));
        requestBody = {'node_id': cleanId(MyAvatar.sessionUUID), 'proposed_node_id': cleanId(id)}; // for use when repeating

        // It would be "simpler" to skip this and just look at the response, but:
        // 1. We don't want to bother the metaverse with request that we know will fail.
        // 2. We don't want our code here to be dependent on precisely how the metaverse responds (400, 401, etc.)
        // 3. We also don't want to connect to someone who is anonymous _now_, but was not earlier and still has
        //    the same node id.  Since the messaging doesn't say _who_ isn't logged in, fail the same as if we are
        //    not logged in.
        if (!Account.isLoggedIn() || isLoggedIn === false) {
            handleConnectionResponseAndMaybeRepeat("401:Unauthorized", {statusCode: 401});
            return;
        }

        // This will immediately set response if successful (e.g., the other guy got his request in first),
        // or immediate failure, and will otherwise poll (using the requestBody we just set).
        request({
            uri: requestUrl,
            method: 'POST',
            json: true,
            body: {'user_connection_request': requestBody}
        }, handleConnectionResponseAndMaybeRepeat);
    }
    function setupConnecting(id, jointIndex) {
        connectingId = id;
        connectingHandJointIndex = jointIndex;
    }

    // we change states, start the connectionInterval where we check
    // to be sure the hand is still close enough.  If not, we terminate
    // the interval, go back to the waiting state.  If we make it
    // the entire CONNECTING_TIME, we make the connection.  We pass in
    // whether or not the connecting id is actually logged in, as now we
    // will allow to start the connection process but have it stop with a
    // fail message before trying to call the backend if the other guy isn't
    // logged in.
    function startConnecting(id, jointIndex, isLoggedIn) {
        var count = 0;
        debug("connecting", id, "hand", jointIndex);
        // do we need to do this?
        setupConnecting(id, jointIndex);
        state = STATES.CONNECTING;

        // play sound
        if (!handshakeInjector) {
            handshakeInjector = Audio.playSound(handshakeSound, {
                position: getHandPosition(MyAvatar, currentHandJointIndex),
                volume: 0.5,
                localOnly: true
            });
        } else {
            handshakeInjector.restart();
        }

        // send message that we are connecting with them
        handStringMessageSend({
            key: "connecting",
            id: id
        });
        Controller.triggerHapticPulse(HAPTIC_DATA.initial.strength, HAPTIC_DATA.initial.duration, handToHaptic(currentHand));

        connectingInterval = Script.setInterval(function () {
            count += 1;
            Controller.triggerHapticPulse(HAPTIC_DATA.background.strength, HAPTIC_DATA.background.duration,
                handToHaptic(currentHand));
            if (state !== STATES.CONNECTING) {
                debug("stopping connecting interval, state changed");
                stopConnecting();
            } else if (!isNearby()) {
                // gotta go back to waiting
                debug(id, "moved, back to waiting");
                stopConnecting();
                messageSend({
                    key: "done"
                });
                startHandshake();
            } else if (count > CONNECTING_TIME / CONNECTING_INTERVAL) {
                debug("made connection with " + id);
                makeConnection(id, isLoggedIn);
                stopConnecting();
            }
        }, CONNECTING_INTERVAL);
    }
    /*
    A simple sequence diagram: NOTE that the ConnectionAck is somewhat
    vestigial, and probably should be removed shortly.

         Avatar A                       Avatar B
            |                               |
            |  <-----(waiting) ----- startHandshake
    startHandshake - (connectionRequest) -> |
            |                               |
            | <----(connectionAck) -------- |
            | <-----(connecting) -- startConnecting
     startConnecting ---(connecting) ---->  |
            |                               |
            |                            connected
       connected                            |
            | <---------  (done) ---------- |
            | ---------- (done) --------->  |
    */
    function messageHandler(channel, messageString, senderID) {
        var message = {};
        function exisitingOrSearchedJointIndex() { // If this is a new connectingId, we'll need to find the jointIndex
            return connectingId ? connectingHandJointIndex : getIdealHandJointIndex(AvatarList.getAvatar(senderID), message[HAND_STRING_PROPERTY]);
        }
        if (channel !== MESSAGE_CHANNEL) {
            return;
        }
        if (MyAvatar.sessionUUID === senderID) { // ignore my own
            return;
        }
        try {
            message = JSON.parse(messageString);
        } catch (e) {
            debug(e);
        }
        switch (message.key) {
        case "waiting":
            // add this guy to waiting object.  Any other message from this person will remove it from the list
            waitingList[senderID] = getIdealHandJointIndex(AvatarList.getAvatar(senderID), message[HAND_STRING_PROPERTY]);
            break;
        case "connectionRequest":
            delete waitingList[senderID];
            if (state === STATES.WAITING && message.id === MyAvatar.sessionUUID && (!connectingId || connectingId === senderID)) {
                // you were waiting for a connection request, so send the ack.  Or, you and the other
                // guy raced and both send connectionRequests.  Handle that too
                setupConnecting(senderID, exisitingOrSearchedJointIndex());
                handStringMessageSend({
                    key: "connectionAck",
                    id: senderID,
                });
            } else if (state === STATES.WAITING && connectingId === senderID) {
                // the person you are trying to connect sent a request to someone else.  See the
                // if statement above.  So, don't cry, just start the handshake over again
                startHandshake();
            }
            break;
        case "connectionAck":
            delete waitingList[senderID];
            if (state === STATES.WAITING && (!connectingId || connectingId === senderID)) {
                if (message.id === MyAvatar.sessionUUID) {
                    stopWaiting();
                    startConnecting(senderID, exisitingOrSearchedJointIndex(), message.isLoggedIn);
                } else if (connectingId) {
                    // this is for someone else (we lost race in connectionRequest),
                    // so lets start over
                    startHandshake();
                }
            }
            // TODO: check to see if we are waiting for this but the person we are connecting sent it to
            // someone else, and try again
            break;
        case "connecting":
            delete waitingList[senderID];
            if (state === STATES.WAITING && senderID === connectingId) {
                if (message.id !== MyAvatar.sessionUUID) {
                    // the person we were trying to connect is connecting to someone else
                    // so try again
                    startHandshake();
                    break;
                }
                startConnecting(senderID, connectingHandJointIndex, message.isLoggedIn);
            }
            break;
        case "done":
            delete waitingList[senderID];
            if (connectingId !== senderID) {
                break;
            }
            if (state === STATES.CONNECTING) {
                // if they are done, and didn't connect us, terminate our
                // connecting
                if (message.connectionId !== MyAvatar.sessionUUID) {
                    stopConnecting();
                    // now just call startHandshake.  Should be ok to do so without a
                    // value for isKeyboard, as we should not change the animation
                    // state anyways (if any)
                    startHandshake();
                } else {
                    // they just created a connection request to us, and we are connecting to
                    // them, so lets just stop connecting and make connection..
                    makeConnection(connectingId, message.isLoggedIn);
                    stopConnecting();
                }
            } else {
                if (state == STATES.MAKING_CONNECTION) {
                    // we are making connection, they just started, so lets reset the
                    // poll count just in case
                    pollCount = 0;
                } else {
                    // if waiting or inactive, lets clear the connecting id. If in makingConnection,
                    // do nothing
                    clearConnecting();
                    if (state !== STATES.INACTIVE) {
                        startHandshake();
                    }
                }
            }
            break;
        default:
            debug("unknown message", message);
            break;
        }
    }

    Messages.subscribe(MESSAGE_CHANNEL);
    Messages.messageReceived.connect(messageHandler);

    function makeGripHandler(hand, animate) {
        // determine if we are gripping or un-gripping
        if (animate) {
            return function (value) {
                updateTriggers(value, true, hand);
            };
        }
        return function (value) {
            updateTriggers(value, false, hand);
        };
    }

    function keyPressEvent(event) {
        if ((event.text.toUpperCase() === "X") && !event.isAutoRepeat && !event.isShifted && !event.isMeta && !event.isControl
                && !event.isAlt) {
            updateTriggers(1.0, true, Controller.Standard.RightHand);
        }
    }
    function keyReleaseEvent(event) {
        if (event.text.toUpperCase() === "X" && !event.isAutoRepeat) {
            updateTriggers(0.0, true, Controller.Standard.RightHand);
        }
    }
    // map controller actions
    var connectionMapping = Controller.newMapping(Script.resolvePath('') + '-grip');
    connectionMapping.from(Controller.Standard.LeftGrip).peek().to(makeGripHandler(Controller.Standard.LeftHand));
    connectionMapping.from(Controller.Standard.RightGrip).peek().to(makeGripHandler(Controller.Standard.RightHand));

    // setup keyboard initiation
    Controller.keyPressEvent.connect(keyPressEvent);
    Controller.keyReleaseEvent.connect(keyReleaseEvent);

    // Xbox controller because that is important
    connectionMapping.from(Controller.Standard.RB).peek().to(makeGripHandler(Controller.Standard.RightHand, true));

    // it is easy to forget this and waste a lot of time for nothing
    connectionMapping.enable();

    // connect updateVisualization to update frequently
    Script.update.connect(updateVisualization);

    // load the sounds when the script loads
    handshakeSound = SoundCache.getSound(HANDSHAKE_SOUND_URL);
    successfulHandshakeSound = SoundCache.getSound(SUCCESSFUL_HANDSHAKE_SOUND_URL);

    Script.scriptEnding.connect(function () {
        debug("removing controller mappings");
        connectionMapping.disable();
        debug("removing key mappings");
        Controller.keyPressEvent.disconnect(keyPressEvent);
        Controller.keyReleaseEvent.disconnect(keyReleaseEvent);
        debug("disconnecting updateVisualization");
        Script.update.disconnect(updateVisualization);
        deleteParticleEffect();
        deleteMakeConnectionParticleEffect();
    });

}()); // END LOCAL_SCOPE
