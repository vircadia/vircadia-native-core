//
//  pistol.js
//  examples/toybox/entityScripts
//
//  Created by Eric Levin on11/11/15.
//  Copyright 2015 High Fidelity, Inc.
//

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */


(function() {
        Script.include("../../libraries/utils.js");
        Script.include("../../libraries/constants.js");

        var scriptURL = Script.resolvePath('pistol.js');
        var _this;
        Pistol = function() {
            _this = this;
            this.equipped = false;
            this.forceMultiplier = 1;
            this.laserOffsets = {
                y: .15
            };
        };

        Pistol.prototype = {

                startEquip: function(id, params) {
                    this.equipped = true;
                    this.hand = JSON.parse(params[0]);
                    Overlays.editOverlay(this.laser, {
                        visible: true
                    });
                    print("EQUIP")
                },

                continueNearGrab: function() {
                    if(!this.equipped) {
                        return;
                    }
                    this.updateLaser();
                },

                updateLaser: function() {
                    var gunProps = Entities.getEntityProperties(this.entityID, ['position', 'rotation']);
                    var position = gunProps.position;
                    var rotation = gunProps.rotation;
                    var direction = Quat.getFront(rotation);
                    var upVec = Quat.getUp(rotation);
                    position = Vec3.sum(position, Vec3.multiply(upVec, this.laserOffsets.y));
                    var tip = Vec3.sum(position, Vec3.multiply(direction, 10));
                    Overlays.editOverlay(this.laser, {
                        start: position,
                        end: tip,
                        alpha: 1
                    });
                },

                unequip: function() {
                    print("UNEQUIP")
                    this.hand = null;
                    this.equipped = false;
                    Overlays.editOverlay(this.laser, {visible: false});
                },

                preload: function(entityID) {
                    this.entityID = entityID;
                    print("INIT CONTROLLER MAPIING")
                    this.initControllerMapping();
                    this.laser = Overlays.addOverlay("line3d", {
                        start: ZERO_VECTOR,
                        end: ZERO_VECTOR,
                        color: COLORS.RED,
                        alpha: 1,
                        visible: true,
                        lineWidth: 2
                    });
            },

            triggerPress: function(hand, value) {
                print("TRIGGER PRESS");
                if (this.hand === hand && value === 1) {
                    //We are pulling trigger on the hand we have the gun in, so fire
                    this.fire();
                }
            },

            fire: function() {
                print("FIRE!");
            },

            initControllerMapping: function() {
                this.mapping = Controller.newMapping();
                this.mapping.from(Controller.Standard.LT).hysteresis(0.0, 0.5).to(function(value) {
                    _this.triggerPress(0, value);
                });


                this.mapping.from(Controller.Standard.RT).hysteresis(0.0, 0.5).to(function(value) {
                    _this.triggerPress(1, value);
                });
                this.mapping.enable();

            },

            unload: function() {
                this.mapping.disable();
                Overlays.deleteOverlay(this.laser); }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new Pistol();
});