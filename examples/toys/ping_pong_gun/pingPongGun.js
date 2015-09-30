//  pingPongGun.js
//
//  Script Type: Entity
//  Created by James B. Pollack @imgntn on 9/21/2015
//  Copyright 2015 High Fidelity, Inc.
//  
//  This script shoots a ping pong ball.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */
(function() {

    Script.include("../../libraries/utils.js");

    var SHOOTING_SOUND_URL = 'http://hifi-public.s3.amazonaws.com/sounds/Switches%20and%20sliders/flashlight_on.wav';
    var MODEL_URL = 'http://hifi-public.s3.amazonaws.com/models/ping_pong_gun/ping_pong_gun.fbx'

    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    function PingPongGun() {
        return;
    }

    //if the trigger value goes below this value, reload the gun.
    var RELOAD_THRESHOLD = 0.7;

    var GUN_TIP_OFFSET = 0.095;
    // Evaluate the world light entity positions and orientations from the model ones
    function evalLightWorldTransform(modelPos, modelRot) {

        return {
            p: Vec3.sum(modelPos, Vec3.multiplyQbyV(modelRot, MODEL_LIGHT_POSITION)),
            q: Quat.multiply(modelRot, MODEL_LIGHT_ROTATION)
        };
    }

    PingPongGun.prototype = {
        hand: null,
        whichHand: null,
        gunTipPosition: null,
        setRightHand: function() {
            this.hand = 'RIGHT';
        },

        setLeftHand: function() {
            this.hand = 'LEFT';
        },

        startNearGrab: function() {
            setWhichHand();
        },

        setWhichHand: function() {
            this.whichHand = this.hand;
        },

        continueNearGrab: function() {
            if (this.whichHand === null) {
                //only set the active hand once -- if we always read the current hand, our 'holding' hand will get overwritten
                this.setWhichHand();
            } else {
                this.checkTriggerPressure(this.whichHand);
            }
        },

        releaseGrab: function() {

        },

        checkTriggerPressure: function(gunHand) {
            var handClickString = gunHand + "_HAND_CLICK";

            var handClick = Controller.findAction(handClickString);

            this.triggerValue = Controller.getActionValue(handClick);

            if (this.triggerValue < RELOAD_THRESHOLD) {
                this.canShoot = true;
            } else if (this.triggerValue >= RELOAD_THRESHOLD && this.canShoot === true) {
                var gunProperties = Entities.getEntityProperties(this.entityID,["position","rotation"])
                this.shootBall(gunProperties);
                this.canShoot = false;
            }
            return;
        },

        shootBall: function(gunProperties,triggerValue) {
        var forwardVec = Quat.getFront(Quat.multiply(gunProperties.rotation , Quat.fromPitchYawRollDegrees(0, 90, 0)));
       //forwardVec = Vec3.normalize(forwardVec);

            var properties = {
                type: 'Sphere'
                color: {
                    red: 0,
                    green: 0,
                    blue: 255
                },
                dimensions: {
                    x: 0.04,
                    y: 0.04,
                    z: 0.04
                },
                linearDamping: 0.2,
                gravity: {
                    x: 0,
                    y: -9.8,
                    z: 0
                },
                rotation:gunProperties.rotation,
                position: this.gunTipPosition,
                velocity: velocity,
                lifetime: 10
            };
            var pingPongBall = Entities.addEntity(properties);
        },

        playSoundAtCurrentPosition: function(playOnSound) {
            var position = Entities.getEntityProperties(this.entityID, "position").position;

            var audioProperties = {
                volume: 0.25,
                position: position
            };

            if (playOnSound) {
                Audio.playSound(this.ON_SOUND, audioProperties);
            } else {
                Audio.playSound(this.OFF_SOUND, audioProperties);
            }
        },

        getGunTipPosition: function(properties) {
            //the tip of the gun is going to be in a different place than the center, so we move in space relative to the model to find that position
            var upVector = Quat.getUp(properties.rotation);
            var upOffset = Vec3.multiply(upVector, GUN_TIP_OFFSET);
            var wandTipPosition = Vec3.sum(properties.position, upOffset);
            return wandTipPosition;
        },
        preload: function(entityID) {
            this.entityID = entityID;
            this.ON_SOUND = SoundCache.getSound(SHOOT_SOUND_URL);

        },

        unload: function() {

        },

    };

    // entity scripts always need to return a newly constructed object of our type
    return new Flashlight();
});