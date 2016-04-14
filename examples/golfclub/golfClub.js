//
//  golfClub.js
//
//  Created by Philip Rosedale on April 11, 2016.
//  Copyright 2015 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  A simple golf club.  If you have equipped it, and pull trigger, it will either make 
//  you a new golf ball, or take you to your ball if one is not made. 

(function () {
    var ball = null;
    var HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
    var collisionSoundURL = HIFI_PUBLIC_BUCKET + "sounds/Collisions-ballhitsandcatches/billiards/collision1.wav";
    var triggerState = false;
    var BALL_GRAVITY = -9.8;
    var BALL_START_VELOCITY = 0.1;
    var BALL_MAX_RANGE = 10;
    var BALL_DROP_DISTANCE = 0.6;
    var BALL_DIAMETER = 0.07;
    var BALL_LIFETIME = 3600;
    var MAX_BRAKING_SPEED = 0.2;
    var BALL_BRAKING_RATE = 0.5;

    var TRIGGER_CONTROLS = [
        Controller.Standard.LT,
        Controller.Standard.RT,
    ];

function triggerPulled(hand) {
    //  Return true if the trigger has just been pulled
    var triggerValue = Controller.getValue(TRIGGER_CONTROLS[hand]);
    var oldTriggerState = triggerState;
    var TRIGGER_PULL_THRESHOLD = 0.5;
    var TRIGGER_RELEASE_THRESHOLD = 0.4;
    if (triggerValue > TRIGGER_PULL_THRESHOLD) {
        triggerState = true;
    }  else if (triggerValue < TRIGGER_RELEASE_THRESHOLD) {
        triggerState = false;
    }
    return (triggerState && (oldTriggerState != triggerState)); 
}

function ballPosition(ball) {
    //  return the position of this entity
    var properties = Entities.getEntityProperties(ball, ['position']);
    if (!properties) {
        return null;
    } else {
        return properties.position;
    }
}

function gotoPointOnGround(position, away) {
    // Position yourself facing in the direction you were originally facing, but with a 
    // point on the ground *away* meters from *position* and in front of you.

    var offset = Quat.getFront(MyAvatar.orientation);
    offset.y = 0.0;
    offset = Vec3.multiply(-away, Vec3.normalize(offset));
    var newAvatarPosition = Vec3.sum(position, offset);

    // Assuming position is on ground, put me distance from eyes to hips higher, plus a 50% adjust for longer legs.
    // TODO: Need avatar callback for exact foot-to-hips height.

    var halfHeight = avatarHalfHeight();
    print("Height = " + halfHeight); 
    newAvatarPosition.y += (halfHeight * 1.5);
    MyAvatar.position = newAvatarPosition;
}

function inFrontOfMe() {
    return Vec3.sum(MyAvatar.position, Vec3.multiply(BALL_DROP_DISTANCE, Quat.getFront(MyAvatar.orientation)));
}

function avatarHalfHeight() {
    return MyAvatar.getDefaultEyePosition().y - MyAvatar.position.y;
}

function brakeBall(ball) {
    //  Check the ball's velocity and slow it down if beyond a threshold
    var properties = Entities.getEntityProperties(ball, ['velocity']);
    if (properties) {
        var velocity = Vec3.length(properties.velocity);
        if ((velocity > 0) && (velocity < MAX_BRAKING_SPEED)) {
            Entities.editEntity(ball, { velocity:  Vec3.multiply(BALL_BRAKING_RATE, properties.velocity) });
        }
    }  
}

function makeBall(position) {
    //  Create a new sphere entity
    ball = Entities.addEntity({
                type: "Sphere",
                position: position,
                color: { red: 255, green: 255, blue: 255 },
                dimensions: { x: BALL_DIAMETER, y: BALL_DIAMETER, z: BALL_DIAMETER },
                gravity: { x: 0, y: BALL_GRAVITY, z: 0 },
                velocity: { x: 0, y: BALL_START_VELOCITY, z: 0 },
                friction: 0.5,
                restitution: 0.5,
                shapeType: "sphere",
                dynamic: true,
                lifetime: BALL_LIFETIME,
                collisionSoundURL: collisionSoundURL
    });
}

function checkClub(params) {
    var hand = params[0] == "left" ? 0 : 1;
    var makeNewBall = false;
    if (triggerPulled(hand)) {
        //  If trigger just pulled, either drop new ball or go to existing one
        var position = ballPosition(ball);
        if (position && (Vec3.distance(MyAvatar.position, position) < BALL_MAX_RANGE)) {
            gotoPointOnGround(position, BALL_DROP_DISTANCE);
        } else {
            Entities.deleteEntity(ball);
            makeBall(inFrontOfMe());
        }
    }
    brakeBall(ball);
}

this.continueEquip = function(id, params) {
    // While holding the club, continuously check for trigger pull and brake ball if moving.
    checkClub(params);
}

});
