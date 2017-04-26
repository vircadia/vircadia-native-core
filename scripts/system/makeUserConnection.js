"use strict";
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
(function() { // BEGIN LOCAL_SCOPE

    var LABEL = "makeUserConnection";
    var MAX_AVATAR_DISTANCE = 0.2; // m
    var GRIP_MIN = 0.05; // goes from 0-1, so 5% pressed is pressed
    var MESSAGE_CHANNEL = "io.highfidelity.makeUserConnection";
    var STATES = {
        INACTIVE: 0,
        WAITING: 1,
        CONNECTING: 2,
        MAKING_CONNECTION: 3
    };
    var STATE_STRINGS = ["inactive", "waiting", "connecting", "makingConnection"];
    var WAITING_INTERVAL = 100; // ms
    var CONNECTING_INTERVAL = 100; // ms
    var MAKING_CONNECTION_TIMEOUT = 800; // ms
    var CONNECTING_TIME = 1600; // ms
    var PARTICLE_RADIUS = 0.15; // m
    var PARTICLE_ANGLE_INCREMENT = 360/45; // 1hz
    var HANDSHAKE_SOUND_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/davidkelly/production/audio/4beat_sweep.wav";
    var SUCCESSFUL_HANDSHAKE_SOUND_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/davidkelly/production/audio/3rdbeat_success_bell.wav";
    var PREFERRER_HAND_JOINT_POSTFIX_ORDER = ['Middle1', 'Index1', ''];
    var HAPTIC_DATA = {
        initial: { duration: 20, strength: 0.6 }, // duration is in ms
        background: { duration: 100, strength: 0.3 }, // duration is in ms
        success: { duration: 60, strength: 1.0 } // duration is in ms
    };
    var PARTICLE_EFFECT_PROPS = {
        "alpha": 0.8,
        "azimuthFinish": Math.PI,
        "azimuthStart": -1*Math.PI,
        "emitRate": 500,
        "emitSpeed": 0.0,
        "emitterShouldTrail": 1,
        "isEmitting": 1,
        "lifespan": 3,
        "maxParticles": 1000,
        "particleRadius": 0.003,
        "polarStart": 1,
        "polarFinish": 1,
        "radiusFinish": 0.008,
        "radiusStart": 0.0025,
        "speedSpread": 0.025,
        "textures": "http://hifi-content.s3.amazonaws.com/alan/dev/Particles/Bokeh-Particle.png",
        "color": {"red": 255, "green": 255, "blue": 255},
        "colorFinish": {"red": 0, "green": 164, "blue": 255},
        "colorStart": {"red": 255, "green": 255, "blue": 255},
        "emitOrientation": {"w": -0.71, "x":0.0, "y":0.0, "z": 0.71},
        "emitAcceleration": {"x": 0.0, "y": 0.0, "z": 0.0},
        "accelerationSpread": {"x": 0.0, "y": 0.0, "z": 0.0},
        "dimensions": {"x":0.05, "y": 0.05, "z": 0.05},
        "type": "ParticleEffect"
    };
    var MAKING_CONNECTION_PARTICLE_PROPS = {
        "alpha": 0.07,
        "alphaStart": 0.011,
        "alphaSpread": 0,
        "alphaFinish": 0,
        "azimuthFinish": Math.PI,
        "azimuthStart": -1*Math.PI,
        "emitRate": 2000,
        "emitSpeed": 0.0,
        "emitterShouldTrail": 1,
        "isEmitting": 1,
        "lifespan": 3.6,
        "maxParticles": 4000,
        "particleRadius": 0.048,
        "polarStart": 0,
        "polarFinish": 1,
        "radiusFinish": 0.3,
        "radiusStart": 0.04,
        "speedSpread": 0.01,
        "radiusSpread": 0.9,
        "textures": "http://hifi-content.s3.amazonaws.com/alan/dev/Particles/Bokeh-Particle.png",
        "color": {"red": 200, "green": 170, "blue": 255},
        "colorFinish": {"red": 0, "green": 134, "blue": 255},
        "colorStart": {"red": 185, "green": 222, "blue": 255},
        "emitOrientation": {"w": -0.71, "x":0.0, "y":0.0, "z": 0.71},
        "emitAcceleration": {"x": 0.0, "y": 0.0, "z": 0.0},
        "accelerationSpread": {"x": 0.0, "y": 0.0, "z": 0.0},
        "dimensions": {"x":0.05, "y": 0.05, "z": 0.05},
        "type": "ParticleEffect"
    };

    var currentHand = undefined;
    var currentHandJointIndex = -1;
    var state = STATES.INACTIVE;
    var connectingInterval;
    var waitingInterval;
    var makingConnectionTimeout;
    var animHandlerId;
    var connectingId;
    var connectingHandString;
    var connectingHandJointIndex = -1;
    var waitingList = {};
    var particleEffect;
    var particleRotationAngle = 0.0;
    var makingConnectionParticleEffect;
    var makingConnectionEmitRate = 2000;
    var particleEmitRate = 500;
    var handshakeInjector;
    var successfulHandshakeInjector;
    var handshakeSound;
    var successfulHandshakeSound;

    function debug() {
        var stateString = "<" + STATE_STRINGS[state] + ">";
        var connecting = "[" + connectingId + "/" + connectingHandString + "]";
        print.apply(null, [].concat.apply([LABEL, stateString, JSON.stringify(waitingList), connecting],
            [].map.call(arguments, JSON.stringify)));
    }

    function cleanId(guidWithCurlyBraces) {
        return guidWithCurlyBraces.slice(1, -1);
    }
    function request(options, callback) { // cb(error, responseOfCorrectContentType) of url. A subset of npm request.
        var httpRequest = new XMLHttpRequest(), key;
        // QT bug: apparently doesn't handle onload. Workaround using readyState.
        httpRequest.onreadystatechange = function () {
            var READY_STATE_DONE = 4;
            var HTTP_OK = 200;
            if (httpRequest.readyState >= READY_STATE_DONE) {
                var error = (httpRequest.status !== HTTP_OK) && httpRequest.status.toString() + ':' + httpRequest.statusText,
                    response = !error && httpRequest.responseText,
                    contentType = !error && httpRequest.getResponseHeader('content-type');
                if (!error && contentType.indexOf('application/json') === 0) { // ignoring charset, etc.
                    try {
                        response = JSON.parse(response);
                    } catch (e) {
                        error = e;
                    }
                }
                if (error) {
                    response = {statusCode: httpRequest.status};
                }
                callback(error, response);
            }
        };
        if (typeof options === 'string') {
            options = {uri: options};
        }
        if (options.url) {
            options.uri = options.url;
        }
        if (!options.method) {
            options.method = 'GET';
        }
        if (options.body && (options.method === 'GET')) { // add query parameters
            var params = [], appender = (-1 === options.uri.search('?')) ? '?' : '&';
            for (key in options.body) {
                if (options.body.hasOwnProperty(key)) {
                    params.push(key + '=' + options.body[key]);
                }
            }
            options.uri += appender + params.join('&');
            delete options.body;
        }
        if (options.json) {
            options.headers = options.headers || {};
            options.headers["Content-type"] = "application/json";
            options.body = JSON.stringify(options.body);
        }
        for (key in options.headers || {}) {
            if (options.headers.hasOwnProperty(key)) {
                httpRequest.setRequestHeader(key, options.headers[key]);
            }
        }
        httpRequest.open(options.method, options.uri, true);
        httpRequest.send(options.body);
    }

    function handToString(hand) {
        if (hand === Controller.Standard.RightHand) {
            return "RightHand";
        } else if (hand === Controller.Standard.LeftHand) {
            return "LeftHand";
        }
        debug("handToString called without valid hand! value: ", hand);
        return "";
    }

    function stringToHand(hand) {
        if (hand === "RightHand") {
            return Controller.Standard.RightHand;
        } else if (hand === "LeftHand") {
            return Controller.Standard.LeftHand;
        }
        debug("stringToHand called with bad hand string:", hand);
        return 0;
    }

    function handToHaptic(hand) {
        if (hand === Controller.Standard.RightHand) {
            return 1;
        } else if (hand === Controller.Standard.LeftHand) {
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
    // [hand]middle1 -> [hand]index1 -> [hand]
    function getIdealHandJointIndex(avatar, hand) {
        debug("got hand " + hand + " for avatar " + avatar.sessionUUID);
        var handString = handToString(hand);
        for (var i = 0; i < PREFERRER_HAND_JOINT_POSTFIX_ORDER.length; i++) {
            var jointName = handString + PREFERRER_HAND_JOINT_POSTFIX_ORDER[i];
            var jointIndex = avatar.getJointIndex(jointName);
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
            debug(new Error().stack);
            return avatar.position;
        }
        return avatar.getJointPosition(handJointIndex);
    }

    function shakeHandsAnimation(animationProperties) {
        // all we are doing here is moving the right hand to a spot
        // that is in front of and a bit above the hips.  Basing how
        // far in front as scaling with the avatar's height (say hips
        // to head distance)
        var headIndex = MyAvatar.getJointIndex("Head");
        var offset = 0.5; // default distance of hand in front of you
        var result = {};
        if (headIndex) {
            offset = 0.8 * MyAvatar.getAbsoluteJointTranslationInObjectFrame(headIndex).y;
        }
        result.rightHandPosition = Vec3.multiply(offset, {x: -0.25, y: 0.8, z: 1.3});
        result.rightHandRotation = Quat.fromPitchYawRollDegrees(90, 0, 90);
        return result;
    }

    function positionFractionallyTowards(posA, posB, frac) {
        return Vec3.sum(posA, Vec3.multiply(frac, Vec3.subtract(posB, posA)));
    }

    function deleteParticleEffect() {
        if (particleEffect) {
            particleEffect = Entities.deleteEntity(particleEffect);
        }
    }

    function deleteMakeConnectionParticleEffect() {
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

    function calcParticlePos(myHand, otherHand, otherOrientation, reset) {
        if (reset) {
            particleRotationAngle = 0.0;
        }
        var position = positionFractionallyTowards(myHand, otherHand, 0.5);
        particleRotationAngle += PARTICLE_ANGLE_INCREMENT; // about 0.5 hz
        var radius = Math.min(PARTICLE_RADIUS, PARTICLE_RADIUS * particleRotationAngle / 360);
        var axis = Vec3.mix(Quat.getFront(MyAvatar.orientation), Quat.inverse(Quat.getFront(otherOrientation)), 0.5);
        return Vec3.sum(position, Vec3.multiplyQbyV(Quat.angleAxis(particleRotationAngle, axis), {x: 0, y: radius, z: 0}));
    }

    // this is called frequently, but usually does nothing
    function updateVisualization() {
        if (state === STATES.INACTIVE) {
            deleteParticleEffect();
            deleteMakeConnectionParticleEffect();
            return;
        }

        var myHandPosition = getHandPosition(MyAvatar, currentHandJointIndex);
        var otherHand;
        var otherOrientation;
        if (connectingId) {
            var other = AvatarList.getAvatar(connectingId);
            if (other) {
                otherOrientation = other.orientation;
                otherHand = getHandPosition(other, connectingHandJointIndex);
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
                positionFractionallyTowards(myHandPosition, otherHand, 0.5);
                // now manage the rest of the entity
                if (!particleEffect) {
                    particleRotationAngle = 0.0;
                    particleEmitRate = 500;
                    particleProps = PARTICLE_EFFECT_PROPS;
                    particleProps.isEmitting = 0;
                    particleProps.position = calcParticlePos(myHandPosition, otherHand, otherOrientation);
                    particleProps.parentID = MyAvatar.sessionUUID;
                    particleEffect = Entities.addEntity(particleProps, true);
                } else {
                    particleProps.position = calcParticlePos(myHandPosition, otherHand, otherOrientation);
                    particleProps.isEmitting = 1;
                    Entities.editEntity(particleEffect, particleProps);
                }
                if (!makingConnectionParticleEffect) {
                    var props = MAKING_CONNECTION_PARTICLE_PROPS;
                    props.parentID = MyAvatar.sessionUUID;
                    makingConnectionEmitRate = 2000;
                    props.emitRate = makingConnectionEmitRate;
                    props.position = myHandPosition;
                    makingConnectionParticleEffect = Entities.addEntity(props, true);
                } else {
                    makingConnectionEmitRate *= 0.5;
                    Entities.editEntity(makingConnectionParticleEffect, {
                        emitRate: makingConnectionEmitRate,
                        position: myHandPosition,
                        isEmitting: true
                    });
                }
                break;
            case STATES.MAKING_CONNECTION:
                particleEmitRate = Math.max(50, particleEmitRate * 0.5);
                Entities.editEntity(makingConnectionParticleEffect, {emitRate: 0, isEmitting: 0, position: myHandPosition});
                Entities.editEntity(particleEffect, {
                    position: calcParticlePos(myHandPosition, otherHand, otherOrientation),
                    emitRate: particleEmitRate
                });
                break;
            default:
                debug("unexpected state", state);
                break;
        }
    }

    function isNearby(id, hand) {
        if (currentHand) {
            var handPos = getHandPosition(MyAvatar, currentHandJointIndex);
            var avatar = AvatarList.getAvatar(id);
            if (avatar) {
                var otherHand = stringToHand(hand);
                var otherHandJointIndex = getIdealHandJointIndex(avatar, otherHand);
                var distance = Vec3.distance(getHandPosition(avatar, otherHandJointIndex), handPos);
                return (distance < MAX_AVATAR_DISTANCE);
            }
        }
        return false;
    }

    function findNearestWaitingAvatar() {
        var handPos = getHandPosition(MyAvatar, currentHandJointIndex);
        var minDistance = MAX_AVATAR_DISTANCE;
        var nearestAvatar = {};
        Object.keys(waitingList).forEach(function (identifier) {
            var avatar = AvatarList.getAvatar(identifier);
            if (avatar) {
                var hand = stringToHand(waitingList[identifier]);
                var handJointIndex = getIdealHandJointIndex(avatar, hand);
                var distance = Vec3.distance(getHandPosition(avatar, handJointIndex), handPos);
                if (distance < minDistance) {
                    minDistance = distance;
                    nearestAvatar = {avatar: identifier, hand: hand, avatarObject: avatar};
                }
            }
        });
        return nearestAvatar;
    }


    // As currently implemented, we select the closest waiting avatar (if close enough) and send
    // them a connectionRequest.  If nobody is close enough we send a waiting message, and wait for a
    // connectionRequest.  If the 2 people who want to connect are both somewhat out of range when they
    // initiate the shake, they will race to see who sends the connectionRequest after noticing the
    // waiting message.  Either way, they will start connecting eachother at that point.
    function startHandshake(fromKeyboard) {
        if (fromKeyboard) {
            debug("adding animation");
            // just in case order of press/unpress is broken
            if (animHandlerId) {
                animHandlerId = MyAvatar.removeAnimationStateHandler(animHandlerId);
            }
            animHandlerId = MyAvatar.addAnimationStateHandler(shakeHandsAnimation, []);
        }
        debug("starting handshake for", currentHand);
        pollCount = 0;
        state = STATES.WAITING;
        connectingId = undefined;
        connectingHandString = undefined;
        connectingHandJointIndex = -1;
        // just in case
        stopWaiting();
        stopConnecting();
        stopMakingConnection();

        var nearestAvatar = findNearestWaitingAvatar();
        if (nearestAvatar.avatar) {
            connectingId = nearestAvatar.avatar;
            connectingHandString = handToString(nearestAvatar.hand);
            connectingHandJointIndex = getIdealHandJointIndex(nearestAvatar.avatarObject, nearestAvatar.hand);
            currentHandJointIndex = getIdealHandJointIndex(MyAvatar, currentHand);
            debug("sending connectionRequest to", connectingId);
            messageSend({
                key: "connectionRequest",
                id: connectingId,
                hand: handToString(currentHand)
            });
        } else {
            // send waiting message
            debug("sending waiting message");
            messageSend({
                key: "waiting",
                hand: handToString(currentHand)
            });
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
        connectingId = undefined;
        connectingHandString = undefined;
        connectingHandJointIndex = -1;
        stopWaiting();
        stopConnecting();
        stopMakingConnection();
        stopHandshakeSound();
        // send done to let connection know you are not making connections now
        messageSend({
            key: "done"
        });

        if (animHandlerId) {
            debug("removing animation");
            MyAvatar.removeAnimationStateHandler(animHandlerId);
        }
        // No-op if we were successful, but this way we ensure that failures and abandoned handshakes don't leave us
        // in a weird state.
        request({uri: requestUrl, method: 'DELETE'}, debug);
    }

    function updateTriggers(value, fromKeyboard, hand) {
        if (currentHand && hand !== currentHand) {
            debug("currentHand", currentHand, "ignoring messages from", hand);
            return;
        }
        if (!currentHand) {
            currentHand = hand;
            currentHandJointIndex = getIdealHandJointIndex(MyAvatar, currentHand);
        }
        // ok now, we are either initiating or quitting...
        var isGripping = value > GRIP_MIN;
        if (isGripping) {
            debug("updateTriggers called - gripping", handToString(hand));
            if (state !== STATES.INACTIVE) {
                return;
            } else {
                startHandshake(fromKeyboard);
            }
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

    function messageSend(message) {
        Messages.sendMessage(MESSAGE_CHANNEL, JSON.stringify(message));
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
                // find the closest in-range avatar, and send connection request
                var nearestAvatar = findNearestWaitingAvatar();
                if (nearestAvatar.avatar) {
                    connectingId = nearestAvatar.avatar;
                    connectingHandString = handToString(nearestAvatar.hand);
                    debug("sending connectionRequest to", connectingId);
                    messageSend({
                        key: "connectionRequest",
                        id: connectingId,
                        hand: handToString(currentHand)
                    });
                }
            } else {
                // something happened, stop looking for avatars to connect
                stopWaiting();
                debug("stopped looking for waiting avatars");
            }
        }, WAITING_INTERVAL);
    }

    /* There is a mini-state machine after entering STATES.makingConnection.
       We make a request (which might immediately succeed, fail, or neither.
       If we immediately fail, we tell the user.
       Otherwise, we wait MAKING_CONNECTION_TIMEOUT. At that time, we poll until success or fail.
     */
    var result, requestBody, pollCount = 0, requestUrl = location.metaverseServerUrl + '/api/v1/user/connection_request';
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
            Window.makeConnection(true, result.connection.new_connection ?
                "You and " + result.connection.username + " are now connected!" : result.connection.username);
            UserActivityLogger.makeUserConnection(connectingId, true, result.connection.new_connection ?
                "new connection" : "already connected");
            return;
        } // failed
        endHandshake();
        debug("failing with result data", result);
        // IWBNI we also did some fail sound/visual effect.
        Window.makeConnection(false, result.connection);
        UserActivityLogger.makeUserConnection(connectingId, false, result.connection);
    }
    var POLL_INTERVAL_MS = 200, POLL_LIMIT = 5;
    function handleConnectionResponseAndMaybeRepeat(error, response) {
        // If response is 'pending', set a short timeout to try again.
        // If we fail other than pending, set result and immediately call connectionRequestCompleted.
        // If we succeed, set result and call connectionRequestCompleted immediately (if we've been polling),
        // and otherwise on a timeout.
        if (response && (response.connection === 'pending')) {
            debug(response, 'pollCount', pollCount);
            if (pollCount++ >= POLL_LIMIT) { // server will expire, but let's not wait that long.
                debug('POLL LIMIT REACHED; TIMEOUT: expired message generated by CLIENT');
                result = {status: 'error', connection: 'expired'};
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
            UserActivityLogger.makeUserConnection(connectingId, false, error || response);
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

    // this should be where we make the appropriate connection call.  For now just make the
    // visualization change.
    function makeConnection(id) {
        // send done to let the connection know you have made connection.
        messageSend({
            key: "done",
            connectionId: id
        });

        state = STATES.MAKING_CONNECTION;

        // continue the haptic background until the timeout fires.  When we make calls, we will have an interval
        // probably, in which we do this.
        Controller.triggerHapticPulse(HAPTIC_DATA.background.strength, MAKING_CONNECTION_TIMEOUT, handToHaptic(currentHand));
        requestBody = {'node_id': cleanId(MyAvatar.sessionUUID), 'proposed_node_id': cleanId(id)}; // for use when repeating

        // It would be "simpler" to skip this and just look at the response, but:
        // 1. We don't want to bother the metaverse with request that we know will fail.
        // 2. We don't want our code here to be dependent on precisely how the metaverse responds (400, 401, etc.)
        if (!Account.isLoggedIn()) {
            handleConnectionResponseAndMaybeRepeat("401:Unauthorized", {statusCode: 401});
            return;
        }

        // This will immediately set response if successful (e.g., the other guy got his request in first),
        // or immediate failure, and will otherwise poll (using the requestBody we just set).
        request({ //
            uri: requestUrl,
            method: 'POST',
            json: true,
            body: {'user_connection_request': requestBody}
        }, handleConnectionResponseAndMaybeRepeat);
    }

    // we change states, start the connectionInterval where we check
    // to be sure the hand is still close enough.  If not, we terminate
    // the interval, go back to the waiting state.  If we make it
    // the entire CONNECTING_TIME, we make the connection.
    function startConnecting(id, hand) {
        var count = 0;
        debug("connecting", id, "hand", hand);
        // do we need to do this?
        connectingId = id;
        connectingHandString = hand;
        connectingHandJointIndex = AvatarList.getAvatarIdentifiers().indexOf(connectingId) !== -1 ?
            getIdealHandJointIndex(AvatarList.getAvatar(connectingId), stringToHand(connectingHandString)) : -1;
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
        messageSend({
            key: "connecting",
            id: id,
            hand: handToString(currentHand)
        });
        Controller.triggerHapticPulse(HAPTIC_DATA.initial.strength, HAPTIC_DATA.initial.duration, handToHaptic(currentHand));

        connectingInterval = Script.setInterval(function () {
            count += 1;
            Controller.triggerHapticPulse(HAPTIC_DATA.background.strength, HAPTIC_DATA.background.duration,
                handToHaptic(currentHand));
            if (state !== STATES.CONNECTING) {
                debug("stopping connecting interval, state changed");
                stopConnecting();
            } else if (!isNearby(id, hand)) {
                // gotta go back to waiting
                debug(id, "moved, back to waiting");
                stopConnecting();
                messageSend({
                    key: "done"
                });
                startHandshake();
            } else if (count > CONNECTING_TIME/CONNECTING_INTERVAL) {
                debug("made connection with " + id);
                makeConnection(id);
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
        if (channel !== MESSAGE_CHANNEL) {
            return;
        }
        if (MyAvatar.sessionUUID === senderID) { // ignore my own
            return;
        }
        var message = {};
        try {
            message = JSON.parse(messageString);
        } catch (e) {
            debug(e);
        }
        switch (message.key) {
            case "waiting":
                // add this guy to waiting object.  Any other message from this person will
                // remove it from the list
                waitingList[senderID] = message.hand;
                break;
            case "connectionRequest":
                delete waitingList[senderID];
                if (state === STATES.WAITING && message.id === MyAvatar.sessionUUID &&
                   (!connectingId || connectingId === senderID)) {
                    // you were waiting for a connection request, so send the ack.  Or, you and the other
                    // guy raced and both send connectionRequests.  Handle that too
                    connectingId = senderID;
                    connectingHandString = message.hand;
                    connectingHandJointIndex = AvatarList.getAvatarIdentifiers().indexOf(connectingId) !== -1 ?
                        getIdealHandJointIndex(AvatarList.getAvatar(connectingId), stringToHand(connectingHandString)) : -1;
                    messageSend({
                        key: "connectionAck",
                        id: senderID,
                        hand: handToString(currentHand)
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
                        // start connecting...
                        connectingId = senderID;
                        connectingHandString = message.hand;
                        connectingHandJointIndex = AvatarList.getAvatarIdentifiers().indexOf(connectingId) !== -1 ?
                            getIdealHandJointIndex(AvatarList.getAvatar(connectingId), stringToHand(connectingHandString)) : -1;
                        stopWaiting();
                        startConnecting(senderID, connectingHandString);
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
                    // temporary logging
                    if (connectingHandString !== message.hand) {
                        debug("connecting hand", connectingHandString, "not same as connecting hand in message", message.hand);
                    }
                    connectingHandString = message.hand;
                    if (message.id !== MyAvatar.sessionUUID) {
                        // the person we were trying to connect is connecting to someone else
                        // so try again
                        startHandshake();
                        break;
                    }
                    startConnecting(senderID, message.hand);
                }
                break;
            case "done":
                delete waitingList[senderID];
                if (state === STATES.CONNECTING && connectingId === senderID) {
                    // if they are done, and didn't connect us, terminate our
                    // connecting
                    if (message.connectionId !== MyAvatar.sessionUUID) {
                        stopConnecting();
                        // now just call startHandshake.  Should be ok to do so without a
                        // value for isKeyboard, as we should not change the animation
                        // state anyways (if any)
                        startHandshake();
                    }
                } else {
                    // if waiting or inactive, lets clear the connecting id. If in makingConnection,
                    // do nothing
                    if (state !== STATES.MAKING_CONNECTION && connectingId === senderID) {
                        connectingId = undefined;
                        connectingHandString = undefined;
                        connectingHandJointIndex = -1;
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
            return function(value) {
                updateTriggers(value, true, hand);
            };

        } else {
            return function (value) {
                updateTriggers(value, false, hand);
            };
        }
    }

    function keyPressEvent(event) {
        if ((event.text === "x") && !event.isAutoRepeat && !event.isShifted && !event.isMeta && !event.isControl &&
           !event.isAlt) {
            updateTriggers(1.0, true, Controller.Standard.RightHand);
        }
    }
    function keyReleaseEvent(event) {
        if ((event.text === "x") && !event.isAutoRepeat && !event.isShifted && !event.isMeta && !event.isControl &&
           !event.isAlt) {
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
