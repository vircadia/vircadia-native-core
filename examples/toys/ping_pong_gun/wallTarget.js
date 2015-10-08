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
    var TARGET_USER_DATA_KEY = 'hifi-ping_pong_target';
    var defaultTargetData = {
        originalPosition: null
    };

    var _this;
    function Target() {
        _this=this;
        return;
    }

    Target.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
            var targetData = getEntityCustomData(TARGET_USER_DATA_KEY, entityID, defaultTargetData);
            this.originalPosition=targetData.originalPosition;
            print('TARGET ORIGINAL POSITION:::'+targetData.originalPosition.x);
        },
        collisionWithEntity: function(me, otherEntity) {
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
            })
        }
    };

    // entity scripts always need to return a newly constructed object of our type
    return new Target();
});