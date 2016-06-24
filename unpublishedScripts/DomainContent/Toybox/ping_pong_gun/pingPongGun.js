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

    Script.include("../libraries/utils.js");

    var SHOOTING_SOUND_URL = 'http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/ping_pong_gun/pong_sound.wav';
    var PING_PONG_BALL_URL = 'http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/ping_pong_gun/ping_pong_ball.fbx';

    function PingPongGun() {
        return;
    }

    //if the trigger value goes below this value, reload the gun.
    var RELOAD_THRESHOLD = 0.95;
    var GUN_TIP_FWD_OFFSET = -0.35;
    var GUN_TIP_UP_OFFSET = 0.12;
    var GUN_FORCE = 9;
    var BALL_RESTITUTION = 0.6;
    var BALL_LINEAR_DAMPING = 0.4;
    var BALL_GRAVITY = {
        x: 0,
        y: -4.8,
        z: 0
    };

    var BALL_DIMENSIONS = {
        x: 0.04,
        y: 0.04,
        z: 0.04
    };


    var BALL_COLOR = {
        red: 255,
        green: 255,
        blue: 255
    };

    var TRIGGER_CONTROLS = [
        Controller.Standard.LT,
        Controller.Standard.RT,
    ];


    PingPongGun.prototype = {
        hand: null,
        gunTipPosition: null,
        canShoot: false,
        canShootTimeout: null,

        startEquip: function(entityID, args) {
            this.hand = args[0] == "left" ? 0 : 1;
        },

        continueEquip: function(entityID, args) {
            if (this.canShootTimeout !== null) {
                Script.clearTimeout(this.canShootTimeout);
            }
            this.checkTriggerPressure(this.hand);
        },

        releaseEquip: function(entityID, args) {
            var _this = this;
            this.canShootTimeout = Script.setTimeout(function() {
                _this.canShoot = false;
            }, 250);
        },

        checkTriggerPressure: function(gunHand) {
            this.triggerValue = Controller.getValue(TRIGGER_CONTROLS[gunHand]);
            if (this.triggerValue < RELOAD_THRESHOLD) {
                // print('RELOAD');
                this.canShoot = true;
            } else if (this.triggerValue >= RELOAD_THRESHOLD && this.canShoot === true) {
                var gunProperties = Entities.getEntityProperties(this.entityID, ["position", "rotation"]);
                this.shootBall(gunProperties);
                this.canShoot = false;
            }

            return;
        },

        shootBall: function(gunProperties) {
            var forwardVec = Quat.getFront(Quat.multiply(gunProperties.rotation, Quat.fromPitchYawRollDegrees(0, 180, 0)));
            forwardVec = Vec3.normalize(forwardVec);
            forwardVec = Vec3.multiply(forwardVec, GUN_FORCE);

            var properties = {
                // type: 'Model',
                // modelURL:PING_PONG_BALL_URL,
                shapeType: 'sphere',
                type: 'Sphere',
                color: BALL_COLOR,
                dimensions: BALL_DIMENSIONS,
                damping: BALL_LINEAR_DAMPING,
                gravity: BALL_GRAVITY,
                restitution: BALL_RESTITUTION,
                dynamic: true,
                rotation: gunProperties.rotation,
                position: this.getGunTipPosition(gunProperties),
                velocity: forwardVec,
                lifetime: 10
            };

            Entities.addEntity(properties);

            this.playSoundAtCurrentPosition(gunProperties.position);
        },

        playSoundAtCurrentPosition: function(position) {
            var audioProperties = {
                volume: 0.2,
                position: position
            };

            Audio.playSound(this.SHOOTING_SOUND, audioProperties);
        },
        
        getGunTipPosition: function(properties) {
            //the tip of the gun is going to be in a different place than the center, so we move in space relative to the model to find that position
            var frontVector = Quat.getFront(properties.rotation);
            var frontOffset = Vec3.multiply(frontVector, GUN_TIP_FWD_OFFSET);
            var upVector = Quat.getUp(properties.rotation);
            var upOffset = Vec3.multiply(upVector, GUN_TIP_UP_OFFSET);

            var gunTipPosition = Vec3.sum(properties.position, frontOffset);
            gunTipPosition = Vec3.sum(gunTipPosition, upOffset);

            return gunTipPosition;
        },

        preload: function(entityID) {
            this.entityID = entityID;
            this.SHOOTING_SOUND = SoundCache.getSound(SHOOTING_SOUND_URL);
        }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new PingPongGun();
});