"use strict";
//
//  friends.js
//  scripts/developer/tests/performance/
//
//  Created by David Kelly on 3/7/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
const version = 0.1;
const label = "Friends";
const MAX_AVATAR_DISTANCE = 1.0;
const GRIP_MIN = 0.05;
const MESSAGE_CHANNEL = "io.highfidelity.friends";
const STATES = {
    inactive : 0,
    waiting: 1,
    friending: 2,
};
const STATE_STRINGS = ["inactive", "waiting", "friending"];
const WAITING_INTERVAL = 100; // ms
const FRIENDING_INTERVAL = 100; // ms
const FRIENDING_TIME = 3000; // ms
const OVERLAY_COLORS = [{red: 0x00, green: 0xFF, blue: 0x00}, {red: 0x00, green: 0x00, blue: 0xFF}];
const FRIENDING_HAPTIC_STRENGTH = 0.5;
const FRIENDING_SUCCESS_HAPTIC_STRENGTH = 1.0;
const HAPTIC_DURATION = 20;

var currentHand;
var state = STATES.inactive;
var friendingInterval;
var entity;
var makingFriends = false; // really just for visualizations for now
var animHandlerId;
var entityDimensionMultiplier = 1.0;
var friendingId;
var pendingFriendAckFrom;
var latestFriendRequestFrom;

function debug() {
    var stateString = "<" + STATE_STRINGS[state] + ">";
    var versionString = "v" + version;
    print.apply(null, [].concat.apply([label, versionString, stateString], [].map.call(arguments, JSON.stringify)));
}

function handToString(hand) {
    if (hand === Controller.Standard.RightHand) {
        return "RightHand";
    } else if (hand === Controller.Standard.LeftHand) {
        return "LeftHand";
    }
    return "";
}

function handToHaptic(hand) {
    if (hand === Controller.Standard.RightHand) {
        return 1;
    } else if (hand === Controller.Standard.LeftHand) {
        return 0;
    }
    return -1;
}


function getHandPosition(avatar, hand) {
    if (!hand) {
        debug("calling getHandPosition with no hand!");
        return;
    }
    var jointName = handToString(hand) + "Middle1";
    return avatar.getJointPosition(avatar.getJointIndex(jointName));
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
    var handPos = Vec3.multiply(offset, {x: -0.25, y: 0.8, z: 1.3});
    result.rightHandPosition = handPos;
    result.rightHandRotation = Quat.fromPitchYawRollDegrees(90, 0, 90);
    return result;
}

// this is called frequently, but usually does nothing
function updateVisualization() {
    if (state == STATES.inactive) {
        if (entity) {
            entity = Entities.deleteEntity(entity);
        }
        return;
    }

    var color = state == STATES.waiting ? OVERLAY_COLORS[0] : OVERLAY_COLORS[1];
    var position = getHandPosition(MyAvatar, currentHand);

    // temp code, though all of this stuff really is temp...
    if (makingFriends) {
        color = { red: 0xFF, green: 0x00, blue: 0x00 };
    }

    // TODO: make the size scale with avatar, up to
    // the actual size of MAX_AVATAR_DISTANCE
    var wrist = MyAvatar.getJointPosition(MyAvatar.getJointIndex(handToString(currentHand)));
    var d = entityDimensionMultiplier * Vec3.distance(wrist, position);
    var dimension = {x: d, y: d, z: d};
    if (!entity) {
        var props =  {
            type: "Sphere",
            color: color,
            position: position,
            dimensions: dimension
        }
        entity = Entities.addEntity(props);
    } else {
        Entities.editEntity(entity, {dimensions: dimension, position: position, color: color});
    }

}
function findNearbyAvatars(nearestOnly) {
    var handPos = getHandPosition(MyAvatar, currentHand);
    var minDistance = MAX_AVATAR_DISTANCE;
    var nearbyAvatars = [];
    AvatarList.getAvatarIdentifiers().forEach(function (identifier) {
        if (!identifier) { return; }
        var avatar = AvatarList.getAvatar(identifier);
        var distanceR = Vec3.distance(getHandPosition(avatar, Controller.Standard.RightHand), handPos);
        var distanceL = Vec3.distance(getHandPosition(avatar, Controller.Standard.LeftHand), handPos);
        var distance = Math.min(distanceL, distanceR);
        if (distance < minDistance) {
            if (nearestOnly) {
                minDistance = distance;
                nearbyAvatars = [];
            }
            var hand = (distance == distanceR ? Controller.Standard.RightHand : Controller.Standard.LeftHand);
            nearbyAvatars.push({avatar: identifier, hand: hand});
        }
    });
    return nearbyAvatars;
}

function startHandshake(fromKeyboard) {
    if (fromKeyboard) {
        debug("adding animation");
        animHandlerId = MyAvatar.addAnimationStateHandler(shakeHandsAnimation, []);
    }
    debug("starting handshake for", currentHand);
    state = STATES.waiting;
    entityDimensionMultiplier = 1.0;
    pendingFriendAckFrom = undefined;
    //  if we have a recent friendRequest, send an ack back
    if (latestFriendRequestFrom) {
        messageSend({
            key: "friendAck",
            id: latestFriendRequestFrom,
            hand: handToString(currentHand)
        });
    } else {
        var nearestAvatar = findNearbyAvatars(true)[0];
        if (nearestAvatar) {
            pendingFriendAckFrom = nearestAvatar.avatar;
            messageSend({
                key: "friendRequest",
                id: nearestAvatar.avatar,
                hand: handToString(nearestAvatar.hand)
            });
        }
    }
}

function endHandshake() {
    debug("ending handshake for", currentHand);
    currentHand = undefined;
    state = STATES.inactive;
    if (friendingInterval) {
        friendingInterval = Script.clearInterval(friendingInterval);
        // send done to let friend know you are not making friends now
        messageSend({
            key: "done"
        });
    }
    if (animHandlerId) {
        debug("removing animation");
        MyAvatar.removeAnimationStateHandler(animHandlerId);
    }
}

function updateTriggers(value, fromKeyboard, hand) {
    if (currentHand && hand !== currentHand) {
        debug("currentHand", currentHand, "ignoring messages from", hand);
        return;
    }
    if (!currentHand) {
        currentHand = hand;
    }
    // ok now, we are either initiating or quitting...
    var isGripping = value > GRIP_MIN;
    if (isGripping) {
        if (state != STATES.inactive) {
            return;
        } else {
            startHandshake(fromKeyboard);
        }
    } else {
        if (state != STATES.inactive) {
            endHandshake();
        } else {
            return;
        }
    }
}

function messageSend(message) {
    Messages.sendMessage(MESSAGE_CHANNEL, JSON.stringify(message));
}

function isNearby(id, hand) {
    var nearbyAvatars = findNearbyAvatars();
    for(var i = 0; i < nearbyAvatars.length; i++) {
        if (nearbyAvatars[i].avatar == id && handToString(nearbyAvatars[i].hand) == hand) {
            return true;
        }
    }
    return false;
}

// this should be where we make the appropriate friend call.  For now just make the
// visualization change.
function makeFriends(id) {
    // temp code to just flash the visualization really (for now!)
    makingFriends = true;
    // send done to let the friend know you have made friends.
    messageSend({
        key: "done",
        friendId: id
    });
    Controller.triggerHapticPulse(FRIENDING_SUCCESS_HAPTIC_STRENGTH, HAPTIC_DURATION, handToHaptic(currentHand));
    Script.setTimeout(function () { makingFriends = false; entityDimensionMultiplier = 1.0; }, 1000);
}
// we change states, start the friendingInterval where we check
// to be sure the hand is still close enough.  If not, we terminate
// the interval, go back to the waiting state.  If we make it
// the entire FRIENDING_TIME, we make friends.
function startFriending(id, hand) {
    var count = 0;
    debug("friending", id, "hand", hand);
    friendingId = id;
    pendingFriendAckFrom = undefined;
    latestFriendRequestFrom = undefined;
    state = STATES.friending;
    Controller.triggerHapticPulse(FRIENDING_HAPTIC_STRENGTH, HAPTIC_DURATION, handToHaptic(currentHand));

    // send message that we are friending them
    messageSend({
        key: "friending",
        id: id,
        hand: handToString(currentHand)
    });

    friendingInterval = Script.setInterval(function () {
        entityDimensionMultiplier = 1.0 + 2.0 * ++count * FRIENDING_INTERVAL / FRIENDING_TIME;
        if (state != STATES.friending) {
            debug("stopping friending interval, state changed");
            friendingInterval = Script.clearInterval(friendingInterval);
        } else if (!isNearby(id, hand)) {
            // gotta go back to waiting
            debug(id, "moved, back to waiting");
            friendingInterval = Script.clearInterval(friendingInterval);
            startHandshake();
        } else if (count > FRIENDING_TIME/FRIENDING_INTERVAL) {
            debug("made friends with " + id);
            makeFriends(id);
            friendingInterval = Script.clearInterval(friendingInterval);
        }
    }, FRIENDING_INTERVAL);
}
/*
A simple sequence diagram:

     Avatar A                       Avatar B
        |                               |
        |  <-----(FriendRequest) -- startHandshake
   startHandshake -- (FriendAck) --->   |
        |                               |
        |  <-------(friending) -- startFriending
   startFriending -- (friending) --->   |
        |                               |
        |                            friends
     friends                            |
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
    case "friendRequest":
        if (state == STATES.inactive && message.id == MyAvatar.sessionUUID) {
            latestFriendRequestFrom = senderID;
        } else if (state == STATES.waiting && (pendingFriendAckFrom == senderID || !pendingFriendAckFrom)) {
            // you are waiting for a friend request, so send the ack.  Or, you and the other
            // guy raced and both send friendRequests.  Handle that too
            pendingFriendAckFrom = senderID;
            messageSend({
                key: "friendAck",
                id: senderID,
                hand: handToString(currentHand)
            });
        }
        // TODO: ponder keeping this up-to-date during
        // other states?
        break;
    case "friendAck":
        if (state == STATES.waiting && message.id == MyAvatar.sessionUUID) {
            if (pendingFriendAckFrom && senderID != pendingFriendAckFrom) {
                debug("ignoring friendAck from", senderID, ", waiting on", pendingFriendAckFrom);
                break;
            }
            // start friending...
            startFriending(senderID, message.hand);
        }
        break;
    case "friending":
        if (state == STATES.waiting && senderID == latestFriendRequestFrom) {
            if (message.id != MyAvatar.sessionUUID) {
                // for now, just ignore these. Hmm
                debug("ignoring friending message", message, "from", senderID);
                break;
            }
            startFriending(senderID, message.hand);
        }
        break;
    case "done":
        if (state == STATES.friending && friendingId == senderID) {
            // if they are done, and didn't friend us, terminate our
            // friending
            if (message.friendId !== friendingId) {
                if (friendingInterval) {
                    friendingInterval = Script.clearInterval(friendingInterval);
                }
                // now just call startHandshake.  Should be ok to do so without a
                // value for isKeyboard, as we should not change the animation
                // state anyways (if any)
                startHandshake();
            }
        } else {
            // if waiting or inactive, lets clear the pending stuff
            if (pendingFriendAckFrom == senderID || lastestFriendRequestFrom == senderID) {
                if (state == STATES.inactive) {
                    pendingFriendAckFrom = undefined;
                    latestFriendRequestFrom = undefined;
                } else {
                    startHandshake();
                }
            }
        }
        break;
    default:
        debug("unknown message", message);
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
    if ((event.text === "x") && !event.isAutoRepeat) {
        updateTriggers(1.0, true, Controller.Standard.RightHand);
    }
}
function keyReleaseEvent(event) {
    if ((event.text === "x") && !event.isAutoRepeat) {
        updateTriggers(0.0, true, Controller.Standard.RightHand);
    }
}
// map controller actions
var friendsMapping = Controller.newMapping(Script.resolvePath('') + '-grip');
friendsMapping.from(Controller.Standard.LeftGrip).peek().to(makeGripHandler(Controller.Standard.LeftHand));
friendsMapping.from(Controller.Standard.RightGrip).peek().to(makeGripHandler(Controller.Standard.RightHand));

// setup keyboard initiation
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);

// xbox controller cuz that's important
friendsMapping.from(Controller.Standard.RB).peek().to(makeGripHandler(Controller.Standard.RightHand, true));

// it is easy to forget this and waste a lot of time for nothing
friendsMapping.enable();

// connect updateVisualization to update frequently
Script.update.connect(updateVisualization);

Script.scriptEnding.connect(function () {
    debug("removing controller mappings");
    friendsMapping.disable();
    debug("removing key mappings");
    Controller.keyPressEvent.disconnect(keyPressEvent);
    Controller.keyReleaseEvent.disconnect(keyReleaseEvent);
    debug("disconnecting updateVisualization");
    Script.update.disconnect(updateVisualization);
    if (entity) {
        entity = Entities.deleteEntity(entity);
    }
});

