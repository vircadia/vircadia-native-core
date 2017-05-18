"use strict";
/* jslint vars: true, plusplus: true, forin: true*/
/* globals Tablet, Script, AvatarList, Users, Entities, MyAvatar, Camera, Overlays, Vec3, Quat, Controller, print, getControllerWorldLocation */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */
//
// floatinLantern.js
//
// Created by MrRoboman on 17/05/04
// Copyright 2017 High Fidelity, Inc.
//
// Makes floating lanterns rise upon being released and corrects their rotation as the fly.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() {
    var _this;

    var SLOW_SPIN_THRESHOLD = 0.1;
    var ROTATION_COMPLETE_THRESHOLD = 0.01;
    var ROTATION_SPEED = 0.2;
    var HOME_ROTATION = {x: 0, y: 0, z: 0, w: 0};


    floatingLantern = function() {
        _this = this;
        this.updateConnected = false;
    };

    floatingLantern.prototype = {

        preload: function(entityID) {
            this.entityID = entityID;
        },

        unload: function(entityID) {
            this.disconnectUpdate();
        },

        startNearGrab: function() {
            this.disconnectUpdate();
        },

        startDistantGrab: function() {
            this.disconnectUpdate();
        },

        releaseGrab: function() {
            Entities.editEntity(this.entityID, {
                gravity: {
                    x: 0,
                    y: 0.5,
                    z: 0
                }
            });
        },

        update: function(dt) {
            var lanternProps = Entities.getEntityProperties(_this.entityID);

            if (lanternProps && lanternProps.rotation && lanternProps.owningAvatarID === MyAvatar.sessionUUID) {

                var spinningSlowly = (
                    Math.abs(lanternProps.angularVelocity.x) < SLOW_SPIN_THRESHOLD &&
                    Math.abs(lanternProps.angularVelocity.y) < SLOW_SPIN_THRESHOLD &&
                    Math.abs(lanternProps.angularVelocity.z) < SLOW_SPIN_THRESHOLD
                );

                var rotationComplete = (
                    Math.abs(lanternProps.rotation.x - HOME_ROTATION.x) < ROTATION_COMPLETE_THRESHOLD &&
                    Math.abs(lanternProps.rotation.y - HOME_ROTATION.y) < ROTATION_COMPLETE_THRESHOLD &&
                    Math.abs(lanternProps.rotation.z - HOME_ROTATION.z) < ROTATION_COMPLETE_THRESHOLD
                );

                if (spinningSlowly && !rotationComplete) {
                    var newRotation = Quat.slerp(lanternProps.rotation, HOME_ROTATION, ROTATION_SPEED * dt);

                    Entities.editEntity(_this.entityID, {
                        rotation: newRotation,
                        angularVelocity: {
                            x: 0,
                            y: 0,
                            z: 0
                        }
                    });
                }
            }
        },

        connectUpdate: function() {
            if (!this.updateConnected) {
                this.updateConnected = true;
                Script.update.connect(this.update);
            }
        },

        disconnectUpdate: function() {
            if (this.updateConnected) {
                this.updateConnected = false;
                Script.update.disconnect(this.update);
            }
        }
    };

    return new floatingLantern();
});
