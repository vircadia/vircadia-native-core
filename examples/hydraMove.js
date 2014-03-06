//
//  hydraMove.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/10/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates use of the Controller and MyAvatar classes to implement
//  avatar flying through the hydra/controller joysticks
//
//

var damping = 0.9;
var position = { x: MyAvatar.position.x, y: MyAvatar.position.y, z: MyAvatar.position.z };
var joysticksCaptured = false;
var THRUST_CONTROLLER = 0;
var VIEW_CONTROLLER = 1;
var INITIAL_THRUST_MULTPLIER = 1.0;
var THRUST_INCREASE_RATE = 1.05;
var MAX_THRUST_MULTIPLIER = 75.0;
var thrustMultiplier = INITIAL_THRUST_MULTPLIER;
var grabDelta = { x: 0, y: 0, z: 0};
var grabDeltaVelocity = { x: 0, y: 0, z: 0};
var grabStartRotation = { x: 0, y: 0, z: 0, w: 1};
var grabCurrentRotation = { x: 0, y: 0, z: 0, w: 1};
var grabbingWithRightHand = false;
var wasGrabbingWithRightHand = false;
var grabbingWithLeftHand = false;
var wasGrabbingWithLeftHand = false;
var EPSILON = 0.000001;
var velocity = { x: 0, y: 0, z: 0};
var THRUST_MAG_UP = 800.0;
var THRUST_MAG_DOWN = 300.0;
var THRUST_MAG_FWD = 500.0;
var THRUST_MAG_BACK = 300.0;
var THRUST_MAG_LATERAL = 250.0;
var THRUST_JUMP = 120.0;

var YAW_MAG = 500.0;
var PITCH_MAG = 100.0;
var THRUST_MAG_HAND_JETS = THRUST_MAG_FWD;
var JOYSTICK_YAW_MAG = YAW_MAG;
var JOYSTICK_PITCH_MAG = PITCH_MAG * 0.5;


var LEFT_PALM = 0;
var LEFT_BUTTON_4 = 4;
var RIGHT_PALM = 2;
var RIGHT_BUTTON_4 = 10;

// Used by handleGrabBehavior() for managing the grab position changes
function getAndResetGrabDelta() {
    var HAND_GRAB_SCALE_DISTANCE = 2.0;
    var delta = Vec3.multiply(grabDelta, (MyAvatar.scale * HAND_GRAB_SCALE_DISTANCE));
    grabDelta = { x: 0, y: 0, z: 0};
    var avatarRotation = MyAvatar.orientation;
    var result = Vec3.multiplyQbyV(avatarRotation, Vec3.multiply(delta, -1));
    return result;
}

// Used by handleGrabBehavior() for managing the grab velocity feature
function getAndResetGrabDeltaVelocity() {
    var HAND_GRAB_SCALE_VELOCITY = 50.0;
    var delta = Vec3.multiply(grabDeltaVelocity, (MyAvatar.scale * HAND_GRAB_SCALE_VELOCITY));
    grabDeltaVelocity = { x: 0, y: 0, z: 0};
    var avatarRotation = MyAvatar.orientation;
    var result = Quat.multiply(avatarRotation, Vec3.multiply(delta, -1));
    return result;
}

// Used by handleGrabBehavior() for managing the grab rotation feature
function getAndResetGrabRotation() {
    var quatDiff = Quat.multiply(grabCurrentRotation, Quat.inverse(grabStartRotation));
    grabStartRotation = grabCurrentRotation;
    return quatDiff;
}

// handles all the grab related behavior: position (crawl), velocity (flick), and rotate (twist)
function handleGrabBehavior(deltaTime) {
    // check for and handle grab behaviors
    grabbingWithRightHand = Controller.isButtonPressed(RIGHT_BUTTON_4);
    grabbingWithLeftHand = Controller.isButtonPressed(LEFT_BUTTON_4);
    stoppedGrabbingWithLeftHand = false;
    stoppedGrabbingWithRightHand = false;
    
    if (grabbingWithRightHand && !wasGrabbingWithRightHand) {
        // Just starting grab, capture starting rotation
        grabStartRotation = Controller.getSpatialControlRawRotation(RIGHT_PALM);
    }
    if (grabbingWithRightHand) {
        grabDelta = Vec3.sum(grabDelta, Vec3.multiply(Controller.getSpatialControlVelocity(RIGHT_PALM), deltaTime));
        grabCurrentRotation = Controller.getSpatialControlRawRotation(RIGHT_PALM);
    }
    if (!grabbingWithRightHand && wasGrabbingWithRightHand) {
        // Just ending grab, capture velocity
        grabDeltaVelocity = Controller.getSpatialControlVelocity(RIGHT_PALM);
        stoppedGrabbingWithRightHand = true;
    }

    if (grabbingWithLeftHand && !wasGrabbingWithLeftHand) {
        // Just starting grab, capture starting rotation
        grabStartRotation = Controller.getSpatialControlRawRotation(LEFT_PALM);
    }

    if (grabbingWithLeftHand) {
        grabDelta = Vec3.sum(grabDelta, Vec3.multiply(Controller.getSpatialControlVelocity(LEFT_PALM), deltaTime));
        grabCurrentRotation = Controller.getSpatialControlRawRotation(LEFT_PALM);
    }
    if (!grabbingWithLeftHand && wasGrabbingWithLeftHand) {
        // Just ending grab, capture velocity
        grabDeltaVelocity = Controller.getSpatialControlVelocity(LEFT_PALM);
        stoppedGrabbingWithLeftHand = true;
    }

    grabbing = grabbingWithRightHand || grabbingWithLeftHand;
    stoppedGrabbing = stoppedGrabbingWithRightHand || stoppedGrabbingWithLeftHand;

    if (grabbing) {
    
        // move position
        var moveFromGrab = getAndResetGrabDelta();
        if (Vec3.length(moveFromGrab) > EPSILON) {
            MyAvatar.position = Vec3.sum(MyAvatar.position, moveFromGrab);
            velocity = { x: 0, y: 0, z: 0};
        }
        
        // add some rotation...
        var deltaRotation = getAndResetGrabRotation();
        var GRAB_CONTROLLER_TURN_SCALING = 0.5;
        var euler = Vec3.multiply(Quat.safeEulerAngles(deltaRotation), GRAB_CONTROLLER_TURN_SCALING);
    
        //  Adjust body yaw by yaw from controller
        var orientation = Quat.multiply(Quat.angleAxis(-euler.y, {x:0, y: 1, z:0}), MyAvatar.orientation);
        MyAvatar.orientation = orientation;

        //  Adjust head pitch from controller
        MyAvatar.headPitch = MyAvatar.headPitch - euler.x;
    }

    // add some velocity...
    if (stoppedGrabbing) {
        velocity = Vec3.sum(velocity, getAndResetGrabDeltaVelocity());
    }

    // handle residual velocity
    if(Vec3.length(velocity) > EPSILON) {
        MyAvatar.position = Vec3.sum(MyAvatar.position, Vec3.multiply(velocity, deltaTime));
        // damp velocity
        velocity = Vec3.multiply(velocity, damping);
    }
    
    
    wasGrabbingWithRightHand = grabbingWithRightHand;
    wasGrabbingWithLeftHand = grabbingWithLeftHand;
}

// Main update function that handles flying and grabbing behaviort
function flyWithHydra(deltaTime) {
    var thrustJoystickPosition = Controller.getJoystickPosition(THRUST_CONTROLLER);
    
    if (thrustJoystickPosition.x != 0 || thrustJoystickPosition.y != 0) {
        if (thrustMultiplier < MAX_THRUST_MULTIPLIER) {
            thrustMultiplier *= 1 + (deltaTime * THRUST_INCREASE_RATE);
        }
        var currentOrientation = MyAvatar.orientation;

        var front = Quat.getFront(currentOrientation);
        var right = Quat.getRight(currentOrientation);
        var up = Quat.getUp(currentOrientation);
    
        var thrustFront = Vec3.multiply(front, MyAvatar.scale * THRUST_MAG_HAND_JETS * 
                                        thrustJoystickPosition.y * thrustMultiplier * deltaTime);
        MyAvatar.addThrust(thrustFront);
        var thrustRight = Vec3.multiply(right, MyAvatar.scale * THRUST_MAG_HAND_JETS * 
                                        thrustJoystickPosition.x * thrustMultiplier * deltaTime);
        MyAvatar.addThrust(thrustRight);
    } else {
        thrustMultiplier = INITIAL_THRUST_MULTPLIER;
    }

    // View Controller
    var viewJoystickPosition = Controller.getJoystickPosition(VIEW_CONTROLLER);
    if (viewJoystickPosition.x != 0 || viewJoystickPosition.y != 0) {

        // change the body yaw based on our x controller
        var orientation = MyAvatar.orientation;
        var deltaOrientation = Quat.fromPitchYawRoll(0, (-1 * viewJoystickPosition.x * JOYSTICK_YAW_MAG * deltaTime), 0);
        MyAvatar.orientation = Quat.multiply(orientation, deltaOrientation);

        // change the headPitch based on our x controller
        //pitch += viewJoystickPosition.y * JOYSTICK_PITCH_MAG * deltaTime;
        var newPitch = MyAvatar.headPitch + (viewJoystickPosition.y * JOYSTICK_PITCH_MAG * deltaTime);
        MyAvatar.headPitch = newPitch;
    }
    handleGrabBehavior(deltaTime);
}
    
Script.update.connect(flyWithHydra);
Controller.captureJoystick(THRUST_CONTROLLER);
Controller.captureJoystick(VIEW_CONTROLLER);

// Map keyPress and mouse move events to our callbacks
function scriptEnding() {
    // re-enabled the standard application for touch events
    Controller.releaseJoystick(THRUST_CONTROLLER);
    Controller.releaseJoystick(VIEW_CONTROLLER);
}
Script.scriptEnding.connect(scriptEnding);

