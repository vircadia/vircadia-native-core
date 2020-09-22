"use strict";
/* jslint vars: true, plusplus: true, forin: true*/
/* globals Tablet, Script, AvatarList, Users, Entities, MyAvatar, Camera, Overlays, Vec3, Quat, Controller, print, getControllerWorldLocation */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */
//
// floatingLanternBox.js
//
// Created by MrRoboman on 17/05/04
// Copyright 2017 High Fidelity, Inc.
//
// Spawns new floating lanterns every couple seconds if the old ones have been removed.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() {

    var _this;
    var LANTERN_MODEL_URL = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/DomainContent/Welcome%20Area/Models/chinaLantern_capsule.fbx");
    var LANTERN_SCRIPT_URL = Script.resolvePath("floatingLantern.js?v=" + Date.now());
    var LIFETIME = 120;
    var RESPAWN_INTERVAL = 1000;
    var MAX_LANTERNS = 4;
    var SCALE_FACTOR = 1;

    var LANTERN = {
        type: "Model",
        name: "Floating Lantern",
        description: "Spawns Lanterns that float away when grabbed and released!",
        modelURL: LANTERN_MODEL_URL,
        script: LANTERN_SCRIPT_URL,
        dimensions: {
            x: 0.2049 * SCALE_FACTOR,
            y: 0.4 * SCALE_FACTOR,
            z: 0.2049 * SCALE_FACTOR
        },
        gravity: {
            x: 0,
            y: -1,
            z: 0
        },
        velocity: {
            x: 0, y: .01, z: 0
        },
        linearDampening: 0,
        shapeType: 'Box',
        lifetime: LIFETIME,
        dynamic: true
    };

    lanternBox = function() {
        _this = this;
    };

    lanternBox.prototype = {

        preload: function(entityID) {
            this.entityID = entityID;
            var props = Entities.getEntityProperties(this.entityID);

            if (props.owningAvatarID === MyAvatar.sessionUUID) {
                this.respawnTimer = Script.setInterval(this.spawnAllLanterns.bind(this), RESPAWN_INTERVAL);
            }
        },

        unload: function(entityID) {
            if (this.respawnTimer) {
                Script.clearInterval(this.respawnTimer);
            }
        },

        spawnAllLanterns: function() {
            var props = Entities.getEntityProperties(this.entityID);
            var lanternCount = 0;
            var nearbyEntities = Entities.findEntities(props.position, props.dimensions.x * 0.75);

            for (var i = 0; i < nearbyEntities.length; i++) {
                var name = Entities.getEntityProperties(nearbyEntities[i], ["name"]).name;
                if (name === "Floating Lantern") {
                    lanternCount++;
                }
            }

            while (lanternCount++ < MAX_LANTERNS) {
                this.spawnLantern();
            }
        },

        spawnLantern: function() {
            var boxProps = Entities.getEntityProperties(this.entityID);

            LANTERN.position = boxProps.position;
            LANTERN.position.x += Math.random() * .2 - .1;
            LANTERN.position.y += Math.random() * .2 + .1;
            LANTERN.position.z += Math.random() * .2 - .1;
            LANTERN.owningAvatarID = boxProps.owningAvatarID;

            return Entities.addEntity(LANTERN);
        }
    };

    return new lanternBox();
});
