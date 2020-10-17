//
//  lightSwitch.js
//  examples/entityScripts
//
//  Created by Eric Levin on 10/2/15.
//  Copyright 2015 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, pointInExtents, vec3equal, setEntityCustomData, getEntityCustomData */
//per script

/*global  LightSwitch */

(function() {
    var _this;
    var utilitiesScript = Script.resolvePath("../libraries/utils.js");
    Script.include(utilitiesScript);
    LightSwitch = function() {
        _this = this;
        this.switchSound = SoundCache.getSound("https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/lights/lamp_switch_2.wav");
    };

    LightSwitch.prototype = {

        clickReleaseOnEntity: function(entityID, mouseEvent) {
            if (!mouseEvent.isLeftButton) {
                return;
            }
            this.toggleLights();
        },

        startNearTrigger: function() {
            this.toggleLights();
        },

        toggleLights: function() {
            var lightData = getEntityCustomData(this.resetKey, this.entityID, {});
            var on = !lightData.on;
            var lightType = lightData.type;

            var lights = Entities.findEntities(this.position, 20);
            lights.forEach(function(light) {
                var type = getEntityCustomData(_this.resetKey, light, {}).type;
                if (type === lightType && JSON.stringify(light) !== JSON.stringify(_this.entityID)) {
                    Entities.editEntity(light, {
                        visible: on
                    });

                }
            });
            this.flipSwitch();
            Audio.playSound(this.switchSound, {
                volume: 0.5,
                position: this.position
            });

            setEntityCustomData(this.resetKey, this.entityID, {
                on: on,
                type: lightType,
                resetMe: true
            });

            setEntityCustomData('grabbableKey', this.entityID, {
                wantsTrigger: true
            });
        },

        flipSwitch: function() {
            var rotation = Entities.getEntityProperties(this.entityID, "rotation").rotation;
            var axis = {
                x: 0,
                y: 1,
                z: 0
            };
            var dQ = Quat.angleAxis(180, axis);
            rotation = Quat.multiply(rotation, dQ);

            Entities.editEntity(this.entityID, {
                rotation: rotation
            });
        },
        preload: function(entityID) {
            this.entityID = entityID;
            this.resetKey = "resetMe";
            //The light switch is static, so just cache its position once
            this.position = Entities.getEntityProperties(this.entityID, "position").position;
        }
    };

    // entity scripts always need to return a newly constructed object of our type
    return new LightSwitch();
});