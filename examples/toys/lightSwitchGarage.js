//
//  lightSwitchGarage.js.js
//  examples/entityScripts
//
//  Created by Eric Levin on 9/21/15.
//  Copyright 2015 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    var _this;

    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    LightSwitchGarage = function() {
        _this = this;

        this.lightStateKey = "lightStateKey";
        this.resetKey = "resetMe";

        this.switchSound = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/sounds/Switches%20and%20sliders/lamp_switch_2.wav");

    };

    LightSwitchGarage.prototype = {

        clickReleaseOnEntity: function(entityID, mouseEvent) {
            if (!mouseEvent.isLeftButton) {
                return;
            }
            this.toggleLights();
        },

        startNearGrabNonColliding: function() {
            this.toggleLights();
        },

        toggleLights: function() {
            var defaultLightData = {
                on: false
            };
            var lightState = getEntityCustomData(this.lightStateKey, this.entityID, defaultLightData);
            if (lightState.on === true) {
                this.clearLights();
            } else if (lightState.on === false) {
                this.createLights();
            }

            this.flipLights();

            Audio.playSound(this.switchSound, {
                volume: 0.5,
                position: this.position
            });

        },

        clearLights: function() {
            var entities = Entities.findEntities(MyAvatar.position, 100);
            var self = this;
            entities.forEach(function(entity) {
                var resetData = getEntityCustomData(self.resetKey, entity, {})
                if (resetData.resetMe === true && resetData.lightType === "Sconce Light Garage") {
                    Entities.deleteEntity(entity);
                }
            });

            setEntityCustomData(this.lightStateKey, this.entityID, {
                on: false
            });
        },

        createLights: function() {

            var sconceLight3 = Entities.addEntity({
                type: "Light",
                position: {
                    x: 545.49468994140625,
                    y: 496.24026489257812,
                    z: 500.63516235351562
                },

                name: "Sconce 3 Light",
                dimensions: {
                    x: 2.545,
                    y: 2.545,
                    z: 2.545
                },
                cutoff: 90,
                color: {
                    red: 217,
                    green: 146,
                    blue: 24
                }
            });

            setEntityCustomData(this.resetKey, sconceLight3, {
                resetMe: true,
                lightType: "Sconce Light Garage"
            });

            var sconceLight4 = Entities.addEntity({
                type: "Light",
                position: {
                    x: 550.90399169921875,
                    y: 496.24026489257812,
                    z: 507.90237426757812
                },
                name: "Sconce 4 Light",
                dimensions: {
                    x: 2.545,
                    y: 2.545,
                    z: 2.545
                },
                cutoff: 90,
                color: {
                    red: 217,
                    green: 146,
                    blue: 24
                }
            });

            setEntityCustomData(this.resetKey, sconceLight4, {
                resetMe: true,
                lightType: "Sconce Light Garage"
            });

            var sconceLight5 = Entities.addEntity({
                type: "Light",
                position: {
                    x: 548.407958984375,
                    y: 496.24026489257812,
                    z: 509.5504150390625
                },
                name: "Sconce 5 Light",
                dimensions: {
                    x: 2.545,
                    y: 2.545,
                    z: 2.545
                },
                cutoff: 90,
                color: {
                    red: 217,
                    green: 146,
                    blue: 24
                }
            });

            setEntityCustomData(this.resetKey, sconceLight5, {
                resetMe: true,
                lightType: "Sconce Light Garage"
            });

            setEntityCustomData(this.lightStateKey, this.entityID, {
                on: true
            });
        },

        flipLights: function() {
            // flip model to give illusion of light switch being flicked
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

            //The light switch is static, so just cache its position once
            this.position = Entities.getEntityProperties(this.entityID, "position").position;
            var defaultLightData = {
                on: false
            };
            var lightState = getEntityCustomData(this.lightStateKey, this.entityID, defaultLightData);

            //If light is off, then we create two new lights- at the position of the sconces
            if (lightState.on === false) {
                this.createLights();
                this.flipLights();
            }
            //If lights are on, do nothing!
        },
    };

    // entity scripts always need to return a newly constructed object of our type
    return new LightSwitchGarage();
})
