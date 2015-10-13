//
//  whiteBoardEntityScript.js
//  examples/painting/whiteboard
//
//  Created by Eric Levin on 10/12/15.
//  Copyright 2015 High Fidelity, Inc.
//

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, pointInExtents, vec3equal, setEntityCustomData, getEntityCustomData */

/*global Whiteboard */

(function() {

    var _this;
    var RIGHT_HAND = 1;
    var LEFT_HAND = 0;
    var SPATIAL_CONTROLLERS_PER_PALM = 2;
    var TIP_CONTROLLER_OFFSET = 1;
    Whiteboard = function() {
        _this = this;
        print("WAAAAAH");
    };

    Whiteboard.prototype = {

        setRightHand: function() {
            this.hand = RIGHT_HAND;
        },

        setLeftHand: function() {
            this.hand = LEFT_HAND;
        },

        startFarGrabNonColliding: function() {
            if (this.hand === RIGHT_HAND) {
                this.getHandPosition = MyAvatar.getRightPalmPosition;
                this.getHandRotation = MyAvatar.getRightPalmRotation;
                this.triggerAction = Controller.findAction("RIGHT_HAND_CLICK");
            } else if (this.hand === LEFT_HAND) {
                this.getHandPosition = MyAvatar.getLeftPalmPosition;
                this.getHandRotation = MyAvatar.getLeftPalmRotation;
                this.triggerAction = Controller.findAction("LEFT_HAND_CLICK");
            }
        },

        continueFarGrabbingNonColliding: function() {
            var handPosition = this.getHandPosition();
            var pickRay = {
                origin: handPosition,
                direction: Quat.getUp(this.getHandRotation())
            };

            this.intersection = Entities.findRayIntersection(pickRay, true, [this.entityID]);
            // Comment above line and uncomment below line to test difference in judder when whitelist is and is not provided
            // this.intersection = Entities.findRayIntersection(pickRay, true);
        },

        releaseGrab: function() {

        },

        preload: function(entityID) {
            this.entityID = entityID;
            this.position = Entities.getEntityProperties(this.entityID, "position").position;
        },

    };

    // entity scripts always need to return a newly constructed object of our type
    return new Whiteboard();
});