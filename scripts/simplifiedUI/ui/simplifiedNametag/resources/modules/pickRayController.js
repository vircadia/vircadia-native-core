//
//  Simplified Nametag
//  pickRayController.js
//  Created by Milad Nazeri on 2019-03-08
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Easy pickray controllers for Entities, Overlays, and Avatars
//


var _this;
function PickRayController(){
    _this = this;

    _this.rayType = null;
    _this.eventHandler = null;
    _this.intersection = null;
    _this.lastPick = null;
    _this.currentPick = null;
    _this.mappingName = null;
    _this.mapping = null;
    _this._boundMousePressHandler = null;
    _this.shouldDoublePress = null;
    _this.controllEnabled = false;
}

 
// *************************************
// START UTILITY
// *************************************
// #region UTILITY


// Returns the right UUID based on hand triggered
function getUUIDFromLaser(hand) {
    hand = hand === Controller.Standard.LeftHand
        ? Controller.Standard.LeftHand
        : Controller.Standard.RightHand;

    var pose = getControllerWorldLocation(hand);
    var start = pose.position;
    // Get the direction that the hand is facing in the world
    var direction = Vec3.multiplyQbyV(pose.orientation, [0, 1, 0]);

    pickRayTypeHandler(start, direction);

    if (_this.currentPick) {
        _this.eventHandler(_this.currentPick, _this.intersection);
    }
}


// The following two functions are a modified version of what's found in scripts/system/libraries/controllers.js

// Utility function for the ControllerWorldLocation offset 
function getGrabPointSphereOffset(handController) {
    // These values must match what's in scripts/system/libraries/controllers.js
    // x = upward, y = forward, z = lateral
    var GRAB_POINT_SPHERE_OFFSET = { x: 0.04, y: 0.13, z: 0.039 };
    var offset = GRAB_POINT_SPHERE_OFFSET;
    if (handController === Controller.Standard.LeftHand) {
        offset = {
            x: -GRAB_POINT_SPHERE_OFFSET.x,
            y: GRAB_POINT_SPHERE_OFFSET.y,
            z: GRAB_POINT_SPHERE_OFFSET.z
        };
    }

    return Vec3.multiply(MyAvatar.sensorToWorldScale, offset);
}


// controllerWorldLocation is where the controller would be, in-world, with an added offset
function getControllerWorldLocation(handController, doOffset) {
    var orientation;
    var position;
    var valid = false;

    if (handController >= 0) {
        var pose = Controller.getPoseValue(handController);
        valid = pose.valid;
        var controllerJointIndex;
        if (pose.valid) {
            if (handController === Controller.Standard.RightHand) {
                controllerJointIndex = MyAvatar.getJointIndex("_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND");
            } else {
                controllerJointIndex = MyAvatar.getJointIndex("_CAMERA_RELATIVE_CONTROLLER_LEFTHAND");
            }
            orientation = Quat.multiply(MyAvatar.orientation, MyAvatar.getAbsoluteJointRotationInObjectFrame(controllerJointIndex));
            position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, MyAvatar.getAbsoluteJointTranslationInObjectFrame(controllerJointIndex)));

            // add to the real position so the grab-point is out in front of the hand, a bit
            if (doOffset) {
                var offset = getGrabPointSphereOffset(handController);
                position = Vec3.sum(position, Vec3.multiplyQbyV(orientation, offset));
            }

        } else if (!HMD.isHandControllerAvailable()) {
            // NOTE: keep _this offset in sync with scripts/system/controllers/handControllerPointer.js:493
            var VERTICAL_HEAD_LASER_OFFSET = 0.1 * MyAvatar.sensorToWorldScale;
            position = Vec3.sum(Camera.position, Vec3.multiplyQbyV(Camera.orientation, { x: 0, y: VERTICAL_HEAD_LASER_OFFSET, z: 0 }));
            orientation = Quat.multiply(Camera.orientation, Quat.angleAxis(-90, { x: 1, y: 0, z: 0 }));
            valid = true;
        }
    }

    return {
        position: position,
        translation: position,
        orientation: orientation,
        rotation: orientation,
        valid: valid
    };
}


// Handle if the uuid picked on is new or different
function handleUUID(uuid){
    if (!_this.lastPick && !_this.currentPick) {
        _this.currentPick = uuid;
        _this.lastPick = uuid;
    } else {
        _this.lastPick = _this.currentPick;
        _this.currentPick = uuid;
    }
}


function pickRayTypeHandler(pickRay){
    // Handle if pickray is system generated or user generated
    if (arguments.length === 2) {
        pickRay = { origin: arguments[0], direction: arguments[1] };
    }

    // Each different ray pick type needs a different findRayIntersection function
    switch (_this.rayType) {
        case 'avatar':
            var avatarIntersection = AvatarList.findRayIntersection(pickRay, [], [MyAvatar.sessionUUID], false);
            _this.intersection = avatarIntersection;
            handleUUID(avatarIntersection.avatarID);
            break;
        case 'local':
            var overlayIntersection = Overlays.findRayIntersection(pickRay, [], []);
            _this.intersection = overlayIntersection;
            handleUUID(overlayIntersection.overlayID);
            break;
        case 'entity':
            var entityIntersection = Entities.findRayIntersection(pickRay, [], []);
            _this.intersection = entityIntersection;
            handleUUID(entityIntersection.avatarID);
            break;
        default:
            console.log("ray type not handled");
    }
}


// Handle the interaction when in desktop and a mouse is pressed
function mousePressHandler(event) {
    if (HMD.active || !event.isLeftButton) {
        return;
    }
    var pickRay = Camera.computePickRay(event.x, event.y);
    pickRayTypeHandler(pickRay);
    if (_this.currentPick) {
        _this.eventHandler(_this.currentPick, _this.intersection);
    }
}


// Function to call when double press is singled
function doublePressHandler(event) {
    mousePressHandler(event);
}


// #endregion
// *************************************
// END UTILITY
// *************************************

// *************************************
// START API
// *************************************
// #region API


// After setup is given, this gets the Controller ready to be enabled
function create(){
    _this.mapping = Controller.newMapping(_this.mappingName);

    _this.mapping.from(Controller.Standard.LTClick).to(function (value) {
        if (value === 0) {
            return;
        }

        getUUIDFromLaser(Controller.Standard.LeftHand);
    });


    _this.mapping.from(Controller.Standard.RTClick).to(function (value) {
        if (value === 0) {
            return;
        }

        getUUIDFromLaser(Controller.Standard.RightHand);
    });

    return _this;
}


// Set type of raypick for what kind of uuids to return
function setType(type){
    _this.rayType = type;

    return _this;
}


// Set if double presses should register as well
function setShouldDoublePress(shouldDoublePress){
    _this.shouldDoublePress = shouldDoublePress;

    return _this;
}


// Set the mapping name for the controller
function setMapName(name) {
    _this.mappingName = name;

    return _this;
}


// Enables mouse press and trigger events  
function enable(){
    if (!_this.controllEnabled) {
        Controller.mousePressEvent.connect(mousePressHandler);
        if (_this.shouldDoublePress) {
            Controller.mouseDoublePressEvent.connect(doublePressHandler);
        }
        Controller.enableMapping(_this.mappingName);
        _this.controllEnabled = true;

        return _this;
    }

    return -1;
}


// Disable the controller and mouse press
function disable(){
    if (_this.controllEnabled) {
        Controller.mousePressEvent.disconnect(mousePressHandler);
        if (_this.shouldDoublePress){
            Controller.mouseDoublePressEvent.disconnect(doublePressHandler);
        }
        Controller.disableMapping(_this.mappingName);
        _this.controllEnabled = false;

        return _this;
    }

    return -1;
}


// Synonym for disable
function destroy(){
    _this.disable();
}


// Register the function to be called on a click
function registerEventHandler(fn){
    _this.eventHandler = fn;

    return _this;
}


// #endregion
// *************************************
// END API
// *************************************

PickRayController.prototype = {
    create: create,
    setType: setType,
    setShouldDoublePress: setShouldDoublePress,
    setMapName: setMapName,
    enable: enable,
    disable: disable,
    destroy: destroy,
    registerEventHandler: registerEventHandler
};


module.exports = PickRayController;
