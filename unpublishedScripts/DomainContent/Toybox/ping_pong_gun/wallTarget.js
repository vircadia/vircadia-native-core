//  wallTarget.js
//
//  Script Type: Entity
//  Created by James B. Pollack @imgntn on 9/21/2015
//  Copyright 2015 High Fidelity, Inc.
//  
//  This script resets an object to its original position when it stops moving after a collision
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */
(function() {


    function Target() {
        return;
    }

    Target.prototype = {
        hasBecomeActive: false,
        preload: function(entityID) {
            this.entityID = entityID;
            var SOUND_URL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/ping_pong_gun/Clay_Pigeon_02.L.wav";
            this.hitSound = SoundCache.getSound(SOUND_URL);
        },
        collisionWithEntity: function(me, otherEntity) {
            if (this.hasBecomeActive === false) {
                var position = Entities.getEntityProperties(me, "position").position;
                Entities.editEntity(me, {
                    gravity: {
                        x: 0,
                        y: -9.8,
                        z: 0
                    },
                    velocity: {
                        x: 0,
                        y: -0.01,
                        z: 0
                    }
                });

                this.audioInjector = Audio.playSound(this.hitSound, {
                    position: position,
                    volume: 0.5
                });

                this.hasBecomeActive = true;

            }

        }
    };

    // entity scripts always need to return a newly constructed object of our type
    return new Target();
});