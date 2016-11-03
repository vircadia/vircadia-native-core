//
//
//  Created by The Content Team 4/10/216
//  Copyright 2016 High Fidelity, Inc.
//
// this finds lights and toggles thier visibility, and flips the emissive texture of some light models
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function() {
    var SEARCH_RADIUS = 10;

    var _this;
    var utilitiesScript = Script.resolvePath('../utils.js');
    Script.include(utilitiesScript);
    Switch = function() {
        _this = this;
        this.switchSound = SoundCache.getSound("atp:/switches/lamp_switch_2.wav");
    };

    Switch.prototype = {
        prefix: 'hifi-home-living-room-desk-lamp-',
        lightName: 'home_light_livingRoomLight',
        clickReleaseOnEntity: function(entityID, mouseEvent) {
            if (!mouseEvent.isLeftButton) {
                return;
            }
            this.toggleLights();
        },

        startNearGrab: function() {
            this.toggleLights();
        },

        modelEmitOn: function(discModel) {
            var data = {
                "Tex.Lamp-Bulldog": "atp:/kineticObjects/lamp/Lamp-Bulldog-Base.fbx/Lamp-Bulldog-Base.fbm/dog_statue.jpg",
                "Texture.001": "atp:/kineticObjects/lamp/Lamp-Bulldog-Base.fbx/Lamp-Bulldog-Base.fbm/Emissive-Map.png"
            }

            Entities.editEntity(glowDisc, {
                textures: JSON.stringify(data)
            })

        },

        modelEmitOff: function(discModel) {
            var data = {
                "Tex.Lamp-Bulldog": "atp:/kineticObjects/lamp/Lamp-Bulldog-Base.fbx/Lamp-Bulldog-Base.fbm/dog_statue.jpg",
                "Texture.001": ""
            };
            Entities.editEntity(glowDisc, {
                textures: JSON.stringify(data)
            })
        },

        masterLightOn: function(masterLight) {
            Entities.editEntity(masterLight, {
                visible: true
            });
        },

        masterLightOff: function(masterLight) {
            print("EBL TURN LIGHT OFF");
            Entities.editEntity(masterLight, {
                visible: false
            });
        },


        findMasterLights: function() {
            var found = [];
            var results = Entities.findEntities(_this.position, SEARCH_RADIUS);
            results.forEach(function(result) {
                var properties = Entities.getEntityProperties(result);
                if (properties.name === _this.lightName) {
                    print("EBL FOUND THE LIGHT!");
                    found.push(result);
                }
            });
            return found;
        },

        findEmitModels: function() {
            var found = [];
            var results = Entities.findEntities(this.position, SEARCH_RADIUS);
            results.forEach(function(result) {
                var properties = Entities.getEntityProperties(result);
                if (properties.name === _this.prefix + "model") {
                    found.push(result);
                }
            });
            return found;
        },

        toggleLights: function() {
            print("EBL TOGGLE LIGHTS");

            this._switch = getEntityCustomData('home-switch', this.entityID, {
                state: 'off'
            });

            var masterLights = this.findMasterLights();
            var emitModels = this.findEmitModels();

            if (this._switch.state === 'off') {
                print("EBL TURN LIGHTS ON");
                masterLights.forEach(function(masterLight) {
                    _this.masterLightOn(masterLight);
                });
                emitModels.forEach(function(emitModel) {
                    _this.modelEmitOn(emitModel);
                });
                setEntityCustomData('home-switch', this.entityID, {
                    state: 'on'
                });

            } else {
                print("EBL TURN LIGHTS OFF");
                masterLights.forEach(function(masterLight) {
                    _this.masterLightOff(masterLight);
                });
                emitModels.forEach(function(emitModel) {
                    _this.modelEmitOff(emitModel);
                });
                setEntityCustomData('home-switch', this.entityID, {
                    state: 'off'
                });
            }

            Audio.playSound(this.switchSound, {
                volume: 0.5,
                position: this.position
            });

        },


        preload: function(entityID) {
            print("EBL PRELOAD LIGHT SWITCH SCRIPT");
            this.entityID = entityID;
            var properties = Entities.getEntityProperties(this.entityID);


            //The light switch is static, so just cache its position once
            this.position = Entities.getEntityProperties(this.entityID, "position").position;
        }
    };

    // entity scripts always need to return a newly constructed object of our type
    return new Switch();
});