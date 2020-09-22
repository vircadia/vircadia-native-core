"use strict";
//
// dart.js
//
// Created by MrRoboman on 17/05/13
// Copyright 2017 High Fidelity, Inc.
//
// Simple throwing dart. Sticks to static objects.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() {
    var THROW_FACTOR = 3;
    var DART_SOUND_URL = Script.resolvePath(Script.getExternalPath(Script.ExternalPaths.HF_Content, '/wadewatts/dart.wav?v=') + "?" + Date.now());

    var Dart = function() {};

    Dart.prototype = {

        preload: function(entityID) {
            this.entityID = entityID;
            this.actionID = null;
            this.soundHasPlayed = true;
            this.dartSound = SoundCache.getSound(DART_SOUND_URL);
        },

        playDartSound: function(sound) {
            if (this.soundHasPlayed) {
                return;
            }
            this.soundHasPlayed = true;
            var position = Entities.getEntityProperties(this.entityID, 'position').position;
            var audioProperties = {
                volume: 0.15,
                position: position
            };
            Audio.playSound(this.dartSound, audioProperties);
        },

        releaseGrab: function() {
            this.soundHasPlayed = false;
            var velocity = Entities.getEntityProperties(this.entityID, 'velocity').velocity;

            var newVelocity = {};
            Object.keys(velocity).forEach(function(key) {
                newVelocity[key] = velocity[key] * THROW_FACTOR;
            });

            Entities.editEntity(this.entityID, {
                velocity: newVelocity
            });

            if (this.actionID) {
                Entities.deleteAction(this.entityID, this.actionID);
            }
            this.actionID = Entities.addAction("travel-oriented", this.entityID, {
                forward: { x: 0, y: 0, z: 1 },
                angularTimeScale: 0.1,
                tag: "throwing dart",
                ttl: 3600
            });
        },

        collisionWithEntity: function(myID, otherID, collisionInfo) {
            this.playDartSound();

            Entities.editEntity(myID, {
                velocity: {x: 0, y: 0, z: 0},
                angularVelocity: {x: 0, y: 0, z: 0}
            });

            if (this.actionID) {
                Entities.deleteAction(myID, this.actionID);
                this.actionID = null;
            }
        }
    };

    return new Dart();
});
